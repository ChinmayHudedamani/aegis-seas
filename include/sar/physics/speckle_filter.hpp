#pragma once

#include "sar/core/matrix2d.hpp"

namespace sar::physics {

enum class FilterType {
    Lee,
    EnhancedLee
};

class SpeckleFilter {
public:
    explicit SpeckleFilter(int window_size = 7, float looks = 4.4f);

    /**
     * @brief Applies speckle suppression on a 2D SAR intensity / dB matrix.
     */
    core::Matrix2D<float> filter(const core::Matrix2D<float>& input, 
                                 FilterType type = FilterType::EnhancedLee) const;

private:
    int window_size_;
    float looks_;
    float cu_;    // Noise coefficient of variation (1 / sqrt(looks))
    float cmax_;  // Upper threshold (sqrt(1 + 2/looks))
};

} // namespace sar::physics
