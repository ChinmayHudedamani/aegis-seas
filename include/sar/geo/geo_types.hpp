#pragma once

#include <vector>
#include <string>
#include <cstddef>

namespace sar::geo {

struct GeoPoint {
    double lon = 0.0;
    double lat = 0.0;
};

struct GeoTransform {
    double origin_lon = -88.35;      // Gulf of Mexico scene center
    double origin_lat = 28.75;
    double pixel_size_meters = 10.0; // Sentinel-1 GRD 10m
};

struct SlickFeature {
    size_t id = 0;
    std::vector<GeoPoint> boundary;
    GeoPoint centroid;
    double area_km2 = 0.0;
    double perimeter_km = 0.0;
    double aspect_ratio = 1.0;
    double orientation_deg = 0.0;
    float confidence_score = 0.0f;
    std::string severity;       // "LOW", "MEDIUM", "CRITICAL"
    std::string classification; // "MINERAL_OIL_SPILL", "BIOGENIC_LOOK_ALIKE", "VESSEL_DISCHARGE"
    GeoPoint drift_vector_24h;  // Projected 24-hour drift centroid
    float mean_damping_db = 0.0f;
    float mean_contrast_db = 0.0f;
};

} // namespace sar::geo
