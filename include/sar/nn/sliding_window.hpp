#pragma once

#include "sar/core/matrix2d.hpp"
#include "sar/nn/unet.hpp"

namespace sar::nn {

class SlidingWindowReconstructor {
public:
    explicit SlidingWindowReconstructor(size_t tile_size = 256, size_t stride = 192);

    /**
     * @brief Performs sliding window neural inference across a large SAR scene with Gaussian blending.
     */
    core::Matrix2D<float> predict_scene(const UNet& model,
                                        const core::Matrix2D<float>& norm_vv,
                                        const core::Matrix2D<float>& norm_vh) const;

private:
    size_t tile_size_;
    size_t stride_;
    core::Matrix2D<float> gaussian_weight_;
};

} // namespace sar::nn
