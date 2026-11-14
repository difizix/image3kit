#include "utest.h"
#include "parity_harness.h"
#include "voxelImageML.h"

namespace {
// Three noisy phases (blocks of 3 levels) for segment2; values scaled to the type.
template <typename T> VoxelImageT<T> phases(int3 n, uint32_t seed) {
    VoxelImageT<T> img(n, T(0));
    std::mt19937 rng(seed);
    std::normal_distribution<double> noise(0.0, 14.0);
    const double scale = sizeof(T) == 1 ? 1.0 : 200.0;
    for (int k = 0; k < n.z; ++k)
        for (int j = 0; j < n.y; ++j)
            for (int i = 0; i < n.x; ++i) {
                const int ph = ((i / 7) + (j / 5) * 2 + (k / 6)) % 3;
                const double v = 50.0 + 70.0 * ph + noise(rng);
                img(i, j, k) = T(std::clamp(v, 0.0, 255.0) * scale);
            }
    return img;
}
template <typename T> vars<intOr<T>> thresholds() {
    const int s = sizeof(T) == 1 ? 1 : 200;
    return vars<intOr<T>>(std::vector<intOr<T>>{intOr<T>(1), intOr<T>(85 * s), intOr<T>(155 * s), intOr<T>(254 * s)});
}

// segment2 on every backend and slab count vs voxlib. Returns #mismatching voxels.
template <typename T> long long segment2_sweep(int3 n, int krnl, int nItrs) {
    long long bad = 0;
    const auto saved = vxhl::get_backend();
    const auto input = phases<T>(n, 77);
    auto expect = input;
    ::segment2(expect, 3, thresholds<T>(), vars<int>(std::vector<int>{1, 2, 2}), 16.0 * (sizeof(T) == 1 ? 1 : 200), 0.05, 0.1, 2.0, 0.0,
               krnl, nItrs, 0);
    for (vxhl::Backend b : vxhl::available_backends()) {
        if (b == vxhl::Backend::Voxlib) continue;
        vxhl::set_backend(b);
        for (int s : {1, 3}) {
            vxhl::set_max_slab_bytes(s == 1 ? 0 : (std::size_t(n.x + 16) * (n.y + 16) * (n.z + 16) * sizeof(T)) / s + 1);
            auto got = input;
            vxhl::segment2(got, 3, thresholds<T>(), vars<int>(std::vector<int>{1, 2, 2}), 16.0 * (sizeof(T) == 1 ? 1 : 200), 0.05, 0.1, 2.0,
                           0.0, krnl, nItrs, 0);
            bad += vxhl::testing::count_mismatches(expect, got, std::string("segment2 ") + vxhl::backend_name(b) +
                                                                     " slabs=" + std::to_string(s));
        }
    }
    vxhl::set_max_slab_bytes(0);
    vxhl::set_backend(saved);
    return bad;
}
}  // namespace

UTEST(VoxlideParity, segment2_u8) { EXPECT_EQ(0LL, segment2_sweep<unsigned char>(int3(40, 36, 32), 2, 6)); }
UTEST(VoxlideParity, segment2_u8_odd_krnl1) { EXPECT_EQ(0LL, segment2_sweep<unsigned char>(int3(17, 23, 31), 1, 7)); }
UTEST(VoxlideParity, segment2_u16) { EXPECT_EQ(0LL, segment2_sweep<unsigned short>(int3(33, 29, 24), 2, 6)); }

// One voting pass alone, from a mid-pipeline state (labels from replaceRange, grad from magGradient).
#define INT8_16_ONLY if constexpr (sizeof(std::decay_t<decltype(m(0, 0, 0))>) > 2) return;
#define VOTE_ARGS(T) vars<T>(std::vector<T>{T(50 * S), T(120 * S), T(190 * S)}), 1, intOr<T>(254 * S), 2, 0.05, 0.1, 0.5, 4.0, 1. / 12, 16.0 * S
#define VOTE_SETUP                                                                             \
    using T = std::decay_t<decltype(m(0, 0, 0))>;                                              \
    const int S = sizeof(T) == 1 ? 1 : 200;                                                    \
    const VoxelImageT<T> orig = m;                                                             \
    const VoxelImageT<T> grad = ::magGradient(orig, 1.0);                                      \
    ::replaceRange(m, T(0), T(85 * S), T(50 * S));                                             \
    ::replaceRange(m, T(85 * S + 1), T(155 * S), T(120 * S));                                  \
    ::replaceRange(m, T(155 * S + 1), maxT(T), T(190 * S));
VXHL_PARITY_TEST(segment_vote, [](auto tag, int3 n) { return phases<decltype(tag)>(n, 78); },
                 [](auto& m) { INT8_16_ONLY VOTE_SETUP ::segmentVote(m, orig, grad, VOTE_ARGS(T)); },
                 [](auto& m) { INT8_16_ONLY VOTE_SETUP vxhl::segmentVote(m, orig, grad, VOTE_ARGS(T)); })
