#include "sar/physics/speckle_filter.hpp"
#include <cmath>
#include <algorithm>

using namespace std;

namespace sar::physics {

SpeckleFilter::SpeckleFilter(int window_size, float looks)
    : window_size_(window_size | 1),
      looks_(looks),
      cu_(1.0f / sqrt(looks)),
      cmax_(sqrt(1.0f + 2.0f / looks)) {}

core::Matrix2D<float> SpeckleFilter::filter(const core::Matrix2D<float>& input, 
                                            FilterType type) const {
    const size_t rows = input.rows();
    const size_t cols = input.cols();
    core::Matrix2D<float> output(rows, cols);

    const int rad = window_size_ / 2;
    const float cu_sq = cu_ * cu_;
    const float damping = 1.0f;

    #pragma omp parallel for schedule(dynamic)
    for (int r = 0; r < static_cast<int>(rows); ++r) {
        for (int c = 0; c < static_cast<int>(cols); ++c) {
            float sum = 0.0f;
            float sum_sq = 0.0f;
            int count = 0;

            const int r_start = max(0, r - rad);
            const int r_end = min(static_cast<int>(rows) - 1, r + rad);
            const int c_start = max(0, c - rad);
            const int c_end = min(static_cast<int>(cols) - 1, c + rad);

            for (int wr = r_start; wr <= r_end; ++wr) {
                for (int wc = c_start; wc <= c_end; ++wc) {
                    float val = input(wr, wc);
                    sum += val;
                    sum_sq += val * val;
                    ++count;
                }
            }

            if (count == 0) {
                output(r, c) = input(r, c);
                continue;
            }

            const float local_mean = sum / count;
            const float local_var = max(0.0f, (sum_sq / count) - (local_mean * local_mean));
            const float ci = (local_mean > 1e-6f) ? (sqrt(local_var) / local_mean) : 0.0f;
            const float current_val = input(r, c);

            if (type == FilterType::Lee) {
                const float ci_sq = ci * ci;
                float weight = 0.0f;
                if (ci_sq > cu_sq) {
                    weight = (ci_sq - cu_sq) / (ci_sq * (1.0f + cu_sq));
                }
                weight = clamp(weight, 0.0f, 1.0f);
                output(r, c) = local_mean + weight * (current_val - local_mean);
            } else {
                if (ci <= cu_) {
                    output(r, c) = local_mean;
                } else if (ci >= cmax_) {
                    output(r, c) = current_val;
                } else {
                    float weight = exp(-damping * (ci - cu_) / (cmax_ - ci));
                    weight = clamp(weight, 0.0f, 1.0f);
                    output(r, c) = local_mean * weight + current_val * (1.0f - weight);
                }
            }
        }
    }

    return output;
}

} // namespace sar::physics
