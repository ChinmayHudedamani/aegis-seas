#include "sar/physics/polarimetry.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>

using namespace std;

namespace sar::physics {

core::Matrix2D<float> PolarimetricAnalyzer::compute_dpr_map(const core::Matrix2D<float>& vv_db, 
                                                            const core::Matrix2D<float>& vh_db) {
    if (vv_db.rows() != vh_db.rows() || vv_db.cols() != vh_db.cols()) {
        throw invalid_argument("VV and VH dimensions must match");
    }

    core::Matrix2D<float> dpr(vv_db.rows(), vv_db.cols());
    const size_t n = vv_db.size();
    const float* p_vv = vv_db.data();
    const float* p_vh = vh_db.data();
    float* p_dpr = dpr.data();

    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < n; ++i) {
        p_dpr[i] = p_vv[i] - p_vh[i];
    }

    return dpr;
}

SlickPolarimetricProfile PolarimetricAnalyzer::analyze_candidate(
    const core::Matrix2D<float>& vv_db,
    const core::Matrix2D<float>& vh_db,
    const core::Matrix2D<uint8_t>& candidate_mask) 
{
    double slick_vv_sum = 0.0;
    double slick_vh_sum = 0.0;
    size_t slick_count = 0;

    double clutter_vv_sum = 0.0;
    double clutter_vh_sum = 0.0;
    size_t clutter_count = 0;

    const size_t rows = vv_db.rows();
    const size_t cols = vv_db.cols();

    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            if (candidate_mask(r, c) > 0) {
                slick_vv_sum += vv_db(r, c);
                slick_vh_sum += vh_db(r, c);
                ++slick_count;
            } else {
                clutter_vv_sum += vv_db(r, c);
                clutter_vh_sum += vh_db(r, c);
                ++clutter_count;
            }
        }
    }

    SlickPolarimetricProfile profile{};
    if (slick_count == 0 || clutter_count == 0) {
        return profile;
    }

    profile.mean_vv_db = static_cast<float>(slick_vv_sum / slick_count);
    profile.mean_vh_db = static_cast<float>(slick_vh_sum / slick_count);
    profile.clutter_vv_db = static_cast<float>(clutter_vv_sum / clutter_count);

    // CCR = Clutter_dB - Slick_dB
    profile.contrast_to_clutter_ratio_db = profile.clutter_vv_db - profile.mean_vv_db;
    profile.dual_pol_ratio_db = profile.mean_vv_db - profile.mean_vh_db;

    if (profile.clutter_vv_db < -21.0f) {
        profile.is_look_alike = true;
        profile.slick_probability = 0.20f;
    } else if (profile.contrast_to_clutter_ratio_db < 3.5f) {
        profile.is_look_alike = true;
        profile.slick_probability = 0.35f;
    } else if (profile.contrast_to_clutter_ratio_db >= 6.0f) {
        profile.is_look_alike = false;
        float score = 1.0f / (1.0f + exp(-0.8f * (profile.contrast_to_clutter_ratio_db - 5.5f)));
        profile.slick_probability = clamp(score, 0.5f, 0.99f);
    } else {
        profile.is_look_alike = false;
        profile.slick_probability = 0.55f;
    }

    return profile;
}

} // namespace sar::physics
