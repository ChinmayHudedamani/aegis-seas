#include "sar/nn/layers.hpp"
#include <cmath>
#include <algorithm>

using namespace std;

namespace sar::nn {

core::Tensor3D<float> Layers::conv2d(const core::Tensor3D<float>& in,
                                     const vector<float>& weights,
                                     const vector<float>& bias,
                                     size_t k_size) {
    const size_t in_ch = in.channels();
    const size_t out_ch = bias.size();
    const size_t H = in.rows();
    const size_t W = in.cols();
    const int rad = static_cast<int>(k_size / 2);

    core::Tensor3D<float> out(out_ch, H, W);

    #pragma omp parallel for collapse(2) schedule(static) if(H * W > 1024)
    for (size_t oc = 0; oc < out_ch; ++oc) {
        for (size_t r = 0; r < H; ++r) {
            float b = bias[oc];
            for (size_t c = 0; c < W; ++c) {
                float sum = b;
                for (size_t ic = 0; ic < in_ch; ++ic) {
                    const size_t w_base = ((oc * in_ch + ic) * k_size);
                    for (int kr = -rad; kr <= rad; ++kr) {
                        int in_r = static_cast<int>(r) + kr;
                        if (in_r < 0 || in_r >= static_cast<int>(H)) continue;

                        for (int kc = -rad; kc <= rad; ++kc) {
                            int in_c = static_cast<int>(c) + kc;
                            if (in_c < 0 || in_c >= static_cast<int>(W)) continue;

                            size_t w_idx = (w_base + (kr + rad)) * k_size + (kc + rad);
                            sum += in(ic, in_r, in_c) * weights[w_idx];
                        }
                    }
                }
                out(oc, r, c) = sum;
            }
        }
    }
    return out;
}

void Layers::relu_inplace(core::Tensor3D<float>& t) noexcept {
    float* d = t.data();
    const size_t n = t.size();
    for (size_t i = 0; i < n; ++i) {
        if (d[i] < 0.0f) d[i] = 0.0f;
    }
}

core::Tensor3D<float> Layers::max_pool2x2(const core::Tensor3D<float>& in) {
    const size_t ch = in.channels();
    const size_t out_h = in.rows() / 2;
    const size_t out_w = in.cols() / 2;
    core::Tensor3D<float> out(ch, out_h, out_w);

    for (size_t c = 0; c < ch; ++c) {
        for (size_t r = 0; r < out_h; ++r) {
            for (size_t col = 0; col < out_w; ++col) {
                float v00 = in(c, 2 * r, 2 * col);
                float v01 = in(c, 2 * r, 2 * col + 1);
                float v10 = in(c, 2 * r + 1, 2 * col);
                float v11 = in(c, 2 * r + 1, 2 * col + 1);
                out(c, r, col) = max({v00, v01, v10, v11});
            }
        }
    }
    return out;
}

core::Tensor3D<float> Layers::upsample2x2_bilinear(const core::Tensor3D<float>& in, 
                                                   size_t target_h, 
                                                   size_t target_w) {
    const size_t ch = in.channels();
    const size_t in_h = in.rows();
    const size_t in_w = in.cols();
    core::Tensor3D<float> out(ch, target_h, target_w);

    for (size_t c = 0; c < ch; ++c) {
        for (size_t r = 0; r < target_h; ++r) {
            float src_r = (r + 0.5f) * (static_cast<float>(in_h) / target_h) - 0.5f;
            int r0 = clamp(static_cast<int>(floor(src_r)), 0, static_cast<int>(in_h) - 1);
            int r1 = clamp(r0 + 1, 0, static_cast<int>(in_h) - 1);
            float dr = src_r - r0;

            for (size_t col = 0; col < target_w; ++col) {
                float src_c = (col + 0.5f) * (static_cast<float>(in_w) / target_w) - 0.5f;
                int c0 = clamp(static_cast<int>(floor(src_c)), 0, static_cast<int>(in_w) - 1);
                int c1 = clamp(c0 + 1, 0, static_cast<int>(in_w) - 1);
                float dc = src_c - c0;

                float v00 = in(c, r0, c0);
                float v01 = in(c, r0, c1);
                float v10 = in(c, r1, c0);
                float v11 = in(c, r1, c1);

                float top = v00 * (1.0f - dc) + v01 * dc;
                float bot = v10 * (1.0f - dc) + v11 * dc;
                out(c, r, col) = top * (1.0f - dr) + bot * dr;
            }
        }
    }
    return out;
}

core::Tensor3D<float> Layers::concat_channels(const core::Tensor3D<float>& t1, 
                                             const core::Tensor3D<float>& t2) {
    const size_t ch1 = t1.channels();
    const size_t ch2 = t2.channels();
    const size_t H = t1.rows();
    const size_t W = t1.cols();
    core::Tensor3D<float> out(ch1 + ch2, H, W);

    for (size_t c = 0; c < ch1; ++c) {
        for (size_t r = 0; r < H; ++r) {
            for (size_t col = 0; col < W; ++col) {
                out(c, r, col) = t1(c, r, col);
            }
        }
    }
    for (size_t c = 0; c < ch2; ++c) {
        for (size_t r = 0; r < H; ++r) {
            for (size_t col = 0; col < W; ++col) {
                out(ch1 + c, r, col) = t2(c, r, col);
            }
        }
    }
    return out;
}

} // namespace sar::nn
