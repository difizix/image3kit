#pragma once
//! Parity checks of vxhl:: ops against their voxlib reference, on every available backend,
//! voxel type, test shape and slab count. Timing lives in voxlide_bench, not here.

#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include "voxlide.h"

namespace vxhl::testing {

//! Shapes that exercise vector tails, strides, slabs and degenerate sizes, not just cubes.
inline std::vector<int3> parity_shapes() {
    return {int3(17, 23, 31), int3(3, 5, 40), int3(40, 3, 5), int3(1, 9, 9), int3(9, 2, 9),
            int3(9, 9, 2), int3(64, 64, 64)};
}
inline std::vector<int> parity_slab_counts() { return {1, 3, 7}; }

//! Few distinct labels on a blocky pattern plus salt-and-pepper noise: many frequency ties.
template <typename T>
VoxelImageT<T> labelled_volume(int3 n, int nLabels, double noise, uint32_t seed, int block = 4) {
    VoxelImageT<T> img(n, T(0));
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> u01(0.0, 1.0);
    std::uniform_int_distribution<int> lbl(0, nLabels - 1);
    for (int k = 0; k < n.z; ++k)
        for (int j = 0; j < n.y; ++j)
            for (int i = 0; i < n.x; ++i) {
                int l = ((i / block) + (j / block) + (k / block)) % nLabels;
                if (u01(rng) < noise) l = lbl(rng);
                img(i, j, k) = T(l * 10 + 5);
            }
    return img;
}

//! Number of voxels where a and b differ by more than tol (and prints the first few).
template <typename T>
long long count_mismatches(const VoxelImageT<T>& a, const VoxelImageT<T>& b, const std::string& label,
                           double tol = 0.0) {
    if (a.size3() != b.size3()) {
        std::cout << label << ": size mismatch\n";
        return 1;
    }
    long long bad = 0;
    double maxAbs = 0, maxRel = 0;
    const int3 n = a.size3();
    for (int k = 0; k < n.z; ++k)
        for (int j = 0; j < n.y; ++j)
            for (int i = 0; i < n.x; ++i) {
                const double d = std::abs(double(a(i, j, k)) - double(b(i, j, k)));
                if (d > tol) {
                    maxAbs = std::max(maxAbs, d);
                    maxRel = std::max(maxRel, d / std::max(1.0, std::abs(double(a(i, j, k)))));
                    if (bad++ < 5)
                        std::cout << label << ": (" << i << "," << j << "," << k << ") ref=" << +a(i, j, k)
                                  << " got=" << +b(i, j, k) << "\n";
                }
            }
    if (bad)
        std::cout << label << ": " << bad << " mismatching voxels (of " << (long long)n.x * n.y * n.z
                  << ", max |diff| " << maxAbs << ", max rel " << maxRel << ")\n";
    return bad;
}

//! Tolerance for the GPU backend only, for ops that voxlib computes in float32: Halide's Vulkan
//! backend ignores strict_float (no NoContraction in the SPIR-V), so the driver may fuse a*b+c.
//! Halide CPU stays exact. Integer images: at most `intFraction` of voxels differ, by at most 1;
//! float images: every |diff| <= floatAbs. Default: exact.
struct GpuTol {
    double intFraction = 0;
    double floatAbs = 0;
};

//! Run `ref` (voxlib) and `test` (vxhl::) on make(type, shape) for every voxel type, shape,
//! available backend and slab count; return the total number of mismatching voxels.
//! make: (auto typeTag, int3 n) -> VoxelImageT<decltype(typeTag)>; ref/test: (auto& img) -> void.
template <typename Make, typename Ref, typename Test>
long long parity_sweep(const std::string& op, Make make, Ref ref, Test test, GpuTol gpuTol = {}) {
    long long bad = 0;
    const Backend saved = get_backend();
    auto oneType = [&](auto tag, const char* tname) {
        for (int3 n : parity_shapes()) {
            const auto input = make(tag, n);
            auto expect = input;
            ref(expect);
            for (Backend b : available_backends()) {
                set_backend(b);
                const std::vector<int> slabs = (b == Backend::Voxlib) ? std::vector<int>{1} : parity_slab_counts();
                for (int s : slabs) {
                    const std::size_t bytes = std::size_t(n.x) * n.y * n.z * sizeof(decltype(tag));
                    set_max_slab_bytes(s == 1 ? 0 : (bytes + s - 1) / s);
                    auto got = input;
                    test(got);
                    const std::string label = op + "<" + tname + "> " + backend_name(b) + " " +
                                              std::to_string(n.x) + "x" + std::to_string(n.y) + "x" +
                                              std::to_string(n.z) + " slabs=" + std::to_string(s);
                    const bool tolerant = b == Backend::Gpu && (gpuTol.intFraction > 0 || gpuTol.floatAbs > 0);
                    if (!tolerant) {
                        bad += count_mismatches(expect, got, label);
                    } else if (std::is_floating_point_v<decltype(tag)>) {
                        bad += count_mismatches(expect, got, label + " (tol " + std::to_string(gpuTol.floatAbs) + ")",
                                                gpuTol.floatAbs);
                    } else {
                        const long long off = count_mismatches(expect, got, label + " (allowed: few off by 1)");
                        const long long big = count_mismatches(expect, got, label + " (off by >1)", 1.0);
                        if (big || off > gpuTol.intFraction * double(n.x) * n.y * n.z) bad += off;
                    }
                }
            }
        }
    };
    oneType((unsigned char)0, "u8");
    oneType((unsigned short)0, "u16");
    oneType(int(0), "i32");
    oneType(float(0), "f32");
    set_max_slab_bytes(0);
    set_backend(saved);
    return bad;
}

}  // namespace vxhl::testing

//! One line per op: parity of vxhl::<op> vs voxlib over types × backends × shapes × slabs.
#define VXHL_PARITY_TEST(NAME, MAKE, REF, TEST) \
    UTEST(VoxlideParity, NAME) { EXPECT_EQ(0LL, vxhl::testing::parity_sweep(#NAME, MAKE, REF, TEST)); }
//! Same, with a GPU-only tolerance (GpuTol{intFraction, floatAbs}) for float32 ops; see GpuTol.
#define VXHL_PARITY_TEST_GPU_TOL(NAME, MAKE, REF, TEST, INT_FRACTION, FLOAT_ABS)                     \
    UTEST(VoxlideParity, NAME) {                                                                      \
        EXPECT_EQ(0LL, vxhl::testing::parity_sweep(#NAME, MAKE, REF, TEST,                           \
                                                   vxhl::testing::GpuTol{INT_FRACTION, FLOAT_ABS})); \
    }
//! For ops that also return a change count: REF/TEST are (auto& img) -> long long. The counts
//! must be equal on every backend/slab count, besides the images.
#define VXHL_PARITY_TEST_COUNT(NAME, MAKE, REF, TEST)                                              \
    UTEST(VoxlideParity, NAME) {                                                                    \
        long long refCount = 0, countBad = 0;                                                       \
        const long long bad = vxhl::testing::parity_sweep(                                          \
            #NAME, MAKE, [&](auto& m) { refCount = (REF)(m); },                                     \
            [&](auto& m) {                                                                          \
                const long long c = (TEST)(m);                                                      \
                if (c != refCount) {                                                                \
                    ++countBad;                                                                     \
                    std::cout << #NAME << ": count " << c << " != reference " << refCount << "\n"; \
                }                                                                                   \
            });                                                                                     \
        EXPECT_EQ(0LL, bad);                                                                        \
        EXPECT_EQ(0LL, countBad);                                                                   \
    }
