#pragma once

#include <vector>
#include <cstdint>
#include <cmath>
#include <string>
#include <iostream>

namespace aegis::sar {

struct GeoPoint {
    double lon = 0.0;
    double lat = 0.0;
};

struct DetectedSlick {
    uint32_t id = 0;
    std::vector<GeoPoint> boundary_wgs84;
    double area_m2 = 0.0;
    double perimeter_m = 0.0;
    float mean_damping_db = 0.0f;
    GeoPoint centroid;
    std::string severity = "MEDIUM";
};

struct HardTarget {
    uint32_t id = 0;
    uint32_t r = 0;
    uint32_t c = 0;
    float peak_rcs_db = 0.0f;
    float snr_db = 0.0f;
    GeoPoint location;
    bool matched_ais = false;
    std::string correlated_mmsi = "";
};

class O1CFARDetector {
public:
    O1CFARDetector(uint32_t training_win = 35, uint32_t guard_win = 11,
                   float alpha_oil = 3.0f, float beta_vessel = 7.0f);

    /**
     * @brief Build 64-bit dual-moment integral images in a single pass.
     */
    void build_integral_images(const std::vector<float>& tile_db, uint32_t width, uint32_t height);

    /**
     * @brief Execute constant-time dual-threshold CA-CFAR evaluation across the raster.
     */
    void detect_anomalies(const std::vector<float>& tile_db, uint32_t width, uint32_t height,
                         double origin_lon, double origin_lat, double pixel_res_m,
                         std::vector<DetectedSlick>& out_slicks,
                         std::vector<HardTarget>& out_targets);

    /**
     * @brief 8-connected Moore-Neighbor contour tracing algorithm.
     */
    static std::vector<std::pair<int, int>> trace_contour(const std::vector<uint8_t>& binary_mask,
                                                          uint32_t width, uint32_t height,
                                                          int start_r, int start_c);

    /**
     * @brief Compute polygon area via Gauss-Green Shoelace Theorem.
     */
    static double compute_shoelace_area(const std::vector<GeoPoint>& polygon, double center_lat);

    /**
     * @brief Compute polygon perimeter via Haversine segment summation.
     */
    static double compute_haversine_perimeter(const std::vector<GeoPoint>& polygon);

private:
    uint32_t training_win_;
    uint32_t guard_win_;
    float alpha_oil_;
    float beta_vessel_;

    std::vector<double> S1_; // First moment integral image (sum of I)
    std::vector<double> S2_; // Second moment integral image (sum of I^2)
    uint32_t img_w_ = 0;
    uint32_t img_h_ = 0;

    double query_sum(const std::vector<double>& S, int r1, int c1, int r2, int c2) const;
};

} // namespace aegis::sar
