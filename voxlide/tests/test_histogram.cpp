#include "utest.h"
#include "parity_harness.h"
#include "voxelImageML.h"

namespace {
// Values 5..195 in blocks plus noise; float images get fractional parts, some NaN.
auto make = [](auto tag, int3 n) {
    using T = decltype(tag);
    auto img = vxhl::testing::labelled_volume<T>(n, 20, 0.3, 909, 3);
    if constexpr (std::is_floating_point_v<T>) {
        long long i = 0;
        for (auto& v : img.data_) {
            v += T(0.37) * T(i % 3);
            if (i % 29 == 0) v = std::numeric_limits<T>::quiet_NaN();
            ++i;
        }
    }
    return img;
};
// Number of bins where vxhl::histogram differs from voxlib's (plus a size mismatch).
template <typename M>
double diff(const M& m, double shift, double span, int nBins, double lo, double hi) {
    HistBins bins;
    bins.shift = shift, bins.span = span, bins.nBins = nBins;
    const auto ref = ::histogram(m, bins, lo, hi);
    const auto got = vxhl::histogram(m, shift, span, nBins, lo, hi);
    double bad = ref.size() != got.size();
    for (std::size_t b = 0; b < std::min(ref.size(), got.size()); ++b) bad += ref[b] != got[b];
    return bad;
}
}  // namespace

#define HIST_PARITY(NAME, ...) \
    VXHL_SCALAR_PARITY_TEST(NAME, make, [](const auto&) { return 0.0; }, [](const auto& m) { return diff(m, __VA_ARGS__); }, 0)

HIST_PARITY(histogram_256, 0, 256, 256, 0, 256)
HIST_PARITY(histogram_shifted, 5, 190, 190, 5, 195)       // one bin per value
HIST_PARITY(histogram_coarse, 0, 200, 16, 0, 1e9)         // many values per bin; values past the span clamp into the last bin
HIST_PARITY(histogram_range_edges, 0, 256, 256, 25, 105)  // v == lo counts, v == hi does not
HIST_PARITY(histogram_clamp_low, 50, 100, 10, -1e9, 1e9) // values below the span clamp into bin 0
HIST_PARITY(histogram_fractional, 0.3, 150.7, 37, 0, 1e9) // bin edges not on integers
HIST_PARITY(histogram_empty_range, 0, 256, 256, 100, 50)
