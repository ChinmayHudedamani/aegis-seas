#include "sar/nn/sliding_window.hpp"
#include <cmath>
#include <algorithm>

using namespace std;

namespace sar::nn {

SlidingWindowReconstructor::SlidingWindowReconstructor(size_t tile_size, size_t stride)
    : tile_size_(tile_size),
      stride_(stride),
      gaussian_weight_(tile_size, tile_size) 
{
    const float center = (tile_size - 1) / 2.0f;
    const float sigma = tile_size / 3.0f;
    const float two_sigma_sq = 2.0f * sigma * sigma;

    for (size_t r = 0; r < tile_size; ++r) {
        for (size_t c = 0; c < tile_size; ++c) {
            float dr = r - center;
            float dc = c - center;
            gaussian_weight_(r, c) = exp(-(dr * dr + dc * dc) / two_sigma_sq);
        }
    }
}

core::Matrix2D<float> SlidingWindowReconstructor::predict_scene(
    const UNet& model,
    const core::Matrix2D<float>& norm_vv,
    const core::Matrix2D<float>& norm_vh) const 
{
    const size_t rows = norm_vv.rows();
    const size_t cols = norm_vv.cols();

    core::Matrix2D<float> accum_prob(rows, cols, 0.0f);
    core::Matrix2D<float> accum_weight(rows, cols, 0.0f);

    for (size_t r = 0; r < rows; r += stride_) {
        size_t r_end = min(r + tile_size_, rows);
        size_t r_start = (r_end == rows && rows >= tile_size_) ? (rows - tile_size_) : r;

        for (size_t c = 0; c < cols; c += stride_) {
            size_t c_end = min(c + tile_size_, cols);
            size_t c_start = (c_end == cols && cols >= tile_size_) ? (cols - tile_size_) : c;

            size_t cur_h = r_end - r_start;
            size_t cur_w = c_end - c_start;

            core::Tensor3D<float> tile(2, cur_h, cur_w);
            for (size_t tr = 0; tr < cur_h; ++tr) {
                for (size_t tc = 0; tc < cur_w; ++tc) {
                    tile(0, tr, tc) = norm_vv(r_start + tr, c_start + tc);
                    tile(1, tr, tc) = norm_vh(r_start + tr, c_start + tc);
                }
            }

            core::Matrix2D<float> tile_prob = model.forward_tile(tile);

            for (size_t tr = 0; tr < cur_h; ++tr) {
                for (size_t tc = 0; tc < cur_w; ++tc) {
                    float gw = (cur_h == tile_size_ && cur_w == tile_size_) ? 
                                gaussian_weight_(tr, tc) : 1.0f;
                    accum_prob(r_start + tr, c_start + tc) += tile_prob(tr, tc) * gw;
                    accum_weight(r_start + tr, c_start + tc) += gw;
                }
            }

            if (c_end == cols) break;
        }
        if (r_end == rows) break;
    }

    core::Matrix2D<float> final_prob(rows, cols);
    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            float w = accum_weight(r, c);
            final_prob(r, c) = (w > 1e-5f) ? (accum_prob(r, c) / w) : 0.0f;
        }
    }

    return final_prob;
}

} // namespace sar::nn
