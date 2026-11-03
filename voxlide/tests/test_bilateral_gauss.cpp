#include "utest.h"
#include "parity_harness.h"
#include "voxelImageML.h"

namespace {
// Smooth ramps plus noise in [0, 250].
auto make = [](auto tag, int3 n) {
    using T = decltype(tag);
    VoxelImageT<T> img(n, T(0));
    std::mt19937 rng(404);
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

// Same float32 / GPU tolerance policy as test_bilateral_x.cpp.
// _bilateralGauss (Python bilateral_gauss): (nItrs, kernRad, sigma², sharp, sigma_d²) as the binding passes them.
#define BILATG_PARITY(NAME, NI, KR, SG, SH, SD)                                                         \
    VXHL_PARITY_TEST_GPU_TOL(NAME, make, [](auto& m) { ::_bilateralGauss(m, NI, KR, SG, SH, SD); },      \
                             [](auto& m) { vxhl::_bilateralGauss(m, NI, KR, SG, SH, SD); }, 1e-4, 2.0)

BILATG_PARITY(bilateral_gauss_defaults, 1, 1, 256.0, 0.1, 4.0)
BILATG_PARITY(bilateral_gauss_3it_r2, 3, 2, 256.0, 0.1, 4.0)
BILATG_PARITY(bilateral_gauss_2it_r3_sharp, 2, 3, 64.0, 0.6, 9.0)
BILATG_PARITY(bilateral_gauss_r0, 1, 0, 64.0, 0.6, 9.0)

VXHL_PARITY_TEST_GPU_TOL(bilateralGauss_cap, make,
                         [](auto& m) { ::bilateralGauss(m, 2, 16.f, 0.05f, 12.f, 40.f, 200.f); },
                         [](auto& m) { vxhl::bilateralGauss(m, 2, 16.f, 0.05f, 12.f, 40.f, 200.f); }, 1e-4, 2.0)

