#include "utest.h"
#include "parity_harness.h"
#include "voxelImageML.h"

namespace {
auto make = [](auto tag, int3 n) {
    using T = decltype(tag);
    auto img = vxhl::testing::labelled_volume<T>(n, 20, 0.3, 515, 3);
    if constexpr (std::is_floating_point_v<T>) {
        long long i = 0;
        for (auto& v : img.data_) v += T(0.4) * T(i++ % 3);
    }
    return img;
};
// Every integer 0..199 (bimodal): voxels sit exactly on the threshold (a bin edge).
auto make_dense = [](auto tag, int3 n) {
    using T = decltype(tag);
    VoxelImageT<T> img(n, T(0));
    long long i = 0;
    for (auto& v : img.data_) { const int r = int((i * 7919) % 200); v = T(i++ % 3 ? r / 2 : 100 + r / 2); }
    return img;
};
// Voxels where vxhl::threshold01_otsu differs from voxlib's (plus header mismatches), on host and DeviceImage.
template <typename M> double diff01(const M& m, int lo, int hi, double skip) {
    const auto ref = ::threshold01_otsu(m, lo, hi, skip);
    using T = std::decay_t<decltype(m(0, 0, 0))>;
    const auto got = vxhl::threshold01_otsu(m, lo, hi, skip);
    const vxhl::DeviceImage<T> d(m);
    const auto gotDev = vxhl::threshold01_otsu(d, lo, hi, skip);
    double bad = !(ref.X0() == got.X0() && ref.dx() == got.dx());
    bad += vxhl::testing::count_mismatches(ref, got, "threshold01_otsu");
    bad += vxhl::testing::count_mismatches(ref, gotDev.host(), "threshold01_otsu dev");
    return bad;
}
}  // namespace

#define OTSU_PARITY(NAME, I, LO, HI) \
    VXHL_SCALAR_PARITY_TEST(NAME, make, [](const auto& m) { return ::otsu_minAvgThresholdAvgMax(m, LO, HI)[I]; }, \
                            [](const auto& m) { return vxhl::otsu_minAvgThresholdAvgMax(m, LO, HI)[I]; }, 0)
OTSU_PARITY(otsu_threshold_0_255, 2, 0, 255)
OTSU_PARITY(otsu_mean0_0_255, 1, 0, 255)
OTSU_PARITY(otsu_mean1_0_255, 3, 0, 255)
OTSU_PARITY(otsu_threshold_wide, 2, 0, 1000)  // more than 256 values: 256 bins of width 1000/256
OTSU_PARITY(otsu_threshold_narrow, 2, 30, 120)

#define T01_PARITY(NAME, ...) \
    VXHL_SCALAR_PARITY_TEST(NAME, make, [](const auto&) { return 0.0; }, [](const auto& m) { return diff01(m, __VA_ARGS__); }, 0)
T01_PARITY(threshold01_otsu_0_255, 0, 255, 0.)
T01_PARITY(threshold01_otsu_wide, 0, 1000, 0.)
T01_PARITY(threshold01_otsu_skip, 0, 255, 0.2)  // skipFrac: threshold from voxlib, apply on the device
VXHL_SCALAR_PARITY_TEST(threshold01_otsu_dense, make_dense, [](const auto&) { return 0.0; }, [](const auto& m) { return diff01(m, 0, 255, 0.); }, 0)
VXHL_SCALAR_PARITY_TEST(otsu_threshold_dense, make_dense, [](const auto& m) { return ::otsu_minAvgThresholdAvgMax(m, 0, 255)[2]; },
                        [](const auto& m) { return vxhl::otsu_minAvgThresholdAvgMax(m, 0, 255)[2]; }, 0)
