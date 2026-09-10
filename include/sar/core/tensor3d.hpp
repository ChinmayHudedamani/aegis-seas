#pragma once

#include <vector>
#include <cstddef>
#include <stdexcept>
#include <algorithm>

namespace sar::core {

/**
 * @brief 3D Tensor for Deep Learning Feature Maps: [Channels, Rows, Cols].
 * Contiguous row-major memory layout.
 */
template <typename T = float>
class Tensor3D {
public:
    Tensor3D() : channels_(0), rows_(0), cols_(0), data_() {}
    Tensor3D(size_t channels, size_t rows, size_t cols, T initial_val = T(0))
        : channels_(channels), rows_(rows), cols_(cols), data_(channels * rows * cols, initial_val) {}

    inline size_t channels() const noexcept { return channels_; }
    inline size_t rows() const noexcept { return rows_; }
    inline size_t cols() const noexcept { return cols_; }
    inline size_t size() const noexcept { return data_.size(); }

    inline T* data() noexcept { return data_.data(); }
    inline const T* data() const noexcept { return data_.data(); }

    inline T& operator()(size_t ch, size_t r, size_t c) noexcept {
        return data_[(ch * rows_ + r) * cols_ + c];
    }

    inline const T& operator()(size_t ch, size_t r, size_t c) const noexcept {
        return data_[(ch * rows_ + r) * cols_ + c];
    }

    void resize(size_t channels, size_t rows, size_t cols, T initial_val = T(0)) {
        channels_ = channels;
        rows_ = rows;
        cols_ = cols;
        data_.assign(channels * rows * cols, initial_val);
    }

    void fill(T val) noexcept {
        std::fill(data_.begin(), data_.end(), val);
    }

private:
    size_t channels_;
    size_t rows_;
    size_t cols_;
    std::vector<T> data_;
};

} // namespace sar::core
