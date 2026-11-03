#include "utest.h"
#include "parity_harness.h"
#include "voxelImageI.h"

namespace {
// Values 5..195; float images get fractional parts (varianceDbl truncates them to int).
auto make = [](auto tag, int3 n) {
    using T = decltype(tag);
    auto img = vxhl::testing::labelled_volume<T>(n, 20, 0.3, 707, 3);
    if constexpr (std::is_floating_point_v<T>) {
        long long i = 0;
        for (auto& v : img.data_) v += T(0.37) * T(i++ % 3);
    }
    return img;
};
// Plus maxT(T) sentinel voxels (porosity excludes them from the total).
auto make_sentinel = [](auto tag, int3 n) {
    using T = decltype(tag);
    auto img = make(tag, n);
    for (std::size_t i = 0; i < img.data_.size(); i += 17) img.data_[i] = maxT(T);
    return img;
};
// Sums of non-integer terms (Σ(v−mean)², f32 values) depend on the summation order, which
// isn't fixed in voxlib (OpenMP) either: naive-summation bound N·eps ≈ 3e-11 at 64³.
constexpr double kTol = 1e-10;
constexpr double kExact = 0;  // counts, comparisons, min/max
}  // namespace

#define STAT_PARITY(NAME, MAKE, EXPR, TOL)                                                                      \
    VXHL_SCALAR_PARITY_TEST(NAME, MAKE, [](const auto& m) { using T [[maybe_unused]] = std::decay_t<decltype(m(0, 0, 0))>; return ::EXPR; }, \
                            [](const auto& m) { using T [[maybe_unused]] = std::decay_t<decltype(m(0, 0, 0))>; return vxhl::EXPR; }, TOL)

STAT_PARITY(variance_all, make, varianceDbl(m, 0, 255), kTol)
STAT_PARITY(variance_range, make, varianceDbl(m, 20, 120), kTol)
STAT_PARITY(variance_one_value, make, varianceDbl(m, 25, 25), kTol)
STAT_PARITY(variance_empty, make, varianceDbl(m, 100, 50), kTol)
STAT_PARITY(stdev_range, make, stdevDbl(m, 10, 200), kTol)
STAT_PARITY(mean_value, make, meanDbl(m), kTol)
STAT_PARITY(mean_value_sentinel, make_sentinel, meanDbl(m), kTol)
STAT_PARITY(min_value, make, minDbl(m), kExact)
STAT_PARITY(max_value_sentinel, make_sentinel, maxDbl(m), kExact)
STAT_PARITY(porosity_0, make_sentinel, porosityDbl(m, T(0)), kExact)
STAT_PARITY(porosity_55, make_sentinel, porosityDbl(m, T(55)), kExact)
STAT_PARITY(porosity_max, make_sentinel, porosityDbl(m, maxT(T)), kExact)
