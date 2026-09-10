#include "sar/attribution/cfar_detector.hpp"
#include <cmath>
#include <algorithm>

using namespace std;

namespace sar::attribution {

CFARDetector::CFARDetector(int guard_cells, int training_cells, float pfa)
    : guard_cells_(guard_cells),
      training_cells_(training_cells),
      pfa_(pfa) 
{
    // Threshold factor for 2D CA-CFAR: alpha = N * (P_fa^(-1/N) - 1)
    int total_window = 2 * (training_cells + guard_cells) + 1;
    int guard_window = 2 * guard_cells + 1;
    int num_training_cells = (total_window * total_window) - (guard_window * guard_window);
    
    threshold_factor_ = num_training_cells * (pow(pfa_, -1.0f / num_training_cells) - 1.0f);
}

vector<RadarHardTarget> CFARDetector::detect_targets(
    const core::Matrix2D<float>& vv_db,
    const geo::GeoTransform& transform) const 
{
    const size_t rows = vv_db.rows();
    const size_t cols = vv_db.cols();
    vector<RadarHardTarget> targets;

    const int total_rad = training_cells_ + guard_cells_;
    const int guard_rad = guard_cells_;

    constexpr double R_earth = 6378137.0;
    constexpr double deg_to_rad = 3.141592653589793 / 180.0;
    constexpr double rad_to_deg = 180.0 / 3.141592653589793;
    double lat_rad = transform.origin_lat * deg_to_rad;

    size_t target_id = 1;

    for (int r = total_rad; r < static_cast<int>(rows) - total_rad; ++r) {
        for (int c = total_rad; c < static_cast<int>(cols) - total_rad; ++c) {
            float cut_linear = pow(10.0f, vv_db(r, c) / 10.0f); // Cell Under Test (CUT)

            // Only consider reasonably bright candidates (> -5 dB)
            if (vv_db(r, c) < -5.0f) continue;

            float training_sum = 0.0f;
            int training_count = 0;

            for (int wr = -total_rad; wr <= total_rad; ++wr) {
                for (int wc = -total_rad; wc <= total_rad; ++wc) {
                    // Skip guard cells and CUT
                    if (abs(wr) <= guard_rad && abs(wc) <= guard_rad) {
                        continue;
                    }
                    training_sum += pow(10.0f, vv_db(r + wr, c + wc) / 10.0f);
                    ++training_count;
                }
            }

            if (training_count == 0) continue;

            float noise_floor_linear = training_sum / training_count;
            float adaptive_threshold = noise_floor_linear * threshold_factor_;

            if (cut_linear > adaptive_threshold) {
                float snr_db = 10.0f * log10(cut_linear / (noise_floor_linear + 1e-7f));

                double d_x = c * transform.pixel_size_meters;
                double d_y = r * transform.pixel_size_meters;
                double lon = transform.origin_lon + (d_x / (R_earth * cos(lat_rad))) * rad_to_deg;
                double lat = transform.origin_lat - (d_y / R_earth) * rad_to_deg;

                RadarHardTarget target;
                target.id = target_id++;
                target.pixel_r = r;
                target.pixel_c = c;
                target.location = {lon, lat};
                target.peak_rcs_db = vv_db(r, c);
                target.snr_db = snr_db;
                targets.push_back(target);
            }
        }
    }

    return targets;
}

} // namespace sar::attribution
