#include "utest.h"
#include "parity_harness.h"
#include "voxelImageProcess.h"

namespace {
// Smooth ramps plus noise in [0, 250]: realistic grey levels, so the weights vary.
auto make = [](auto tag, int3 n) {
    using T = decltype(tag);
    VoxelImageT<T> img(n, T(0));
    std::mt19937 rng(303);
    std::normal_distribution<double> noise(0.0, 12.0);
    for (int k = 0; k < n.z; ++k)
        for (int j = 0; j < n.y; ++j)
            for (int i = 0; i < n.x; ++i) {
                const double base = ((i / 5 + j / 4 + k / 3) % 3) * 80.0 + 30.0;
                img(i, j, k) = T(std::clamp(base + noise(rng), 0.0, 250.0));
            }
    return img;
};
}  // namespace

// (nItrs, kernRad, sigma, sharpness): defaults, several outer×inner iterations, sharper, tiny sigma.
#define BILAT_PARITY(NAME, NI, KR, SG, SH)                                                          \
    VXHL_PARITY_TEST(NAME, make, [](auto& m) { VoxLib::smoothBilateral(m, NI, KR, SG, SH); }, \
                     [](auto& m) { vxhl::smoothBilateral(m, NI, KR, SG, SH); })

BILAT_PARITY(smooth_bilateral_1_1, 1, 1, 16.0, 0.1)
BILAT_PARITY(smooth_bilateral_2_3, 2, 3, 16.0, 0.1)
BILAT_PARITY(smooth_bilateral_3_2_sharp, 3, 2, 64.0, 0.8)
BILAT_PARITY(smooth_bilateral_1_2_sigma1, 1, 2, 1.0, 0.0)
