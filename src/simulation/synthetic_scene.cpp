#include "sar/simulation/synthetic_scene.hpp"
#include <random>
#include <cmath>

using namespace std;

namespace sar::simulation {

void SyntheticSARScene::generate_scene(
    size_t rows, size_t cols,
    core::Matrix2D<float>& out_vv_dn,
    core::Matrix2D<float>& out_vh_dn,
    vector<GroundTruthSlick>& out_ground_truth,
    unsigned int seed) 
{
    out_vv_dn.resize(rows, cols);
    out_vh_dn.resize(rows, cols);
    out_ground_truth.clear();

    mt19937 rng(seed);

    constexpr float mean_vv_linear = 0.10f; // ~ -10 dB
    constexpr float mean_vh_linear = 0.01f; // ~ -20 dB
    constexpr float looks = 4.4f;

    gamma_distribution<float> speckle_dist(looks, 1.0f / looks);

    // 1. Vessel Bilge Trail
    GroundTruthSlick spill1{rows / 3, cols / 3, rows / 8, cols / 4, 9.5f, false};
    out_ground_truth.push_back(spill1);

    // 2. Platform Leak
    GroundTruthSlick spill2{(2 * rows) / 3, (2 * cols) / 3, rows / 10, rows / 10, 11.0f, false};
    out_ground_truth.push_back(spill2);

    // 3. Biogenic Look-Alike
    GroundTruthSlick look_alike{rows / 4, (3 * cols) / 4, rows / 12, cols / 8, 3.0f, true};
    out_ground_truth.push_back(look_alike);

    constexpr float ak = 100.0f;

    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            float vv_reflectivity = mean_vv_linear;
            float vh_reflectivity = mean_vh_linear;

            for (const auto& slick : out_ground_truth) {
                float dr = static_cast<float>(r) - slick.center_r;
                float dc = static_cast<float>(c) - slick.center_c;
                float norm_dist_sq = (dr * dr) / (slick.radius_r * slick.radius_r) +
                                     (dc * dc) / (slick.radius_c * slick.radius_c);

                if (norm_dist_sq <= 1.0f) {
                    float damping_factor = pow(10.0f, -slick.damping_db / 10.0f);
                    vv_reflectivity *= damping_factor;
                    vh_reflectivity *= max(0.2f, damping_factor * 1.5f);
                }
            }

            // Ship target
            if (r >= (rows / 3 - 2) && r <= (rows / 3 + 2) &&
                c >= (cols / 3 - 2) && c <= (cols / 3 + 2)) {
                vv_reflectivity = 25.0f;
                vh_reflectivity = 10.0f;
            }

            float speckle_vv = speckle_dist(rng);
            float speckle_vh = speckle_dist(rng);

            float noisy_vv_linear = vv_reflectivity * speckle_vv;
            float noisy_vh_linear = vh_reflectivity * speckle_vh;

            out_vv_dn(r, c) = sqrt(max(0.0f, noisy_vv_linear) * ak * ak);
            out_vh_dn(r, c) = sqrt(max(0.0f, noisy_vh_linear) * ak * ak);
        }
    }
}

} // namespace sar::simulation
