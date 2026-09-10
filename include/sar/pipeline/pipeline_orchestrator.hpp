#pragma once

#include "sar/core/matrix2d.hpp"
#include "sar/pipeline/pipeline_config.hpp"
#include "sar/physics/calibrator.hpp"
#include "sar/physics/speckle_filter.hpp"
#include "sar/nn/unet.hpp"
#include "sar/nn/sliding_window.hpp"
#include "sar/geo/slick_analyzer.hpp"
#include "sar/geo/geojson_writer.hpp"
#include "sar/drift/lagrangian_drift.hpp"
#include "sar/attribution/cfar_detector.hpp"
#include "sar/attribution/ais_engine.hpp"
#include <vector>
#include <string>

namespace sar::pipeline {

struct PipelineTelemetry {
    double calibration_ms = 0.0;
    double despeckle_ms = 0.0;
    double polarimetry_ms = 0.0;
    double segmentation_ms = 0.0;
    double polygonization_ms = 0.0;
    double cfar_ms = 0.0;
    double drift_ms = 0.0;
    double ais_ms = 0.0;
    double total_ms = 0.0;
    float mean_vv_db = 0.0f;
    float mean_vh_db = 0.0f;
    size_t slicks_detected = 0;
    size_t radar_targets_detected = 0;
    size_t suspect_vessels_correlated = 0;
};

struct PipelineResult {
    std::vector<geo::SlickFeature> slicks;
    std::vector<attribution::RadarHardTarget> radar_targets;
    std::vector<drift::ReverseDriftResult> drift_corridors;
    std::vector<attribution::AISAttributionReport> suspect_vessels;
    std::vector<attribution::AISTransmission> ais_feed;
    PipelineTelemetry telemetry;
    core::Matrix2D<float> vv_calibrated_db;
    core::Matrix2D<float> vh_calibrated_db;
    core::Matrix2D<float> segmentation_probability;

    std::string to_geojson() const {
        return geo::GeoJSONWriter::serialize(slicks, drift_corridors, radar_targets, suspect_vessels, ais_feed);
    }

    bool save_geojson(const std::string& filepath) const {
        return geo::GeoJSONWriter::write_file(filepath, slicks, drift_corridors, radar_targets, suspect_vessels, ais_feed);
    }
};

class PipelineOrchestrator {
public:
    explicit PipelineOrchestrator(const PipelineConfig& config = PipelineConfig());

    /**
     * @brief Processes a raw dual-pol SAR scene through the entire end-to-end pipeline,
     * executing calibration, filtering, segmentation, polygonization, CFAR detection,
     * reverse Lagrangian drift dispersion, and AIS maritime polluter attribution.
     */
    PipelineResult process_scene(const core::Matrix2D<float>& raw_vv_dn,
                                 const core::Matrix2D<float>& raw_vh_dn,
                                 const std::vector<attribution::AISTransmission>& ais_feed = {}) const;

private:
    PipelineConfig config_;
    physics::Calibrator calibrator_;
    physics::SpeckleFilter speckle_filter_;
    nn::UNet unet_model_;
    nn::SlidingWindowReconstructor reconstructor_;
    geo::SlickAnalyzer slick_analyzer_;
    attribution::CFARDetector cfar_detector_;
    drift::LagrangianDriftEngine drift_engine_;
    attribution::AISEngine ais_engine_;
};

} // namespace sar::pipeline
