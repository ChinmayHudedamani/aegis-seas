#pragma once

#include "sar/core/matrix2d.hpp"
#include <vector>

namespace sar::simulation {

struct GroundTruthSlick {
    size_t center_r;
    size_t center_c;
    size_t radius_r;
    size_t radius_c;
    float damping_db;
    bool is_look_alike;
};

class SyntheticSARScene {
public:
    static void generate_scene(
        size_t rows, size_t cols,
        core::Matrix2D<float>& out_vv_dn,
        core::Matrix2D<float>& out_vh_dn,
        std::vector<GroundTruthSlick>& out_ground_truth,
        unsigned int seed = 1337);
};

} // namespace sar::simulation
