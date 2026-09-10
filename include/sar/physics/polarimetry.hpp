#pragma once

#include "sar/core/matrix2d.hpp"
#include <cstdint>

namespace sar::physics {

struct SlickPolarimetricProfile {
    float mean_vv_db;
    float mean_vh_db;
    float clutter_vv_db;
    float contrast_to_clutter_ratio_db; // CCR in dB
    float dual_pol_ratio_db;           // VV(dB) - VH(dB)
    float slick_probability;           // Confidence score [0.0, 1.0]
    bool is_look_alike;                // Flagged as low-wind or biogenic film
};

class PolarimetricAnalyzer {
public:
    /**
     * @brief Computes the Dual-Polarization Ratio map: DPR_dB = VV_dB - VH_dB.
     */
    static core::Matrix2D<float> compute_dpr_map(const core::Matrix2D<float>& vv_db, 
                                                 const core::Matrix2D<float>& vh_db);

    /**
     * @brief Evaluates a candidate binary mask against background ambient clutter.
     */
    static SlickPolarimetricProfile analyze_candidate(
        const core::Matrix2D<float>& vv_db,
        const core::Matrix2D<float>& vh_db,
        const core::Matrix2D<uint8_t>& candidate_mask);
};

} // namespace sar::physics
