#include "sar/pipeline/pipeline_orchestrator.hpp"
#include "sar/simulation/synthetic_scene.hpp"
#include "sar/io/ais_parser.hpp"

#include <iostream>
#include <iomanip>
#include <string>

using namespace std;

int main(int argc, char* argv[]) {
    cout << "===================================================================\n";
    cout << "  AEGIS-SEAS: AUTONOMOUS SATELLITE RADAR OIL SPILL DETECTION (C++20)\n";
    cout << "  Multi-Layer Attribution Engine | SIH 2026 Sovereign Architecture  \n";
    cout << "===================================================================\n\n";

    constexpr size_t ROWS = 512;
    constexpr size_t COLS = 512;

    // 1. Simulation & Scene Ingestion
    cout << "[Ingestion] Generating Synthetic Dual-Pol SAR Scene (" << ROWS << "x" << COLS << ")...\n";
    sar::core::Matrix2D<float> raw_vv;
    sar::core::Matrix2D<float> raw_vh;
    vector<sar::simulation::GroundTruthSlick> ground_truth;
    sar::simulation::SyntheticSARScene::generate_scene(ROWS, COLS, raw_vv, raw_vh, ground_truth);
    cout << "  -> Synthetic scene synthesized with " << ground_truth.size() << " ground truth targets.\n";

    // 2. AIS Telemetry Ingestion
    string ais_file = "data/sample_ais_traffic.csv";
    for (int i = 1; i < argc - 1; ++i) {
        if (string(argv[i]) == "--ais") {
            ais_file = argv[i + 1];
        }
    }
    cout << "[Ingestion] Ingesting Maritime AIS Telemetry Feed ('" << ais_file << "')...\n";
    auto ais_feed = sar::io::AISParser::parse_csv_file(ais_file);
    cout << "  -> Loaded " << ais_feed.size() << " maritime AIS position records.\n\n";

    // 3. Configure Unified Pipeline
    sar::pipeline::PipelineConfig config;
    config.calibration.calibration_constant = 100.0f;
    config.speckle_window_size = 7;
    config.equivalent_looks = 4.4f;
    config.segmentation_threshold = 0.40f;
    config.damping_contrast_threshold_db = 5.5f;
    config.wind_speed_mps = 7.2f;
    config.wind_direction_deg = 55.0f;
    config.surface_current_u_mps = 0.18f;
    config.surface_current_v_mps = -0.06f;
    config.hindcast_hours = -4.0; // 4-hour reverse drift
    config.scene_timestamp_epoch_sec = 1700000000;
    config.geo_transform.origin_lon = -88.35;
    config.geo_transform.origin_lat = 28.75;
    config.geo_transform.pixel_size_meters = 10.0;

    sar::pipeline::PipelineOrchestrator orchestrator(config);

    // 4. Process Scene
    cout << "[Execution] Executing Modular Pipeline Orchestrator...\n";
    sar::pipeline::PipelineResult result = orchestrator.process_scene(raw_vv, raw_vh, ais_feed);

    // 5. Output Telemetry Metrics
    cout << "\n===================================================================\n";
    cout << "                        TELEMETRY METRICS                          \n";
    cout << "===================================================================\n";
    cout << " [1] Radiometric Calibration : " << fixed << setprecision(2) << result.telemetry.calibration_ms << " ms\n"
         << "     - Mean VV: " << result.telemetry.mean_vv_db << " dB | Mean VH: " << result.telemetry.mean_vh_db << " dB\n"
         << " [2] Enhanced Lee Despeckle  : " << result.telemetry.despeckle_ms << " ms\n"
         << " [3] Polarimetric Scaling    : " << result.telemetry.polarimetry_ms << " ms\n"
         << " [4] Neural U-Net Inference  : " << result.telemetry.segmentation_ms << " ms\n"
         << " [5] Moore-Neighbor Tracing  : " << result.telemetry.polygonization_ms << " ms (" 
         << result.telemetry.slicks_detected << " Slicks)\n"
         << " [6] 2D CA-CFAR Target Detect: " << result.telemetry.cfar_ms << " ms (" 
         << result.telemetry.radar_targets_detected << " Radar Hard-Targets)\n"
         << " [7] Lagrangian Hindcast     : " << result.telemetry.drift_ms << " ms (" 
         << result.drift_corridors.size() << " Dispersion Corridors)\n"
         << " [8] AIS Polluter Attribution: " << result.telemetry.ais_ms << " ms (" 
         << result.telemetry.suspect_vessels_correlated << " Candidates Correlated)\n"
         << "-------------------------------------------------------------------\n"
         << " Total End-to-End Latency    : " << result.telemetry.total_ms << " ms\n"
         << " Processing Throughput       : " << setprecision(1) << (ROWS * COLS / (result.telemetry.total_ms / 1000.0) / 1e6) << " Megapixels/sec\n";

    // 6. Output Detected Oil Spill Dossier
    cout << "\n===================================================================\n";
    cout << "                     DETECTED OIL SPILL DOSSIER                    \n";
    cout << "===================================================================\n";
    for (const auto& slick : result.slicks) {
        cout << " [Slick #" << slick.id << "]\n"
             << "  - Classification : " << slick.classification << "\n"
             << "  - Severity Level : " << slick.severity << "\n"
             << "  - Confidence     : " << fixed << setprecision(1) << (slick.confidence_score * 100.0f) << " %\n"
             << "  - Surface Area   : " << setprecision(3) << slick.area_km2 << " km^2 (" << slick.boundary.size() << " polygon vertices)\n"
             << "  - Aspect Ratio   : " << setprecision(2) << slick.aspect_ratio << " (Orientation: " << slick.orientation_deg << " deg)\n"
             << "  - Marangoni Damp : " << setprecision(1) << slick.mean_damping_db << " dB contrast\n"
             << "  - Centroid (WGS84): Lat " << slick.centroid.lat << " deg, Lon " << slick.centroid.lon << " deg\n\n";
    }

    // 7. Output Radar Hard Targets
    cout << "===================================================================\n";
    cout << "                  CA-CFAR RADAR TARGET CONTACTS                    \n";
    cout << "===================================================================\n";
    for (const auto& target : result.radar_targets) {
        cout << " [Radar Target #" << target.id << "]\n"
             << "  - Position (WGS84): Lat " << fixed << setprecision(4) << target.location.lat 
             << " deg, Lon " << target.location.lon << " deg (pixel " << target.pixel_r << ", " << target.pixel_c << ")\n"
             << "  - Peak RCS / SNR  : " << setprecision(1) << target.peak_rcs_db << " dB / " << target.snr_db << " dB\n"
             << "  - Correlated AIS  : " << (target.has_correlated_ais ? ("MMSI " + target.correlated_mmsi) : "NONE (POTENTIAL DARK VESSEL)") << "\n\n";
    }

    // 8. Output Suspect Polluter Attribution Dossier
    cout << "===================================================================\n";
    cout << "                 POLLUTER ATTRIBUTION DOSSIER                      \n";
    cout << "===================================================================\n";
    for (const auto& vessel : result.suspect_vessels) {
        cout << " [Candidate MMSI: " << vessel.candidate_mmsi << " - " << vessel.vessel_name << " (" << vessel.vessel_type << ")]\n"
             << "  - Attribution Score : " << fixed << setprecision(1) << (vessel.attribution_confidence * 100.0f) << " %\n"
             << "  - Min Distance Origin: " << setprecision(2) << vessel.min_distance_to_origin_km << " km\n"
             << "  - Radar Verification : " << (vessel.matched_radar_hard_target ? ("Verified by CFAR Target #" + to_string(vessel.radar_target_id)) : "Unmatched") << "\n"
             << "  - Threat Flags       : ";
        if (vessel.is_dark_vessel) cout << "[DARK VESSEL ALERT: " << setprecision(1) << vessel.dark_gap_hours << "h silence] ";
        if (vessel.is_spoofed_teleportation) cout << "[GPS SPOOFING DETECTED] ";
        if (!vessel.is_dark_vessel && !vessel.is_spoofed_teleportation) cout << "[Standard Track]";
        cout << "\n  - Status Summary     : " << vessel.status_summary << "\n\n";
    }

    // 9. Save Multi-Layer GeoJSON
    if (result.save_geojson("web/detected_spills.geojson")) {
        cout << " [✓] Exported 4-layer tactical GeoJSON to: 'web/detected_spills.geojson'\n";
    }
    if (result.save_geojson("detected_spills.geojson")) {
        cout << " [✓] Exported 4-layer tactical GeoJSON to: 'detected_spills.geojson'\n";
    }

    cout << "===================================================================\n";
    return 0;
}
