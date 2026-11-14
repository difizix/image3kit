#include "utest.h"
#include "parity_harness.h"
#include "voxelImageI.h"

namespace {
// 0/1 noisy blocks (pore = 0).
auto make01 = [](auto tag, int3 n) {
    auto img = vxhl::testing::labelled_volume<decltype(tag)>(n, 2, 0.35, 818, 3);
    for (auto& v : img.data_) v = decltype(tag)(v > 5 ? 1 : 0);
    return img;
};
// Zeros plus values 1, 2, 3 and (signed types) -1: neighbour SUMS cancel to 0 for signed types.
auto make_signed = [](auto tag, int3 n) {
    using T = decltype(tag);
    auto img = vxhl::testing::labelled_volume<T>(n, 5, 0.5, 819, 2);
    for (auto& v : img.data_) {
        const int c = (int(v) - 5) / 10;  // 0..4
        v = T(std::is_signed_v<T> ? (c == 0 ? 0 : c == 1 ? 1 : c == 2 ? -1 : c == 3 ? 2 : 0) : (c == 4 ? 0 : c));
    }
    return img;
};
bool has_dim1(int3 s) { return s.x < 2 || s.y < 2 || s.z < 2; }  // growPore reads out of range there (voxlib)
}  // namespace

// growPore: dimensions >= 2 (voxlib reads out of range when one is 1)
VXHL_PARITY_TEST(grow_pore, make01, [](auto& m) { if (!has_dim1(m.size3())) m.growPore(); },
                 [](auto& m) { if (!has_dim1(m.size3())) vxhl::growPore(m); })
VXHL_PARITY_TEST(grow_pore_values, make_signed, [](auto& m) { if (!has_dim1(m.size3())) m.growPore(); },
                 [](auto& m) { if (!has_dim1(m.size3())) vxhl::growPore(m); })
// shrinkPore: every shape (its face loops only look tangentially)
VXHL_PARITY_TEST(shrink_pore, make01, [](auto& m) { m.shrinkPore(); }, [](auto& m) { vxhl::shrinkPore(m); })
VXHL_PARITY_TEST(shrink_pore_signed_sums, make_signed, [](auto& m) { m.shrinkPore(); }, [](auto& m) { vxhl::shrinkPore(m); })
// the idiom: shrink then grow, repeated (an opening)
VXHL_PARITY_TEST(pore_open_close, make01,
                 [](auto& m) { if (has_dim1(m.size3())) return; for (int i = 0; i < 2; ++i) m.shrinkPore(); for (int i = 0; i < 2; ++i) m.growPore(); },
                 [](auto& m) { if (has_dim1(m.size3())) return; for (int i = 0; i < 2; ++i) vxhl::shrinkPore(m); for (int i = 0; i < 2; ++i) vxhl::growPore(m); })

// median_x / _y / _z on label noise
auto make_noise = [](auto tag, int3 n) { return vxhl::testing::labelled_volume<decltype(tag)>(n, 9, 0.6, 820, 2); };
VXHL_PARITY_TEST(median_x, make_noise, [](auto& m) { m = ::medianx(m); }, [](auto& m) { m = vxhl::medianx(m); })
VXHL_PARITY_TEST(median_y, make_noise, [](auto& m) { m = ::mediany(m); }, [](auto& m) { m = vxhl::mediany(m); })
VXHL_PARITY_TEST(median_z, make_noise, [](auto& m) { m = ::medianz(m); }, [](auto& m) { m = vxhl::medianz(m); })
VXHL_PARITY_TEST(median_xyz_device, make_noise, [](auto& m) { m = ::medianz(::mediany(::medianx(m))); },
                 [](auto& m) { vxhl::DeviceImage d(m); vxhl::medianx(d); vxhl::mediany(d); vxhl::medianz(d); m = d.host(); })
