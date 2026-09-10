#include "sar/pipeline/pipeline_orchestrator.hpp"
#include <chrono>
#include <cmath>
#include <map>

using namespace std;
using namespace std::chrono;

namespace sar::pipeline {

PipelineOrchestrator::PipelineOrchestrator(const PipelineConfig& config)
    : config_(config),
      calibrator_(config.calibration),
      speckle_filter_(config.speckle_window_size, config.equivalent_looks),
      unet_model_(),
      reconstructor_(config.tile_size, config.tile_stride),
      slick_analyzer_(config.geo_transform),
      cfar_detector_(config.cfar_guard_cells, config.cfar_training_cells, config.cfar_pfa),
      drift_engine_(1000, 500.0),
      ais_engine_(config.ais_corridor_radius_km, config.ais_max_acceptable_gap_sec) {}

PipelineResult PipelineOrchestrator::process_scene(
    const core::Matrix2D<float>& raw_vv_dn,
    const core::Matrix2D<float>& raw_vh_dn,
    const vector<attribution::AISTransmission>& ais_feed) const 
{
    PipelineResult result;
    result.ais_feed = ais_feed;
    const size_t rows = raw_vv_dn.rows();
    const size_t cols = raw_vv_dn.cols();

    auto t_start_total = high_resolution_clock::now();

    // 1. Radiometric Calibration
    auto t0 = high_resolution_clock::now();
    result.vv_calibrated_db = calibrator_.calibrate_to_db(raw_vv_dn);
    result.vh_calibrated_db = calibrator_.calibrate_to_db(raw_vh_dn);
    auto t1 = high_resolution_clock::now();
    result.telemetry.calibration_ms = duration<double, milli>(t1 - t0).count();
    result.telemetry.mean_vv_db = static_cast<float>(result.vv_calibrated_db.mean());
    result.telemetry.mean_vh_db = static_cast<float>(result.vh_calibrated_db.mean());

    // 2. Adaptive Speckle Suppression
    t0 = high_resolution_clock::now();
    core::Matrix2D<float> vv_filtered = speckle_filter_.filter(result.vv_calibrated_db, config_.speckle_filter_type);
    core::Matrix2D<float> vh_filtered = speckle_filter_.filter(result.vh_calibrated_db, config_.speckle_filter_type);
    t1 = high_resolution_clock::now();
    result.telemetry.despeckle_ms = duration<double, milli>(t1 - t0).count();

    // 3. Polarimetric Normalization
    t0 = high_resolution_clock::now();
    core::Matrix2D<float> norm_vv = physics::Calibrator::normalize_minmax(vv_filtered, -30.0f, 0.0f);
    core::Matrix2D<float> norm_vh = physics::Calibrator::normalize_minmax(vh_filtered, -35.0f, -5.0f);
    t1 = high_resolution_clock::now();
    result.telemetry.polarimetry_ms = duration<double, milli>(t1 - t0).count();

    // 4. Neural Segmentation & Gaussian Sliding Window
    t0 = high_resolution_clock::now();
    result.segmentation_probability = reconstructor_.predict_scene(unet_model_, norm_vv, norm_vh);

    core::Matrix2D<uint8_t> binary_mask(rows, cols, 0);
    const float clean_sea_vv = -11.0f;
    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            float damping_contrast = clean_sea_vv - vv_filtered(r, c);
            if ((result.segmentation_probability(r, c) > config_.segmentation_threshold && vv_filtered(r, c) < -15.0f) || 
               (damping_contrast > config_.damping_contrast_threshold_db && vv_filtered(r, c) < -16.0f)) {
                binary_mask(r, c) = 1;
            }
        }
    }
    t1 = high_resolution_clock::now();
    result.telemetry.segmentation_ms = duration<double, milli>(t1 - t0).count();

    // 5. Polygonization & Feature Extraction (Moore-Neighbor Boundary Tracing)
    t0 = high_resolution_clock::now();
    result.slicks = slick_analyzer_.extract_slicks(
        binary_mask, vv_filtered, vh_filtered, config_.wind_speed_mps, config_.wind_direction_deg);
    t1 = high_resolution_clock::now();
    result.telemetry.polygonization_ms = duration<double, milli>(t1 - t0).count();
    result.telemetry.slicks_detected = result.slicks.size();

    // 6. CA-CFAR Radar Hard-Target Detection (Vessel/Platform Metallic Hulls)
    t0 = high_resolution_clock::now();
    result.radar_targets = cfar_detector_.detect_targets(result.vv_calibrated_db, config_.geo_transform);
    t1 = high_resolution_clock::now();
    result.telemetry.cfar_ms = duration<double, milli>(t1 - t0).count();
    result.telemetry.radar_targets_detected = result.radar_targets.size();

    // 7. Backward-in-Time Lagrangian Drift Dispersion Modeling
    t0 = high_resolution_clock::now();
    constexpr double deg_to_rad = 3.141592653589793 / 180.0;
    drift::MetoceanField metocean;
    metocean.surface_current_u_mps = config_.surface_current_u_mps;
    metocean.surface_current_v_mps = config_.surface_current_v_mps;
    metocean.wind_10m_u_mps = config_.wind_speed_mps * static_cast<float>(sin(config_.wind_direction_deg * deg_to_rad));
    metocean.wind_10m_v_mps = config_.wind_speed_mps * static_cast<float>(cos(config_.wind_direction_deg * deg_to_rad));
    metocean.diffusion_coeff_m2ps = config_.turbulent_diffusion_m2ps;
    metocean.wind_drift_factor = 0.03f;

    for (const auto& slick : result.slicks) {
        // Trace hindcast corridor for actionable slicks
        if (slick.classification != "BIOGENIC_LOOK_ALIKE") {
            auto corridor = drift_engine_.trace_backward(slick.centroid, config_.hindcast_hours, metocean);
            result.drift_corridors.push_back(corridor);
        }
    }
    t1 = high_resolution_clock::now();
    result.telemetry.drift_ms = duration<double, milli>(t1 - t0).count();

    // 8. AIS Maritime Telemetry Polluter Attribution & Dark Vessel Detection
    t0 = high_resolution_clock::now();
    if (!ais_feed.empty()) {
        map<string, attribution::AISAttributionReport> best_candidate_by_mmsi;

        // Correlate against each reconstructed spill origin or observed slick centroid
        vector<geo::GeoPoint> attribution_origins;
        for (const auto& corridor : result.drift_corridors) {
            attribution_origins.push_back(corridor.estimated_origin);
        }
        if (attribution_origins.empty()) {
            for (const auto& slick : result.slicks) {
                attribution_origins.push_back(slick.centroid);
            }
        }

        for (const auto& origin : attribution_origins) {
            auto reports = ais_engine_.correlate_vessels(
                origin, config_.scene_timestamp_epoch_sec, ais_feed, result.radar_targets);

            for (const auto& rep : reports) {
                auto it = best_candidate_by_mmsi.find(rep.candidate_mmsi);
                if (it == best_candidate_by_mmsi.end() || rep.attribution_confidence > it->second.attribution_confidence) {
                    best_candidate_by_mmsi[rep.candidate_mmsi] = rep;
                }
            }
        }

        for (const auto& [mmsi, rep] : best_candidate_by_mmsi) {
            result.suspect_vessels.push_back(rep);

            // Cross-tag matched radar targets with correlated AIS MMSI
            if (rep.matched_radar_hard_target) {
                for (auto& target : result.radar_targets) {
                    if (target.id == rep.radar_target_id) {
                        target.has_correlated_ais = true;
                        target.correlated_mmsi = rep.candidate_mmsi;
                    }
                }
            }
        }
    }
    t1 = high_resolution_clock::now();
    result.telemetry.ais_ms = duration<double, milli>(t1 - t0).count();
    result.telemetry.suspect_vessels_correlated = result.suspect_vessels.size();

    auto t_end_total = high_resolution_clock::now();
    result.telemetry.total_ms = duration<double, milli>(t_end_total - t_start_total).count();

    return result;
}

} // namespace sar::pipeline
