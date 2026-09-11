#pragma once

#include "sar/physics/calibrator.hpp"
#include "sar/physics/speckle_filter.hpp"
#include "sar/geo/geo_types.hpp"
#include <cstdint>
#include <string>

namespace sar::pipeline {

struct PipelineConfig {
    physics::CalibrationMetadata calibration;
    int speckle_window_size = 7;
    float equivalent_looks = 4.4f;
    physics::FilterType speckle_filter_type = physics::FilterType::EnhancedLee;

    size_t tile_size = 256;
    size_t tile_stride = 192;
    float segmentation_threshold = 0.40f;
    float damping_contrast_threshold_db = 5.5f;

    std::string model_weights_path = "data/unet_weights.bin";

    geo::GeoTransform geo_transform;
    float wind_speed_mps = 7.2f;
    float wind_direction_deg = 55.0f;
    float surface_current_u_mps = 0.15f;  // Eastward ocean surface current (m/s)
    float surface_current_v_mps = -0.05f; // Northward ocean surface current (m/s)
    float turbulent_diffusion_m2ps = 2.5f;

    double hindcast_hours = -4.0;         // Reverse Lagrangian drift window (hours)
    int64_t scene_timestamp_epoch_sec = 1700000000;

    // 2D CA-CFAR Hard Target Radar Sensitivity
    int cfar_guard_cells = 3;
    int cfar_training_cells = 8;
    float cfar_pfa = 1e-5f;

    // AIS Vessel Attribution Parameters
    double ais_corridor_radius_km = 15.0;
    int64_t ais_max_acceptable_gap_sec = 14400; // 4 hours transponder silence threshold
};

} // namespace sar::pipeline
