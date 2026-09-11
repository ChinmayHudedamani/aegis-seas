#include "sar/nn/unet.hpp"
#include "sar/nn/layers.hpp"
#include <cmath>
#include <random>
#include <fstream>
#include <iostream>

using namespace std;

namespace sar::nn {

UNet::UNet(const std::string& weights_path) {
    if (!load_weights(weights_path)) {
        init_weights();
    }
}

bool UNet::load_weights(const std::string& weights_path) {
    ifstream in(weights_path, ios::binary | ios::ate);
    if (!in.is_open()) {
        return false;
    }

    streamsize size = in.tellg();
    in.seekg(0, ios::beg);

    constexpr size_t EXPECTED_FLOATS = 
        (2 * 8 * 3 * 3 + 8) +      // conv1: 144 + 8 = 152
        (8 * 8 * 3 * 3 + 8) +      // conv2: 576 + 8 = 584
        (8 * 16 * 3 * 3 + 16) +    // conv3: 1152 + 16 = 1168
        (16 * 16 * 3 * 3 + 16) +   // conv4: 2304 + 16 = 2320
        (24 * 8 * 3 * 3 + 8) +     // conv5: 1728 + 8 = 1736
        (8 * 1 * 1 * 1 + 1);       // conv_out: 8 + 1 = 9
                                   // Total = 5969 floats = 23876 bytes

    if (static_cast<size_t>(size) != EXPECTED_FLOATS * sizeof(float)) {
        cerr << "[UNet] Warning: Weight file size mismatch for '" << weights_path 
             << "'. Expected " << EXPECTED_FLOATS * sizeof(float) << " bytes, got " 
             << size << " bytes.\n";
        return false;
    }

    auto read_layer = [&in](vector<float>& w, vector<float>& b,
                            size_t in_c, size_t out_c, size_t k) {
        w.resize(out_c * in_c * k * k);
        b.resize(out_c);
        in.read(reinterpret_cast<char*>(w.data()), w.size() * sizeof(float));
        in.read(reinterpret_cast<char*>(b.data()), b.size() * sizeof(float));
    };

    read_layer(conv1_w_, conv1_b_, 2, 8, 3);
    read_layer(conv2_w_, conv2_b_, 8, 8, 3);
    read_layer(conv3_w_, conv3_b_, 8, 16, 3);
    read_layer(conv4_w_, conv4_b_, 16, 16, 3);
    read_layer(conv5_w_, conv5_b_, 24, 8, 3);
    read_layer(conv_out_w_, conv_out_b_, 8, 1, 1);

    weights_loaded_ = true;
    weights_provenance_ = "Real Sentinel-1 SAR Dual-Pol Model (" + weights_path + ")";
    cout << "  [✓] Loaded real trained weights: " << weights_provenance_ << " (" << EXPECTED_FLOATS << " params)\n";
    return true;
}

core::Matrix2D<float> UNet::forward_tile(const core::Tensor3D<float>& input_tensor) const {
    const size_t H = input_tensor.rows();
    const size_t W = input_tensor.cols();

    // ------------------ ENCODER LEVEL 1 ------------------
    core::Tensor3D<float> e1_c1 = Layers::conv2d(input_tensor, conv1_w_, conv1_b_);
    Layers::relu_inplace(e1_c1);
    core::Tensor3D<float> e1_c2 = Layers::conv2d(e1_c1, conv2_w_, conv2_b_);
    Layers::relu_inplace(e1_c2);
    core::Tensor3D<float> p1 = Layers::max_pool2x2(e1_c2);

    // ------------------ ENCODER LEVEL 2 (BOTTLENECK) ------------------
    core::Tensor3D<float> b_c1 = Layers::conv2d(p1, conv3_w_, conv3_b_);
    Layers::relu_inplace(b_c1);
    core::Tensor3D<float> b_c2 = Layers::conv2d(b_c1, conv4_w_, conv4_b_);
    Layers::relu_inplace(b_c2);

    // ------------------ DECODER LEVEL 1 ------------------
    core::Tensor3D<float> up1 = Layers::upsample2x2_bilinear(b_c2, H, W);
    core::Tensor3D<float> cat1 = Layers::concat_channels(up1, e1_c2);

    core::Tensor3D<float> d1_c1 = Layers::conv2d(cat1, conv5_w_, conv5_b_);
    Layers::relu_inplace(d1_c1);

    // Final 1x1 Conv Head
    core::Tensor3D<float> out_feat = Layers::conv2d(d1_c1, conv_out_w_, conv_out_b_);

    // Sigmoid Activation
    core::Matrix2D<float> prob_map(H, W);
    for (size_t r = 0; r < H; ++r) {
        for (size_t c = 0; c < W; ++c) {
            float z = out_feat(0, r, c);
            prob_map(r, c) = 1.0f / (1.0f + exp(-z));
        }
    }

    return prob_map;
}

void UNet::init_weights() {
    auto init_layer = [](vector<float>& w, vector<float>& b, 
                         size_t in_c, size_t out_c, size_t k, float scale) {
        w.resize(out_c * in_c * k * k);
        b.resize(out_c, 0.0f);
        mt19937 rng(42);
        normal_distribution<float> dist(0.0f, scale / sqrt(in_c * k * k));
        for (auto& val : w) val = dist(rng);
    };

    init_layer(conv1_w_, conv1_b_, 2, 8, 3, 1.2f);
    init_layer(conv2_w_, conv2_b_, 8, 8, 3, 1.0f);
    init_layer(conv3_w_, conv3_b_, 8, 16, 3, 1.0f);
    init_layer(conv4_w_, conv4_b_, 16, 16, 3, 1.0f);
    init_layer(conv5_w_, conv5_b_, 24, 8, 3, 1.0f);
    init_layer(conv_out_w_, conv_out_b_, 8, 1, 1, 0.8f);

    conv_out_b_[0] = -1.5f;
}

} // namespace sar::nn
