#include "utest.h"
#include "voxelImageML.h"
#include <random>

// Otsu on 8- and 16-bit images with the voxel values mapped to 256 bins (shift and scale) and the
// thresholds mapped back: scaling the field must scale the thresholds.

namespace {
// Two noisy phases (about 60 and 160 on a 0..255 scale) in blocks: bimodal.
template <typename T> VoxelImageT<T> phases(int3 n, long long scale, uint32_t seed) {
    VoxelImageT<T> img(n, T(0));
    std::mt19937 rng(seed);
    std::normal_distribution<double> noise(0.0, 12.0);
    for (int k = 0; k < n.z; ++k)
        for (int j = 0; j < n.y; ++j)
            for (int i = 0; i < n.x; ++i) {
                const double v = ((i / 6 + j / 5 + k / 4) % 2 ? 160.0 : 60.0) + noise(rng);
                img(i, j, k) = T(std::lround(std::clamp(v, 1.0, 254.0)) * scale);
            }
    return img;
}
template <typename T> VoxelImageT<T> scaled(const VoxelImageT<T>& f, long long a, long long shift = 0) {
    VoxelImageT<T> g = f;
    for (auto& v : g.data_) v = T(v * a + shift);
    return g;
}
}  // namespace

// Range [0, 256): 256 one-value bins for f, and 256 bins of width `a` for a·f: the same histogram.
UTEST(Otsu, scaled_field_gives_scaled_thresholds_u16) {
    const auto f = phases<unsigned short>(int3(40, 36, 32), 1, 5);
    const auto bf = otsuBins(0, 256, true);
    ASSERT_EQ(256, bf.nBins);
    const auto tf = otsu_minAvgThresholdAvgMax(f, 0, 256);
    ASSERT_TRUE(tf[2] > 90 && tf[2] < 130);  // between the phases
    for (long long a : {2, 3, 5, 7, 100, 255, 256}) {  // 255·256 = 65280: still a u16
        const auto g = scaled(f, a);
        const auto bg = otsuBins(0, 256 * a, true);
        ASSERT_EQ(256, bg.nBins);
        // identical histograms: the binning is invariant under scaling value, shift and span
        const auto hf = histogram(f, bf, 0, 256), hg = histogram(g, bg, 0, 256 * a);
        for (size_t i = 0; i < hf.size(); ++i) ASSERT_EQ(hf[i], hg[i]);
        const auto tg = otsu_minAvgThresholdAvgMax(g, 0, int(256 * a));
        for (int c = 0; c < 5; ++c) EXPECT_NEAR(a * tf[c], tg[c], 1e-9 * a * 256);  // min, mean_0, level, mean_1, max
        EXPECT_GT(tg[2], 90.0 * a);
        EXPECT_LT(tg[2], 130.0 * a);
    }
}

// Coarser bins (a 16-bit field binned to 256) find the same threshold as the full-resolution 8-bit
// field up to the bin width: here the 8-bit field times 200 against the 8-bit threshold.
UTEST(Otsu, u16_with_256_bins_matches_8bit_threshold_within_a_bin) {
    const auto f = phases<unsigned char>(int3(40, 36, 32), 1, 5);
    const auto g = scaled(phases<unsigned short>(int3(40, 36, 32), 1, 5), 200);
    const double tf = otsu_minAvgThresholdAvgMax(f, 1, 254)[2];
    const double tg = otsu_minAvgThresholdAvgMax(g, 200, 254 * 200)[2];
    EXPECT_NEAR(tf * 200, tg, 200.0 * 2);  // within about 2 gray levels of the 8-bit result
}

UTEST(Otsu, shifted_field_gives_shifted_thresholds_u16) {
    const auto f = phases<unsigned short>(int3(40, 36, 32), 100, 6);  // 100..25400
    const auto tf = otsu_minAvgThresholdAvgMax(f, 100, 25400);
    for (long long s : {1, 1000, 30000}) {
        const auto g = scaled(f, 1, s);
        const auto tg = otsu_minAvgThresholdAvgMax(g, int(100 + s), int(25400 + s));
        for (int c = 0; c < 5; ++c) EXPECT_NEAR(tf[c] + s, tg[c], 1e-9 * 65535);
    }
}

// An 8-bit range below 256 keeps one bin per value, so thresholds are voxel values as before, and
// the class means come back in voxel values (they were bin indices, off by the range's start).
UTEST(Otsu, u8_unit_bins_and_means_in_voxel_values) {
    const auto f = phases<unsigned char>(int3(40, 36, 32), 1, 7);
    const auto b = otsuBins(1, 254, true);
    EXPECT_EQ(253, b.nBins);
    EXPECT_NEAR(1.0, b.scale(), 0);
    const auto t = otsu_minAvgThresholdAvgMax(f, 1, 254);
    EXPECT_TRUE(t[1] > 50 && t[1] < 70);    // mean of the low phase ~60 (bin lower edges: ~60)
    EXPECT_TRUE(t[3] > 150 && t[3] < 170);  // mean of the high phase ~160
    EXPECT_TRUE(t[2] > t[1] && t[2] < t[3]);
    EXPECT_NEAR(1, t[0], 0);    // min and max: the range's ends
    EXPECT_NEAR(254, t[4], 0);
}

// The binning stays inside the histogram for every range (the old integer bin width indexed past
// its end for e.g. 1..60000).
UTEST(Otsu, bins_stay_in_range) {
    for (double hi : {255., 256., 257., 1000., 60000., 65534.})
        for (double lo : {0., 1., 7.}) {
            const auto b = otsuBins(lo, hi, true);
            EXPECT_LE(b.nBins, 256);
            for (double v : {lo, lo + 1, (lo + hi) / 2, hi - 1}) {
                EXPECT_GE(b.bin(v), 0);
                EXPECT_LT(b.bin(v), b.nBins);
            }
            EXPECT_EQ(b.nBins - 1 >= 0, true);
        }
    VoxelImageT<unsigned short> img(int3(9, 9, 9), 0);  // 1..60000: truncated-width indexing wrote past the end
    for (std::size_t i = 0; i < img.data_.size(); ++i) img.data_[i] = (unsigned short)(1 + (i * 7919) % 59999);
    const auto t = otsu_minAvgThresholdAvgMax(img, 1, 60000);
    EXPECT_TRUE(t[2] >= 1 && t[2] <= 60000);
}

// Independent oracle on an 8-bit image: brute-force the split that maximises the between-class
// variance (first maximum), and compare the threshold and both class means (in voxel values).
UTEST(Otsu, u8_matches_brute_force) {
    const auto f = phases<unsigned char>(int3(40, 36, 32), 1, 11);
    std::vector<double> h(256, 0.);
    for (auto v : f.data_) if (1 <= v && v < 254) h[v] += 1;
    double best = 0, bt = 1, n0 = 0, s0 = 0, n = 0, s = 0;
    for (int v = 1; v < 254; ++v) n += h[v], s += h[v] * v;
    double c = 0, cs = 0;
    for (int t = 1; t < 254; ++t) {
        c += h[t], cs += h[t] * t;
        if (c == 0 || c == n) continue;
        const double d = cs * n - s * c, sigma = d * d / (c * (n - c));
        if (sigma > best) best = sigma, bt = t, n0 = c, s0 = cs;
    }
    const auto t = otsu_minAvgThresholdAvgMax(f, 1, 254);
    EXPECT_NEAR(bt, t[2], 1e-9);
    EXPECT_NEAR(s0 / n0, t[1], 1e-9);
    EXPECT_NEAR((s - s0) / (n - n0), t[3], 1e-9);
}
