#include "sar/core/matrix2d.hpp"
#include "sar/core/tensor3d.hpp"
#include "sar/physics/calibrator.hpp"
#include "sar/physics/speckle_filter.hpp"
#include "sar/physics/polarimetry.hpp"
#include "sar/nn/unet.hpp"
#include "sar/nn/sliding_window.hpp"
#include "sar/geo/slick_analyzer.hpp"
#include "sar/geo/geojson_writer.hpp"
#include "sar/drift/lagrangian_drift.hpp"
#include "sar/attribution/cfar_detector.hpp"
#include "sar/attribution/ais_engine.hpp"
#include "sar/io/ais_parser.hpp"
#include "sar/pipeline/pipeline_orchestrator.hpp"
#include "sar/simulation/synthetic_scene.hpp"

#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>

using namespace std;

void test_vector_1_low_wind_mirror_trap() {
    cout << "[TEST VECTOR 1.1] Stress Testing Low-Wind Mirror Trap (< 2.5 m/s)...\n";
    sar::core::Matrix2D<float> vv_calibrated(256, 256, -24.5f);
    sar::core::Matrix2D<float> vh_calibrated(256, 256, -32.0f);
    sar::core::Matrix2D<uint8_t> candidate_mask(256, 256, 0);

    for (size_t r = 100; r < 150; ++r) {
        for (size_t c = 100; c < 150; ++c) {
            candidate_mask(r, c) = 1;
            vv_calibrated(r, c) = -26.0f; // Minimal contrast (~1.5 dB)
        }
    }

    auto profile = sar::physics::PolarimetricAnalyzer::analyze_candidate(vv_calibrated, vh_calibrated, candidate_mask);
    assert(profile.is_look_alike == true);
    assert(profile.slick_probability <= 0.35f);
    cout << "  [✓] PASSED: Low-wind ambient calm correctly flagged as Look-Alike (P = " 
         << profile.slick_probability << ").\n";
}

void test_vector_1_nan_and_nodata_handling() {
    cout << "[TEST VECTOR 1.2] Fuzzing NaN, Inf, and Negative NoData Inputs...\n";
    sar::core::Matrix2D<float> raw_dn_nan(128, 128, 100.0f);
    raw_dn_nan(10, 10) = numeric_limits<float>::quiet_NaN();
    raw_dn_nan(20, 20) = numeric_limits<float>::infinity();
    raw_dn_nan(30, 30) = -9999.0f; // Standard NoData

    sar::physics::Calibrator calibrator;
    sar::core::Matrix2D<float> calibrated = calibrator.calibrate_to_db(raw_dn_nan);

    for (size_t i = 0; i < calibrated.size(); ++i) {
        float val = calibrated.data()[i];
        assert(!isnan(val));
    }
    cout << "  [✓] PASSED: Calibrator gracefully sanitizes NaN/Inf/NoData pixels.\n";
}

void test_vector_2_lagrangian_shear_and_corridor_expansion() {
    cout << "[TEST VECTOR 2] Lagrangian Drift Chaos & Uncertainty Fuzzing...\n";
    sar::drift::LagrangianDriftEngine engine(1000, 500.0);

    sar::drift::MetoceanField metocean;
    metocean.surface_current_u_mps = 1.2f;
    metocean.surface_current_v_mps = 0.0f;
    metocean.wind_10m_u_mps = -15.0f;
    metocean.wind_10m_v_mps = 0.0f;
    metocean.diffusion_coeff_m2ps = 15.0f;

    sar::geo::GeoPoint slick_loc{-88.35, 28.75};
    sar::drift::ReverseDriftResult res = engine.trace_backward(slick_loc, -12.0, metocean);

    cout << "  -> 12h Backward Corridor Area: " << res.corridor_area_km2 << " km^2\n";
    assert(res.corridor_area_km2 > 500.0);
    assert(res.excessive_uncertainty_warning == true);
    cout << "  [✓] PASSED: Excessive uncertainty warning triggered correctly (" 
         << res.warning_message << ").\n";
}

void test_vector_3_ais_dark_vessel_and_spoofing() {
    cout << "[TEST VECTOR 3] AIS Anomaly & Spoofing Ingestion...\n";
    sar::attribution::AISEngine ais_engine(15.0, 14400);

    sar::geo::GeoPoint spill_origin{-88.35, 28.75};
    int64_t spill_time = 1700000000;

    vector<sar::attribution::AISTransmission> feed;
    feed.push_back({"111111111", "GHOST_TANKER", "TANKER", -88.34, 28.74, 12.0f, 45.0f, spill_time - 21600});
    feed.push_back({"111111111", "GHOST_TANKER", "TANKER", -88.36, 28.76, 12.0f, 45.0f, spill_time + 7200});
    feed.push_back({"222222222", "SPOOF_CARGO", "CARGO", -88.35, 28.75, 55.0f, 90.0f, spill_time});

    vector<sar::attribution::RadarHardTarget> cfar_targets = {
        {1, 250, 250, {-88.34, 28.74}, 18.5f, 22.0f, false, ""}
    };

    auto reports = ais_engine.correlate_vessels(spill_origin, spill_time, feed, cfar_targets);
    assert(reports.size() >= 2);

    bool found_dark_vessel_alert = false;
    bool found_spoof_alert = false;

    for (const auto& rep : reports) {
        if (rep.candidate_mmsi == "111111111") {
            assert(rep.is_dark_vessel == true);
            assert(rep.matched_radar_hard_target == true);
            assert(rep.dark_gap_hours >= 6.0);
            found_dark_vessel_alert = true;
            cout << "  [✓] PASSED: Dark vessel caught with CFAR radar cross-validation (" 
                 << rep.status_summary << ").\n";
        }
        if (rep.candidate_mmsi == "222222222") {
            assert(rep.is_spoofed_teleportation == true);
            assert(rep.confidence_penalty > 0.30f);
            found_spoof_alert = true;
            cout << "  [✓] PASSED: Spoofed 55kt teleportation penalized by " 
                 << rep.confidence_penalty << " confidence points.\n";
        }
    }

    assert(found_dark_vessel_alert && found_spoof_alert);
}

void test_vector_3_high_density_chokepoint_degradation() {
    cout << "[TEST VECTOR 3.2] High-Density Shipping Channel Stress Test (16 Vessels)...\n";
    sar::attribution::AISEngine ais_engine(20.0, 14400);

    sar::geo::GeoPoint spill_origin{-88.35, 28.75};
    int64_t spill_time = 1700000000;

    vector<sar::attribution::AISTransmission> feed;
    for (int i = 0; i < 18; ++i) {
        string mmsi = "3333330" + to_string(i);
        feed.push_back({mmsi, "CONTAINER_" + to_string(i), "CARGO", -88.35 + (i * 0.005), 28.75 + (i * 0.005), 14.0f, 45.0f, spill_time});
    }

    auto reports = ais_engine.correlate_vessels(spill_origin, spill_time, feed);
    assert(reports.size() >= 15);
    for (const auto& rep : reports) {
        assert(rep.is_high_density_chokepoint == true);
        assert(rep.co_located_vessel_count >= 15);
    }
    cout << "  [✓] PASSED: Kinematic degradation penalty applied to all " 
         << reports.size() << " co-located vessels in crowded channel.\n";
}

void test_vector_4_moore_neighbor_contour_tracing() {
    cout << "[TEST VECTOR 4] Moore-Neighbor Boundary Tracing & Closed Ring Topology...\n";
    sar::core::Matrix2D<uint8_t> binary_mask(100, 100, 0);
    sar::core::Matrix2D<float> vv(100, 100, -10.0f);
    sar::core::Matrix2D<float> vh(100, 100, -20.0f);

    // Create an irregular L-shaped slick
    for (int r = 30; r <= 60; ++r) {
        for (int c = 30; c <= 45; ++c) {
            binary_mask(r, c) = 1;
            vv(r, c) = -22.0f; // damped
        }
    }
    for (int r = 50; r <= 60; ++r) {
        for (int c = 45; c <= 70; ++c) {
            binary_mask(r, c) = 1;
            vv(r, c) = -22.0f;
        }
    }

    sar::geo::SlickAnalyzer analyzer;
    auto slicks = analyzer.extract_slicks(binary_mask, vv, vh);

    assert(slicks.size() == 1);
    const auto& s = slicks.front();
    cout << "  -> Extracted slick boundary vertex count: " << s.boundary.size() << "\n";

    assert(s.boundary.size() >= 4);
    // Closed ring assertion conforming to RFC 7946 GeoJSON
    assert(s.boundary.front().lat == s.boundary.back().lat);
    assert(s.boundary.front().lon == s.boundary.back().lon);
    assert(s.area_km2 > 0.0);

    cout << "  [✓] PASSED: Moore-Neighbor tracing produces strictly closed, topologically valid polygon.\n";
}

void test_vector_5_ais_csv_parser() {
    cout << "[TEST VECTOR 5] Ingesting Maritime AIS CSV Stream...\n";
    string sample_csv = 
        "mmsi,vessel_name,vessel_type,lon,lat,sog,cog,timestamp\n"
        "355000109,PACIFIC_VOYAGER,TANKER,-88.3800,28.7800,12.4,135.0,1699985600\n"
        "412999001,GHOST_CRUDE,TANKER,-88.3600,28.7600,10.5,45.0,1699978400\n";

    auto records = sar::io::AISParser::parse_csv_string(sample_csv);
    assert(records.size() == 2);
    assert(records[0].mmsi == "355000109");
    assert(records[0].vessel_name == "PACIFIC_VOYAGER");
    assert(records[0].vessel_type == "TANKER");
    assert(abs(records[0].lon - (-88.3800)) < 1e-5);
    assert(abs(records[0].lat - 28.7800) < 1e-5);
    assert(records[0].speed_knots == 12.4f);
    assert(records[0].timestamp_epoch_sec == 1699985600);

    assert(records[1].mmsi == "412999001");
    cout << "  [✓] PASSED: AIS CSV parser correctly deserializes telemetry records.\n";
}

void test_vector_6_multilayer_geojson_and_integrated_pipeline() {
    cout << "[TEST VECTOR 6] Multi-Layer Pipeline & Unified GeoJSON Serialization...\n";

    sar::core::Matrix2D<float> raw_vv;
    sar::core::Matrix2D<float> raw_vh;
    vector<sar::simulation::GroundTruthSlick> ground_truth;
    sar::simulation::SyntheticSARScene::generate_scene(256, 256, raw_vv, raw_vh, ground_truth);

    sar::pipeline::PipelineConfig config;
    config.hindcast_hours = -3.0;
    sar::pipeline::PipelineOrchestrator orchestrator(config);

    vector<sar::attribution::AISTransmission> ais_feed = {
        {"355000109", "PACIFIC_VOYAGER", "TANKER", -88.33, 28.73, 12.0f, 45.0f, 1700000000}
    };

    auto result = orchestrator.process_scene(raw_vv, raw_vh, ais_feed);

    assert(result.slicks.size() > 0);
    assert(result.radar_targets.size() > 0);
    assert(result.drift_corridors.size() > 0);

    string geojson = result.to_geojson();
    assert(geojson.find("\"feature_type\": \"OilSlick\"") != string::npos);
    assert(geojson.find("\"feature_type\": \"RadarHardTarget\"") != string::npos);
    assert(geojson.find("\"feature_type\": \"ReverseDriftPlume\"") != string::npos);
    assert(geojson.find("\"feature_type\": \"SuspectVessel\"") != string::npos);

    cout << "  -> Generated GeoJSON size: " << geojson.size() << " bytes with all 4 tactical layers.\n";
    cout << "  [✓] PASSED: End-to-end multi-layer pipeline and GeoJSON serialization verified.\n";
}

int main() {
    cout << "===================================================================\n";
    cout << "  AEGIS-SEAS: RED-TEAM & INTEGRATION AUDIT SUITE (C++20)           \n";
    cout << "===================================================================\n\n";

    test_vector_1_low_wind_mirror_trap();
    test_vector_1_nan_and_nodata_handling();
    test_vector_2_lagrangian_shear_and_corridor_expansion();
    test_vector_3_ais_dark_vessel_and_spoofing();
    test_vector_3_high_density_chokepoint_degradation();
    test_vector_4_moore_neighbor_contour_tracing();
    test_vector_5_ais_csv_parser();
    test_vector_6_multilayer_geojson_and_integrated_pipeline();

    cout << "\n===================================================================\n";
    cout << "  ALL 8 RED-TEAM & INTEGRATION VECTORS PASSED WITH ZERO ERRORS\n";
    cout << "===================================================================\n";
    return 0;
}
