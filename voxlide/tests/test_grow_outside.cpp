#include "utest.h"
#include "parity_harness.h"
#include "voxelImageProcess.h"

namespace {
// A sample (value 7 and 9) in a vo=0 surround with holes, runs of vo reaching each face, and noise.
auto make = [](auto tag, int3 n) {
    using T = decltype(tag);
    VoxelImageT<T> img(n, T(0));
    std::mt19937 rng(77);
    std::uniform_real_distribution<double> u(0, 1);
    for (int k = 0; k < n.z; ++k)
        for (int j = 0; j < n.y; ++j)
            for (int i = 0; i < n.x; ++i) {
                const bool inside = i >= n.x / 5 && i < n.x - n.x / 4 && j >= n.y / 6 && j < n.y - n.y / 5 && k >= n.z / 4 && k < n.z - n.z / 6;
                double r = u(rng);
                img(i, j, k) = T(inside ? (r < 0.25 ? 0 : (r < 0.6 ? 7 : 9)) : (r < 0.93 ? 0 : 7));
            }
    return img;
};
}  // namespace

VXHL_PARITY_TEST(grow_outside_value_5, make, [](auto& m) { VoxLib::growOutSideValue(m, 0, 2, 5); }, [](auto& m) { vxhl::growOutSideValue(m, 0, 2, 5); })
VXHL_PARITY_TEST(grow_outside_value_0, make, [](auto& m) { VoxLib::growOutSideValue(m, 0, 2, 0); }, [](auto& m) { vxhl::growOutSideValue(m, 0, 2, 0); })
VXHL_PARITY_TEST(grow_outside_value_other, make, [](auto& m) { VoxLib::growOutSideValue(m, 7, 3, 2); }, [](auto& m) { vxhl::growOutSideValue(m, 7, 3, 2); })
#define GROW_RANGE_TEST(NAME, MID, LO, HI)                                                                       \
    VXHL_PARITY_TEST(NAME, make, [](auto& m) { using T = std::decay_t<decltype(m(0, 0, 0))>; ::grow(m, T(MID), T(LO), T(HI)); }, \
                     [](auto& m) { using T = std::decay_t<decltype(m(0, 0, 0))>; vxhl::grow(m, T(MID), T(LO), T(HI)); })
GROW_RANGE_TEST(grow_range, 7, 0, 2)
GROW_RANGE_TEST(grow_range_wide, 9, 3, 8)
// A DeviceImage on the GPU stays on the device through the scans and the grows.
UTEST(VoxlideGrowOutside, device_chain) {
    for (vxhl::Backend b : vxhl::available_backends()) {
        vxhl::set_backend(b);
        auto img = make((unsigned char)0, int3(30, 31, 29));
        auto ref = img;
        VoxLib::growOutSideValue(ref, 0, 2, 3);
        vxhl::DeviceImage<unsigned char> d(img);
        vxhl::growOutSideValue(d, 0, 2, 3);
        EXPECT_EQ(0LL, vxhl::testing::count_mismatches(ref, d.host(), "device"));
        if (b == vxhl::Backend::Gpu) { EXPECT_EQ(1LL, d.transfers().up); EXPECT_EQ(1LL, d.transfers().down); }
    }
    vxhl::set_backend(vxhl::Backend::Auto);
}
