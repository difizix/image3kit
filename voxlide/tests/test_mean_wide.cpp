#include <filesystem>
#include "utest.h"
#include "parity_harness.h"
#include "voxelImageProcess.h"
#include "voxelImageML.h"

namespace {
namespace fs = std::filesystem;
// A smooth intensity ramp (a "lens artifact") on two phases, plus noise: what mean_wide is for.
template <typename T> VoxelImageT<T> make_ramp(int3 n, unsigned seed) {
    VoxelImageT<T> img(n, T(0));
    std::mt19937 rng(seed);
    std::normal_distribution<double> noise(0, 3);
    for (int k = 0; k < n.z; ++k)
        for (int j = 0; j < n.y; ++j)
            for (int i = 0; i < n.x; ++i) {
                const bool solid = ((i / 5) + (j / 6) + (k / 4)) % 3 == 0;
                const double ramp = 20.0 * i / n.x;
                img(i, j, k) = T(std::clamp((solid ? 150.0 : 60.0) + ramp + noise(rng), 3.0, 240.0));
            }
    return img;
}
//! The dump*.raw files go to a scratch directory.
struct InScratchDir {
    fs::path prev = fs::current_path();
    InScratchDir() {
        const fs::path d = fs::temp_directory_path() / "vxhl_mean_wide_test";
        fs::create_directories(d);
        fs::current_path(d);
    }
    ~InScratchDir() { fs::current_path(prev); }
};
template <typename T> long long run_case(int3 n, int nW, int avg, int nItrs, const char* label) {
    long long bad = 0;
    const vxhl::Backend saved = vxhl::get_backend();
    const VoxelImageT<T> input = make_ramp<T>(n, 11);
    VoxelImageT<T> expect = input;
    VoxLib::meanWide(expect, nW, 4, avg, 20, nItrs, "");
    {  // the test must exercise something: the iterations change many voxels
        long long changed = 0;
        for (std::size_t i = 0; i < expect.data_.size(); ++i) changed += expect.data_[i] != input.data_[i];
        if (changed < 100) { std::cout << label << ": only " << changed << " voxels change\n"; ++bad; }
    }
    for (vxhl::Backend b : vxhl::available_backends()) {
        vxhl::set_backend(b);
        for (int slabs : b == vxhl::Backend::Voxlib ? std::vector<int>{1} : std::vector<int>{1, 3}) {
            vxhl::set_max_slab_bytes(slabs == 1 ? 0 : (input.data_.size() * sizeof(T) * 2 / slabs));
            const std::string tag = std::string(label) + " " + vxhl::backend_name(b) + " slabs=" + std::to_string(slabs) + " nW=" + std::to_string(nW);
            VoxelImageT<T> got = input;
            vxhl::meanWide(got, nW, 4, avg, 20, nItrs, "");
            bad += vxhl::testing::count_mismatches(expect, got, tag);
            vxhl::DeviceImage<T> d(input);
            vxhl::meanWide(d, nW, 4, avg, 20, nItrs, "");
            bad += vxhl::testing::count_mismatches(expect, d.host(), tag + " (DeviceImage)");
        }
    }
    vxhl::set_max_slab_bytes(0);
    vxhl::set_backend(saved);
    return bad;
}
}  // namespace

UTEST(VoxlideMeanWide, u8) {
    InScratchDir d;
    EXPECT_EQ(0LL, run_case<unsigned char>(int3(26, 27, 28), 2, 100, 3, "u8 avg=100"));
    EXPECT_EQ(0LL, run_case<unsigned char>(int3(31, 20, 33), 1, 0, 4, "u8 avg=otsu"));
    EXPECT_EQ(0LL, run_case<unsigned char>(int3(20, 22, 21), 3, 100, 5, "u8 nW=3"));
}
UTEST(VoxlideMeanWide, u16) {
    InScratchDir d;
    EXPECT_EQ(0LL, run_case<unsigned short>(int3(26, 27, 28), 2, 100, 3, "u16"));
}
UTEST(VoxlideMeanWide, i32) {
    InScratchDir d;
    EXPECT_EQ(0LL, run_case<int>(int3(24, 25, 26), 2, 100, 3, "i32"));
}
// The pieces on their own.
namespace {
template <typename T> long long pieces(const char* label) {
    long long bad = 0;
    for (vxhl::Backend b : vxhl::available_backends()) {
        vxhl::set_backend(b);
        VoxelImageT<T> in = make_ramp<T>(int3(17, 23, 31), 5), expect = in;
        // padEdges + mean are internal; check them through meanWide's two stages by running the whole thing with 0 iterations
        VoxLib::meanWide(expect, 2, 4, 100, 20, 0, "");
        VoxelImageT<T> got = in;
        vxhl::meanWide(got, 2, 4, 100, 20, 0, "");
        bad += vxhl::testing::count_mismatches(expect, got, std::string(label) + " prep only " + vxhl::backend_name(b));
    }
    return bad;
}
}  // namespace
UTEST(VoxlideMeanWide, prep_only) {
    InScratchDir d;
    EXPECT_EQ(0LL, pieces<unsigned char>("u8"));
    EXPECT_EQ(0LL, pieces<unsigned short>("u16"));
    vxhl::set_backend(vxhl::Backend::Auto);
}

// ---- the building blocks, each against voxlib on random data (every type, backend, slab count)
namespace {
// Random values 0..199 (noisy: neighbouring voxels differ), at least 13 per axis (padEdges' assumption n >= 10).
auto make_rand = [](auto tag, int3 n) {
    using T = decltype(tag);
    n = int3(std::max(n.x, 13), std::max(n.y, 13), std::max(n.z, 13));
    VoxelImageT<T> img(n, T(0), dbl3(0.5, 0.5, 0.5), dbl3(1, 2, 3));
    std::mt19937 rng(321);
    std::uniform_int_distribution<int> u(0, 199);
    for (auto& v : img.data_) v = T(u(rng));
    return img;
};
template <typename M> auto other_of(const M& m, unsigned seed) {
    M o = m;
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> u(0, 199);
    for (auto& v : o.data_) v = std::decay_t<decltype(v)>(u(rng));
    return o;
}
}  // namespace
namespace {
auto pad_ref = [](auto& m) {
    const int3 n = m.size3();
    for (int i : {1, 2, 3}) {
        m.replaceyLayer(n[1] - 1 - i, 4), m.replaceyLayer(i, n[1] - 5);
        m.replacexLayer(n[0] - 1 - i, 4), m.replacexLayer(i, n[0] - 5);
        m.setLayer(n[2] - 1 - i, &m(0, 0, 4)), m.setLayer(i, &m(0, 0, n[2] - 5));
    }
};
}  // namespace
VXHL_PARITY_TEST(pad_edges, make_rand, pad_ref, [](auto& m) { vxhl::padEdges(m); })
VXHL_PARITY_TEST(mean_filter, make_rand, [](auto& m) { m = ::mean(m); }, [](auto& m) { vxhl::meanFilter(m); })
namespace {
auto above_ref = [](auto& m) {
    using T = std::decay_t<decltype(m(0, 0, 0))>;
    const auto g = other_of(m, 1);
    for (std::size_t i = 0; i < m.data_.size(); ++i) if (g.data_[i] > T(120)) m.data_[i] = T(7);
};
auto above_test = [](auto& m) {
    using T = std::decay_t<decltype(m(0, 0, 0))>;
    const auto g = other_of(m, 1);
    vxhl::setWhereAbove(m, g, T(120), T(7));
};
auto step_ref = [](auto& m, int nW, int lo, int hi, int gt) {
    using T = std::decay_t<decltype(m(0, 0, 0))>;
    auto g = other_of(m, 2);
    m = ::meanWide(m, nW, T(lo), T(hi), g, T(gt));
};
auto step_test = [](auto& m, int nW, int lo, int hi, int gt) {
    using T = std::decay_t<decltype(m(0, 0, 0))>;
    const auto g = other_of(m, 2);
    vxhl::meanWideStep(m, nW, T(lo), T(hi), g, T(gt));
};
}  // namespace
VXHL_PARITY_TEST(set_where_above, make_rand, above_ref, above_test)
#define STEP_TEST(NAME, NW, LO, HI, GT) \
    VXHL_PARITY_TEST(NAME, make_rand, [](auto& m) { step_ref(m, NW, LO, HI, GT); }, [](auto& m) { step_test(m, NW, LO, HI, GT); })
STEP_TEST(mean_wide_step_1, 1, 60, 140, 150)
STEP_TEST(mean_wide_step_2, 2, 80, 120, 100)
STEP_TEST(mean_wide_step_3, 3, 40, 160, 199)
STEP_TEST(mean_wide_step_wide_range, 2, 0, 199, 199)
