#include "utest.h"
#include "parity_harness.h"
#include "voxelImageI.h"

namespace {
// 0/1 blocks with salt-and-pepper holes
auto make01 = [](auto tag, int3 n) {
    auto img = vxhl::testing::labelled_volume<decltype(tag)>(n, 2, 0.12, 961, 6);
    for (auto& v : img.data_) v = decltype(tag)(v > 5 ? 1 : 0);
    return img;
};
// three labels (5, 15, 25) with noise for delense032
auto make3 = [](auto tag, int3 n) { return vxhl::testing::labelled_volume<decltype(tag)>(n, 3, 0.3, 962, 3); };
bool has_dim1(int3 s) { return s.x < 2 || s.y < 2 || s.z < 2; }  // growPore reads out of range there (voxlib)
}  // namespace

// ---- fill_holes (maxHoleRadius 0/1: the first two stages only; 2, 3: all four)
#define FILL(NAME, R)                                                                                  \
    VXHL_PARITY_TEST(NAME, make01, [](auto& m) { if (!has_dim1(m.size3())) m.fillHoles(R); },          \
                     [](auto& m) { if (!has_dim1(m.size3())) vxhl::fillHoles(m, R); })
FILL(fill_holes_r1, 1)
FILL(fill_holes_r2, 2)
FILL(fill_holes_r3, 3)

// ---- delense032 (grows the image by 2 first, so any shape works)
#define DELENSE(NAME, NI, A0, A1, L0, L1)                                                                                \
    VXHL_PARITY_TEST(NAME, make3, [](auto& m) { ::_delense032(m, NI, A0, A1, L0, L1); },                                  \
                     [](auto& m) { vxhl::_delense032(m, NI, A0, A1, L0, L1); })
DELENSE(delense032_a, 1, 14, 14, 5, 15)
DELENSE(delense032_two_iterations, 2, 10, 18, 15, 5)
DELENSE(delense032_no_iterations, 0, 12, 12, 5, 25)
DELENSE(delense032_extreme_thresholds, 1, 30, 1, 5, 15)

// the chains on a resident image: same result, and (on the GPU) the whole chain is one upload + one download
UTEST(VoxlideDevice, chains_on_device_image) {
    const auto img = make01((unsigned char)0, int3(48, 40, 36));
    auto ref = img;
    ref.fillHoles(2);
    const auto saved = vxhl::get_backend();
    for (auto b : vxhl::available_backends()) {
        if (b == vxhl::Backend::Voxlib) continue;
        vxhl::set_backend(b);
        vxhl::DeviceImage<unsigned char> d(img);
        vxhl::fillHoles(d, 2);
        EXPECT_EQ(0LL, vxhl::testing::count_mismatches(ref, d.host(), std::string("fillHoles on DeviceImage, ") + vxhl::backend_name(b)));
        if (b == vxhl::Backend::Gpu) {
            // up: self + tmp (zero image); down: only the final host()
            EXPECT_LE(d.transfers().up, 2LL);
            EXPECT_EQ(1LL, d.transfers().down);
        }
    }
    vxhl::set_backend(saved);
}
