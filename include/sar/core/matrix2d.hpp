#pragma once

#include <vector>
#include <cstddef>
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace sar::core {

/**
 * @brief High-performance, cache-aligned 2D tensor buffer for radar imagery.
 * Stored in row-major continuous memory to maximize CPU L1/L2 prefetching.
 */
template <typename T = float>
class Matrix2D {
public:
    Matrix2D() : rows_(0), cols_(0), data_() {}
    Matrix2D(size_t rows, size_t cols, T initial_val = T(0))
        : rows_(rows), cols_(cols), data_(rows * cols, initial_val) {}

    inline size_t rows() const noexcept { return rows_; }
    inline size_t cols() const noexcept { return cols_; }
    inline size_t size() const noexcept { return data_.size(); }

    inline T* data() noexcept { return data_.data(); }
    inline const T* data() const noexcept { return data_.data(); }

    inline T& operator()(size_t r, size_t c) noexcept {
        return data_[r * cols_ + c];
    }

    inline const T& operator()(size_t r, size_t c) const noexcept {
        return data_[r * cols_ + c];
    }

    inline T& at(size_t r, size_t c) {
        if (r >= rows_ || c >= cols_) {
            throw std::out_of_range("Matrix2D index out of bounds");
        }
        return data_[r * cols_ + c];
    }

    inline const T& at(size_t r, size_t c) const {
        if (r >= rows_ || c >= cols_) {
            throw std::out_of_range("Matrix2D index out of bounds");
        }
        return data_[r * cols_ + c];
    }

    void fill(T value) noexcept {
        std::fill(data_.begin(), data_.end(), value);
    }

    void resize(size_t rows, size_t cols, T initial_val = T(0)) {
        rows_ = rows;
        cols_ = cols;
        data_.assign(rows * cols, initial_val);
    }

    T min_val() const noexcept {
        if (data_.empty()) return T(0);
        return *std::min_element(data_.begin(), data_.end());
    }

    T max_val() const noexcept {
        if (data_.empty()) return T(0);
        return *std::max_element(data_.begin(), data_.end());
    }

    double mean() const noexcept {
        if (data_.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& v : data_) sum += v;
        return sum / data_.size();
    }

    double variance() const noexcept {
        if (data_.size() <= 1) return 0.0;
        double m = mean();
        double sum_sq = 0.0;
        for (const auto& v : data_) {
            double diff = v - m;
            sum_sq += diff * diff;
        }
        return sum_sq / data_.size();
    }

private:
    size_t rows_;
    size_t cols_;
    std::vector<T> data_;
};

} // namespace sar::core
