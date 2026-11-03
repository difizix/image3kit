#include "utest.h"
#include "parity_harness.h"
#include "voxelImageI.h"
#include <algorithm>

namespace {
auto make = [](auto tag, int3 n) {
    using T = decltype(tag);
    auto img = vxhl::testing::labelled_volume<T>(n, 20, 0.3, 313, 3);
    if constexpr (std::is_floating_point_v<T>) {
        long long i = 0;
        for (auto& v : img.data_) v += T(0.25) * T(i++ % 4);
    }
    return img;
};
//! Run f on a DeviceImage copy of m (the result stays on the device until host()).
template <typename M, typename F> void on_dev(M& m, F f) {
    vxhl::DeviceImage<std::decay_t<decltype(m(0, 0, 0))>> d(m);
    f(d);
    m = d.host();
}
}  // namespace

// Sizes are clamped to each shape (parity_shapes has 1- and 2-voxel-thin images).
#define CROP_ARGS(m) int3(std::min(1, m.nx() - 1), 0, std::min(2, m.nz() - 1)), int3(m.nx(), std::max(1, m.ny() - 1), m.nz())

VXHL_PARITY_TEST(grow_box_3, make, [](auto& m) { m.growBox(3); }, [](auto& m) { vxhl::growBox(m, 3); })
VXHL_PARITY_TEST(grow_box_1_dev, make, [](auto& m) { m.growBox(1); }, [](auto& m) { on_dev(m, [](auto& d) { vxhl::growBox(d, 1); }); })
VXHL_PARITY_TEST(shrink_box_1, make, [](auto& m) { if (m.nx() > 2 && m.ny() > 2 && m.nz() > 2) m.shrinkBox(1); },
                 [](auto& m) { if (m.nx() > 2 && m.ny() > 2 && m.nz() > 2) vxhl::shrinkBox(m, 1); })
VXHL_PARITY_TEST(crop, make, [](auto& m) { m.cropD(CROP_ARGS(m)); }, [](auto& m) { vxhl::cropD(m, CROP_ARGS(m)); })
VXHL_PARITY_TEST(crop_empty_layers, make, [](auto& m) { m.cropD(CROP_ARGS(m), 2, 7); },
                 [](auto& m) { vxhl::cropD(m, CROP_ARGS(m), 2, 7); })
VXHL_PARITY_TEST(crop_empty_layers_dev, make, [](auto& m) { m.cropD(CROP_ARGS(m), 1, 9); },
                 [](auto& m) { on_dev(m, [&](auto& d) { vxhl::cropD(d, CROP_ARGS(m), 1, 9); }); })
VXHL_PARITY_TEST(crop_to_from_end, make, [](auto& m) { m.cropD(int3(0, 0, 0), int3(0, 0, -0)); },
                 [](auto& m) { vxhl::cropD(m, int3(0, 0, 0), int3(0, 0, -0)); })

// X0 / dx follow voxlib, on every backend.
UTEST(VoxlideRemap, header) {
    const vxhl::Backend saved = vxhl::get_backend();
    for (vxhl::Backend b : vxhl::available_backends()) {
        vxhl::set_backend(b);
        VoxelImageT<unsigned char> a(int3(9, 8, 7), 3, dbl3(0.5, 2, 3), dbl3(1, 2, 3)), r = a;
        a.cropD(int3(1, 2, 3), int3(8, 7, 6), 2, 5);
        vxhl::cropD(r, int3(1, 2, 3), int3(8, 7, 6), 2, (unsigned char)5);
        EXPECT_TRUE(a.X0() == r.X0() && a.dx() == r.dx() && a.size3() == r.size3());
        a.growBox(4), vxhl::growBox(r, 4);
        EXPECT_TRUE(a.X0() == r.X0() && a.size3() == r.size3());
    }
    vxhl::set_backend(saved);
}
// growBox → median → shrinkBox on a DeviceImage on the GPU: one upload, one download.
UTEST(VoxlideRemap, device_chain_transfers) {
    const auto bs = vxhl::available_backends();
    if (std::find(bs.begin(), bs.end(), vxhl::Backend::Gpu) == bs.end()) return;
    const vxhl::Backend saved = vxhl::get_backend();
    auto img = make((unsigned char)0, int3(20, 21, 22));
    auto ref = img;
    vxhl::set_backend(vxhl::Backend::Voxlib);
    ref.growBox(2), ref = vxhl::median(ref, 1), ref.shrinkBox(2);
    vxhl::set_backend(vxhl::Backend::Gpu);
    vxhl::DeviceImage<unsigned char> d(img);
    vxhl::growBox(d, 2), vxhl::median(d, 1), vxhl::shrinkBox(d, 2);
    EXPECT_EQ(0LL, vxhl::testing::count_mismatches(ref, d.host(), "chain"));
    EXPECT_EQ(1LL, d.transfers().up);
    EXPECT_EQ(1LL, d.transfers().down);
    vxhl::set_backend(saved);
}

VXHL_PARITY_TEST(redirect_z, make, [](auto& m) { m.rotate('z'); }, [](auto& m) { vxhl::redirect(m, 'z'); })
VXHL_PARITY_TEST(redirect_y, make, [](auto& m) { m.rotate('y'); }, [](auto& m) { vxhl::redirect(m, 'y'); })
VXHL_PARITY_TEST(redirect_flip, make, [](auto& m) { m.rotate('-'); }, [](auto& m) { vxhl::redirect(m, '-'); })
VXHL_PARITY_TEST(redirect_z_dev, make, [](auto& m) { m.rotate('Z'); }, [](auto& m) { on_dev(m, [](auto& d) { vxhl::redirect(d, 'Z'); }); })
UTEST(VoxlideRemap, redirect_header) {
    const vxhl::Backend saved = vxhl::get_backend();
    for (vxhl::Backend b : vxhl::available_backends()) {
        vxhl::set_backend(b);
        for (char d : {'z', 'y'}) {
            VoxelImageT<unsigned char> a(int3(9, 8, 7), 3, dbl3(0.5, 2, 3), dbl3(1, 2, 3)), r = a;
            a.rotate(d), vxhl::redirect(r, d);
            EXPECT_TRUE(a.X0() == r.X0() && a.dx() == r.dx() && a.size3() == r.size3());
        }
    }
    vxhl::set_backend(saved);
}

#define RESAMPLE_PARITY(NAME, FN, F)                                                                 \
    VXHL_PARITY_TEST(NAME, make, [](auto& m) { m = ::FN(m, F); }, [](auto& m) { m = vxhl::FN(m, F); }) \
    VXHL_PARITY_TEST(NAME##_dev, make, [](auto& m) { m = ::FN(m, F); },                               \
                     [](auto& m) { on_dev(m, [](auto& d) { d = vxhl::FN(d, F); }); })
RESAMPLE_PARITY(resample_up_2, resampleMean, 0.5)
RESAMPLE_PARITY(resample_up_3, resampleMean, 0.3)
RESAMPLE_PARITY(resample_mean_2, resampleMean, 2.0)
RESAMPLE_PARITY(resample_mean_3, resampleMean, 3.0)
RESAMPLE_PARITY(reslice_up, resliceZ, 0.4)
RESAMPLE_PARITY(reslice_up_07, resliceZ, 0.7)
RESAMPLE_PARITY(reslice_down_2, resliceZ, 2.0)
UTEST(VoxlideRemap, resample_header) {
    const vxhl::Backend saved = vxhl::get_backend();
    for (vxhl::Backend b : vxhl::available_backends()) {
        vxhl::set_backend(b);
        for (double f : {0.5, 2.0}) {
            const VoxelImageT<unsigned char> a(int3(9, 8, 7), 3, dbl3(0.5, 2, 3), dbl3(1, 2, 3));
            const auto r = ::resampleMean(a, f), g = vxhl::resampleMean(a, f);
            EXPECT_TRUE(r.X0() == g.X0() && r.dx() == g.dx() && r.size3() == g.size3());
            const auto rz = ::resliceZ(a, f), gz = vxhl::resliceZ(a, f);
            EXPECT_TRUE(rz.X0() == gz.X0() && rz.dx() == gz.dx() && rz.size3() == gz.size3());
        }
    }
    vxhl::set_backend(saved);
}
