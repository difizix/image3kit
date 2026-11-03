#include "utest.h"
#include "parity_harness.h"
#include "voxelImageML.h"

namespace {
// Labels spread over the type's range: large gradients (uint32 wrap on u16/i32, T wrap on u8).
auto make = [](auto tag, int3 n) {
    using T = decltype(tag);
    auto img = vxhl::testing::labelled_volume<T>(n, 20, 0.4, 333, 3);
    const double scale = std::is_same_v<T, unsigned char> ? 1.0 : std::is_same_v<T, unsigned short> ? 300.0 : 1e5;
    for (auto& v : img.data_) v = T(double(v) * scale);
    return img;
};
}  // namespace

// No kernel for float images (voxlib UB): an explicit backend throws for them, so skip them.
#define INT_ONLY if constexpr (std::is_floating_point_v<std::decay_t<decltype(m(0, 0, 0))>>) return;
#define MAGGRAD_PARITY(NAME, D)                                                                   \
    VXHL_PARITY_TEST(NAME, make, [](auto& m) { INT_ONLY m = ::magGradient(m, D); },               \
                     [](auto& m) { INT_ONLY m = vxhl::magGradient(m, D); })

MAGGRAD_PARITY(mag_gradient_1, 1.0)
MAGGRAD_PARITY(mag_gradient_small, 0.01)
MAGGRAD_PARITY(mag_gradient_large, 37.5)

// The in-place DeviceImage overload.
VXHL_PARITY_TEST(mag_gradient_device, make, [](auto& m) { INT_ONLY m = ::magGradient(m, 2.0); },
                 [](auto& m) { INT_ONLY vxhl::DeviceImage d(m); vxhl::magGradient(d, 2.0); m = d.host(); })

// sqrt(s) on or within ulps of a half-integer k − 0.5, where rounding decides the result (the GPU
// kernel has no f64 sqrt and settles these with exact tests): an x-ramp has s = 4·d inside, and
// d = (k − 0.5)²/4 nudged by −2..+2 ulps.
UTEST(VoxlideParity, mag_gradient_half_integers) {
    auto ramp = [](auto tag, int3 n) {
        VoxelImageT<decltype(tag)> img(n, decltype(tag)(0));
        for (int k = 0; k < n.z; ++k)
            for (int j = 0; j < n.y; ++j)
                for (int i = 0; i < n.x; ++i) img(i, j, k) = decltype(tag)(i);
        return img;
    };
    long long bad = 0;
    for (double kk : {1.0, 2.0, 3.0, 50.0, 1000.0, 40000.0})
        for (int ulps = -2; ulps <= 2; ++ulps) {
            double d = (kk - 0.5) * (kk - 0.5) / 4;
            for (int u = 0; u < std::abs(ulps); ++u) d = std::nextafter(d, ulps > 0 ? 1e300 : 0.0);
            bad += vxhl::testing::parity_sweep(
                "mag_gradient_half_integers", ramp, [d](auto& m) { INT_ONLY m = ::magGradient(m, d); },
                [d](auto& m) { INT_ONLY m = vxhl::magGradient(m, d); });
        }
    EXPECT_EQ(0LL, bad);
}

// voxlib saturates at maxT(T): a step of 200 levels with diffuse 10 has sqrt(200²·10) ≈ 632, which
// used to wrap (u8: 632 mod 256 = 120) because T() was applied before the clamp.
UTEST(VoxlideParity, mag_gradient_saturates) {
    VoxelImageT<unsigned char> img(int3(8, 8, 8), 0);
    for (int k = 0; k < 8; ++k)
        for (int j = 0; j < 8; ++j)
            for (int i = 4; i < 8; ++i) img(i, j, k) = 200;
    const auto g = ::magGradient(img, 10.0);
    EXPECT_EQ(255, int(g(3, 4, 4)));
    EXPECT_EQ(255, int(g(4, 4, 4)));
    EXPECT_EQ(0, int(g(1, 4, 4)));
    VoxelImageT<unsigned short> big(int3(8, 8, 8), 0);  // 4·65000² > 2^32: the uint32 sum overflowed before
    for (int k = 0; k < 8; ++k) for (int j = 0; j < 8; ++j) for (int i = 4; i < 8; ++i) big(i, j, k) = 65000;
    EXPECT_EQ(65535, int(::magGradient(big, 4.0)(3, 4, 4)));
}
