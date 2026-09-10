#pragma once

#include "sar/core/matrix2d.hpp"
#include "sar/geo/geo_types.hpp"
#include "sar/physics/polarimetry.hpp"
#include <vector>
#include <cstdint>
#include <utility>

namespace sar::geo {

class SlickAnalyzer {
public:
    explicit SlickAnalyzer(const GeoTransform& transform = GeoTransform());

    /**
     * @brief Extracts vector features, aspect ratio, WGS84 area, and Fay drift vectors from candidate mask.
     */
    std::vector<SlickFeature> extract_slicks(const core::Matrix2D<uint8_t>& mask,
                                             const core::Matrix2D<float>& vv_db,
                                             const core::Matrix2D<float>& vh_db,
                                             float wind_speed_mps = 6.5f,
                                             float wind_dir_deg = 45.0f) const;

private:
    GeoPoint pixel_to_geo(double r, double c) const;

    SlickFeature compute_feature_metrics(
        size_t id,
        const std::vector<std::pair<int, int>>& pixels,
        const core::Matrix2D<uint8_t>& comp_mask,
        const physics::SlickPolarimetricProfile& profile,
        float wind_speed_mps,
        float wind_dir_deg) const;

    std::vector<GeoPoint> trace_moore_contour(
        const core::Matrix2D<uint8_t>& comp_mask,
        int min_r, int max_r, int min_c, int max_c) const;

    static std::vector<std::pair<double, double>> simplify_rdp(
        const std::vector<std::pair<double, double>>& points,
        double epsilon);

    GeoTransform transform_;
};

} // namespace sar::geo
