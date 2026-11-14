#include "utest.h"
#include "parity_harness.h"
#include "voxelImageI.h"

namespace {
// Three labels (5, 15, 25) with noise: every neighbourhood count occurs.
auto make3 = [](auto tag, int3 n) { return vxhl::testing::labelled_volume<decltype(tag)>(n, 3, 0.45, 717, 3); };
// 0/1 images for median06 (values summed): noisy blocks, and a variant with values {0, 1, 3}.
auto make01 = [](auto tag, int3 n) {
    auto img = vxhl::testing::labelled_volume<decltype(tag)>(n, 2, 0.4, 718, 3);
    for (auto& v : img.data_) v = decltype(tag)(v > 5 ? 1 : 0);
    return img;
};
auto make013 = [](auto tag, int3 n) {
    auto img = vxhl::testing::labelled_volume<decltype(tag)>(n, 3, 0.4, 719, 3);
    for (auto& v : img.data_) v = decltype(tag)(v < 10 ? 0 : v < 20 ? 1 : 3);
    return img;
};
template <typename M> using VoxT = std::decay_t<decltype(std::declval<M&>()(0, 0, 0))>;
}  // namespace

// ---- median032 (nAdj0, nAdj1, lbl0, lbl1): counts over 27 + 6 voxels
#define M032(NAME, A0, A1, L0, L1)                                                                              \
    VXHL_PARITY_TEST(NAME, make3, [](auto& m) { using T = VoxT<decltype(m)>; ::median032(m, A0, A1, T(L0), T(L1)); }, \
                     [](auto& m) { using T = VoxT<decltype(m)>; vxhl::median032(m, A0, A1, T(L0), T(L1)); })
M032(median032_a, 14, 14, 15, 25)
M032(median032_low_thresholds, 0, 0, 5, 15)
M032(median032_asym, 20, 8, 25, 5)
M032(median032_unreachable, 99, 99, 15, 25)
M032(median032_same_labels, 3, 3, 15, 15)  // lbl0 == lbl1: nothing can change

// ---- median06_grow (FaceMedGrowToFrom: lblTo, lblFrm, ndif)
#define M06G(NAME, TO, FRM, NDIF)                                                                              \
    VXHL_PARITY_TEST(NAME, make3, [](auto& m) { using T = VoxT<decltype(m)>; ::FaceMedGrowToFrom(m, T(TO), T(FRM), NDIF); }, \
                     [](auto& m) { using T = VoxT<decltype(m)>; vxhl::FaceMedGrowToFrom(m, T(TO), T(FRM), NDIF); })
M06G(median06_grow_0, 15, 5, 0)
M06G(median06_grow_ndif2, 25, 15, 2)
M06G(median06_grow_negative_ndif, 5, 25, -3)
M06G(median06_grow_absent_label, 99, 5, 0)

// ---- median06 (nAdj0, nAdj1) with the change count
#define M06(NAME, MAKE, A0, A1)                                                                      \
    VXHL_PARITY_TEST_COUNT(NAME, MAKE, [](auto& m) { return (long long)::median06(m, A0, A1); },   \
                           [](auto& m) { return (long long)vxhl::median06(m, A0, A1); })
M06(median06_a, make01, 1, 5)
M06(median06_b, make01, 2, 4)
M06(median06_sums, make013, 3, 9)
M06(median06_never_zero, make01, -1, 7)  // nothing set to 0, nothing set to 1

// ---- grow_label
#define GL(NAME, L)                                                                                     \
    VXHL_PARITY_TEST(NAME, make3, [](auto& m) { using T = VoxT<decltype(m)>; ::growLabel(m, T(L)); },  \
                     [](auto& m) { using T = VoxT<decltype(m)>; vxhl::growLabel(m, T(L)); })
GL(grow_label_15, 15)
GL(grow_label_absent, 99)

// repeated growth (the Python loop idiom): 3 steps compose like voxlib's
VXHL_PARITY_TEST(grow_label_x3, make3,
                 [](auto& m) { using T = VoxT<decltype(m)>; for (int i = 0; i < 3; ++i) ::growLabel(m, T(25)); },
                 [](auto& m) { using T = VoxT<decltype(m)>; for (int i = 0; i < 3; ++i) vxhl::growLabel(m, T(25)); })
