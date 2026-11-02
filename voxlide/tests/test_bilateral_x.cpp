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

// voxlib computes in float32: Halide CPU is exact (strict_float); the GPU gets a tolerance
// because Halide's Vulkan backend drops strict_float (GpuTol in parity_harness.h): at most
// 0.01% of integer voxels off by 1; float images within 2.

// _bilateralX (Python bilateral_wide): (nItrs, kernRad, Xstp, sigma², sharp, sigma_d²) as the binding passes them.
#define BILATX_PARITY(NAME, NI, KR, XS, SG, SH, SD)                                                            \
    VXHL_PARITY_TEST_GPU_TOL(NAME, make, [](auto& m) { ::_bilateralX(m, NI, KR, XS, SG, SH, SD); },      \
                             [](auto& m) { vxhl::_bilateralX(m, NI, KR, XS, SG, SH, SD); }, 1e-4, 2.0)

BILATX_PARITY(bilateral_wide_defaults, 1, 1, 2, 256.0, 0.1, 4.0)
BILATX_PARITY(bilateral_wide_3it_r2_x2, 3, 2, 2, 256.0, 0.1, 4.0)
BILATX_PARITY(bilateral_wide_2it_r1_x3_sharp, 2, 1, 3, 64.0, 0.6, 9.0)

// single pass with a radius that is not a multiple of the step (asymmetric offsets), as other ops call it
VXHL_PARITY_TEST_GPU_TOL(bilateralX_r3_s2, make, [](auto& m) { ::bilateralX(m, 3, 2, 100.f, 0.05f, 12.f); },
                         [](auto& m) { vxhl::bilateralX(m, 3, 2, 100.f, 0.05f, 12.f); }, 1e-4, 2.0)
VXHL_PARITY_TEST_GPU_TOL(bilateralX_r1_s1_cap, make,
                         [](auto& m) { ::bilateralX(m, 1, 1, 16.f, 0.05f, 12.f, 40.f, 200.f); },
                         [](auto& m) { vxhl::bilateralX(m, 1, 1, 16.f, 0.05f, 12.f, 40.f, 200.f); }, 1e-4, 2.0)
