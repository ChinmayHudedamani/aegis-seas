#pragma once

#include "sar/core/matrix2d.hpp"
#include "sar/geo/geo_types.hpp"
#include <vector>

namespace sar::attribution {

struct RadarHardTarget {
    size_t id;
    int pixel_r;
    int pixel_c;
    geo::GeoPoint location;
    float peak_rcs_db;
    float snr_db;
    bool has_correlated_ais = false;
    std::string correlated_mmsi;
};

/**
 * @brief 2D Cell-Averaging Constant False Alarm Rate (CA-CFAR) Radar Target Detector.
 * Identifies physical metal hulls directly from radar backscatter to catch dark vessels.
 */
class CFARDetector {
public:
    explicit CFARDetector(int guard_cells = 3, int training_cells = 8, float pfa = 1e-5f);

    /**
     * @brief Detects radar point targets from calibrated backscatter.
     */
    std::vector<RadarHardTarget> detect_targets(
        const core::Matrix2D<float>& vv_db,
        const geo::GeoTransform& transform = geo::GeoTransform()) const;

private:
    int guard_cells_;
    int training_cells_;
    float pfa_;
    float threshold_factor_;
};

} // namespace sar::attribution
