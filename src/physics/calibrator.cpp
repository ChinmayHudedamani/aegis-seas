#include "sar/physics/calibrator.hpp"
#include <cmath>
#include <algorithm>

using namespace std;

namespace sar::physics {

Calibrator::Calibrator(const CalibrationMetadata& meta)
    : meta_(meta) {}

core::Matrix2D<float> Calibrator::calibrate_to_db(const core::Matrix2D<float>& raw_dn) const {
    core::Matrix2D<float> sigma0_db(raw_dn.rows(), raw_dn.cols());
    const size_t n = raw_dn.size();
    const float* src = raw_dn.data();
    float* dst = sigma0_db.data();

    const float inv_ak_sq = 1.0f / (meta_.calibration_constant * meta_.calibration_constant);
    const float noise = meta_.noise_equivalent_sigma0;
    constexpr float eps = 1e-7f;

    #pragma omp parallel for schedule(static) if(n > 65536)
    for (size_t i = 0; i < n; ++i) {
        float dn = src[i];
        if (isnan(dn) || isinf(dn) || dn <= 0.0f) {
            dst[i] = meta_.min_db_clip;
            continue;
        }
        float sigma0_linear = (dn * dn + noise) * inv_ak_sq;
        if (sigma0_linear < eps) {
            sigma0_linear = eps;
        }
        float db = 10.0f * log10(sigma0_linear);
        dst[i] = clamp(db, meta_.min_db_clip, meta_.max_db_clip);
    }

    return sigma0_db;
}

core::Matrix2D<float> Calibrator::normalize_minmax(const core::Matrix2D<float>& db_matrix, 
                                                  float min_db, 
                                                  float max_db) {
    core::Matrix2D<float> normalized(db_matrix.rows(), db_matrix.cols());
    const size_t n = db_matrix.size();
    const float* src = db_matrix.data();
    float* dst = normalized.data();
    const float range_inv = 1.0f / (max_db - min_db);

    for (size_t i = 0; i < n; ++i) {
        float val = (src[i] - min_db) * range_inv;
        dst[i] = clamp(val, 0.0f, 1.0f);
    }

    return normalized;
}

} // namespace sar::physics
