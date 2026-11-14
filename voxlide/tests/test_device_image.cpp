#include "utest.h"
#include "parity_harness.h"
#include "voxelImageML.h"
#include "voxelImageProcess.h"

// DeviceImage: chains of ported ops on one image (resident on the GPU when it fits in one piece)
// equal the same voxlib sequence on every backend and slab count, and on the GPU a chain costs
// one upload and one download.

namespace {
auto make = [](auto tag, int3 n) { return vxhl::testing::labelled_volume<decltype(tag)>(n, 6, 0.4, 909, 3); };
template <typename T> VoxelImageT<T> src_for(int3 n) { return vxhl::testing::labelled_volume<T>(n, 20, 0.5, 910, 3); }
template <typename T> using Img = VoxelImageT<T>;
}  // namespace

// mode26 → median ×3 → shrink (Aux) → mode6 (count) → smooth_bilateral (growBox: host round trip)
// → mode26: stencils, Snapshot, Aux and change counts on a resident image, and a host transition.
UTEST(VoxlideParity, device_image_chain) {
    long long refCount = 0, countBad = 0;
    const long long bad = vxhl::testing::parity_sweep(
        "device_image_chain", make,
        [&](auto& m) {
            using T = std::decay_t<decltype(m(0, 0, 0))>;
            ::mode26(m, 2);
            m = ::median(m, 3);
            ::shrink(m, T(15), T(25), src_for<T>(m.size3()));
            refCount = ::modeNSames(m, 2);
            VoxLib::smoothBilateral(m, 1, 2, 16.0, 0.1);
            ::mode26(m, 1);
        },
        [&](auto& m) {
            using T = std::decay_t<decltype(m(0, 0, 0))>;
            vxhl::DeviceImage<T> d(m);
            vxhl::mode26(d, 2);
            vxhl::median(d, 3);
            vxhl::shrink(d, T(15), T(25), vxhl::DeviceImage<T>(src_for<T>(m.size3())));
            if (vxhl::modeNSames(d, 2) != refCount) ++countBad;
            vxhl::smoothBilateral(d, 1, 2, 16.0, 0.1);
            vxhl::mode26(d, 1);
            m = d.host();
        });
    EXPECT_EQ(0LL, bad);
    EXPECT_EQ(0LL, countBad);
}

// Reductions read the resident (device) copy after GPU ops.
VXHL_SCALAR_PARITY_TEST(
    device_image_reduction, make,
    [](const auto& m) { auto c = m; ::mode26(c, 2); return ::varianceDbl(c, 0, 255) + ::maxDbl(c); },
    [](const auto& m) { vxhl::DeviceImage d(m); vxhl::mode26(d, 2); return vxhl::varianceDbl(d, 0, 255) + vxhl::maxDbl(d); },
    1e-10)

UTEST(VoxlideDevice, chain_transfers) {
    const auto backends = vxhl::available_backends();
    if (std::find(backends.begin(), backends.end(), vxhl::Backend::Gpu) == backends.end()) return;  // no GPU
    const auto saved = vxhl::get_backend();
    vxhl::set_backend(vxhl::Backend::Gpu);
    const auto img = make((unsigned char)0, int3(64, 64, 64));
    vxhl::DeviceImage<unsigned char> d(img);
    EXPECT_EQ(0LL, d.transfers().up);
    vxhl::mode26(d, 2);
    vxhl::median(d, 3);
    vxhl::shrink(d, (unsigned char)15, (unsigned char)25, src_for<unsigned char>(img.size3()));
    vxhl::modeNSames(d, 2);
    const double var = vxhl::varianceDbl(d, 0, 255);
    vxhl::mode26(d, 1);
    EXPECT_EQ(1LL, d.transfers().up);
    EXPECT_EQ(0LL, d.transfers().down);
    const VoxelImageT<unsigned char> h = d.host();
    d.host();  // already in sync: no second download
    EXPECT_EQ(1LL, d.transfers().down);
    d.host_mut()(1, 1, 1) = 0;  // modified on the host: the next GPU op uploads again
    vxhl::mode26(d, 1);
    EXPECT_EQ(2LL, d.transfers().up);

    // A DeviceImage as the Aux (source) image: read on the device, uploaded once, never downloaded.
    vxhl::DeviceImage<unsigned char> src(src_for<unsigned char>(img.size3()));
    vxhl::shrink(d, (unsigned char)15, (unsigned char)25, src);
    vxhl::shrink(d, (unsigned char)5, (unsigned char)15, src);
    EXPECT_EQ(1LL, src.transfers().up);
    EXPECT_EQ(0LL, src.transfers().down);
    EXPECT_EQ(2LL, d.transfers().up);
    vxhl::set_backend(saved);

    auto ref = img;  // and the results are the voxlib ones
    ::mode26(ref, 2);
    ref = ::median(ref, 3);
    ::shrink(ref, (unsigned char)15, (unsigned char)25, src_for<unsigned char>(img.size3()));
    ::modeNSames(ref, 2);
    EXPECT_NEAR(::varianceDbl(ref, 0, 255), var, 1e-10 * var);
    ::mode26(ref, 1);
    EXPECT_EQ(0LL, vxhl::testing::count_mismatches(ref, h, "chain_transfers"));
}
