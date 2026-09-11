#pragma once

#include "sar/core/matrix2d.hpp"
#include "sar/core/tensor3d.hpp"
#include <vector>
#include <string>

namespace sar::nn {

class UNet {
public:
    explicit UNet(const std::string& weights_path = "data/unet_weights.bin");

    /**
     * @brief Performs forward pass on a 2-channel tile (VV, VH) of shape [2, H, W].
     * @return core::Matrix2D<float> Probability map [H, W] with values in [0.0, 1.0].
     */
    core::Matrix2D<float> forward_tile(const core::Tensor3D<float>& input_tensor) const;

    bool load_weights(const std::string& weights_path);
    bool is_weights_loaded() const noexcept { return weights_loaded_; }
    const std::string& weights_provenance() const noexcept { return weights_provenance_; }

private:
    void init_weights();

    bool weights_loaded_ = false;
    std::string weights_provenance_ = "Calibrated Synthetic Normal Distribution (Seed 42)";

    std::vector<float> conv1_w_, conv1_b_;
    std::vector<float> conv2_w_, conv2_b_;
    std::vector<float> conv3_w_, conv3_b_;
    std::vector<float> conv4_w_, conv4_b_;
    std::vector<float> conv5_w_, conv5_b_;
    std::vector<float> conv_out_w_, conv_out_b_;
};

} // namespace sar::nn
