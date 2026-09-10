#pragma once

#include "sar/core/tensor3d.hpp"
#include <vector>

namespace sar::nn {

class Layers {
public:
    static core::Tensor3D<float> conv2d(const core::Tensor3D<float>& in,
                                        const std::vector<float>& weights,
                                        const std::vector<float>& bias,
                                        size_t k_size = 3);

    static void relu_inplace(core::Tensor3D<float>& t) noexcept;

    static core::Tensor3D<float> max_pool2x2(const core::Tensor3D<float>& in);

    static core::Tensor3D<float> upsample2x2_bilinear(const core::Tensor3D<float>& in, 
                                                      size_t target_h, 
                                                      size_t target_w);

    static core::Tensor3D<float> concat_channels(const core::Tensor3D<float>& t1, 
                                                 const core::Tensor3D<float>& t2);
};

} // namespace sar::nn
