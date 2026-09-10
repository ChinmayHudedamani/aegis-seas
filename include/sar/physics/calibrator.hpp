#pragma once

#include "sar/core/matrix2d.hpp"

namespace sar::physics {

struct CalibrationMetadata {
    float calibration_constant = 100.0f;     // A_k calibration scaling factor
    float noise_equivalent_sigma0 = 1e-4f;    // Thermal noise floor (~ -40 to -22 dB)
    float min_db_clip = -40.0f;               // Lower bound (dB)
    float max_db_clip = 15.0f;                // Upper bound (dB)
};

class Calibrator {
public:
    explicit Calibrator(const CalibrationMetadata& meta = CalibrationMetadata());

    /**
     * @brief Transforms raw Digital Number (DN) matrix to calibrated Sigma Naught (dB).
     */
    core::Matrix2D<float> calibrate_to_db(const core::Matrix2D<float>& raw_dn) const;

    /**
     * @brief Normalizes Decibel range [min_db, max_db] to unit float [0.0, 1.0] for Neural Network input.
     */
    static core::Matrix2D<float> normalize_minmax(const core::Matrix2D<float>& db_matrix, 
                                                  float min_db = -30.0f, 
                                                  float max_db = 0.0f);

private:
    CalibrationMetadata meta_;
};

} // namespace sar::physics
