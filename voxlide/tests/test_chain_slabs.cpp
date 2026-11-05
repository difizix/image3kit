// vxhl::Chain: a chain of local ops run over z-slabs with a deep halo (Σ step halos) equals the same
// steps on the whole image (computed by voxlib), for every slab size the halo allows.
#include "utest.h"
#include "parity_harness.h"
#include "voxelImageI.h"
#include <algorithm>

namespace {
using vxhl::DeviceImage;
using vxhl::ZSlab;

//! median ×2 (halo 2), mode26 (1), [smooth_bilateral 1×1 (1): growBox, a host round trip], shrink with a
//! second image (1).
template <typename T> vxhl::Chain<T> test_chain(const VoxelImageT<T>& src, bool bilateral = true) {
    vxhl::Chain<T> c;
    c.then(2, [](DeviceImage<T>& d) { vxhl::median(d, 2); }).then(1, [](DeviceImage<T>& d) { vxhl::mode26(d, 1); });
    if (bilateral) c.then(1, [](DeviceImage<T>& d) { vxhl::smoothBilateral(d, 1, 1, 16.0, 0.1); });
    c.then(1, [&src](DeviceImage<T>& d, ZSlab s) { vxhl::shrink(d, T(10), T(30), vxhl::slab_of(src, s)); });
    return c;
}
template <typename T> VoxelImageT<T> source_of(int3 n) { return vxhl::testing::labelled_volume<T>(n, 4, 0.2, 77, 5); }

template <typename T> long long chain_sweep(const char* tname) {
    long long bad = 0;
    const auto saved = vxhl::get_backend();
    for (int3 n : {int3(17, 23, 31), int3(9, 5, 40), int3(64, 64, 64), int3(5, 6, 12)}) {
        const auto img = vxhl::testing::labelled_volume<T>(n, 4, 0.3, 991, 3);
        const auto src = source_of<T>(n);
        const auto chain = test_chain<T>(src);
        vxhl::set_backend(vxhl::Backend::Voxlib);  // reference: the steps on the whole image, voxlib
        vxhl::set_max_slab_bytes(0);
        auto ref = img;
        chain.run(ref);
        const long long layer = (long long)n.x * n.y * sizeof(T);
        for (auto b : vxhl::available_backends()) {
            vxhl::set_backend(b);
            for (int interior : {0, 1, 2, 7}) {  // 0: one piece; else slabs of `interior` layers + halo
                vxhl::set_max_slab_bytes(interior ? std::size_t((2 * chain.halo() + interior) * layer) : 0);
                auto got = img;
                chain.run(got);
                const long long m = vxhl::testing::count_mismatches(
                    ref, got, std::string("chain<") + tname + "> " + vxhl::backend_name(b) + " " + std::to_string(n.x) + "x" +
                                  std::to_string(n.y) + "x" + std::to_string(n.z) + " interior=" + std::to_string(interior) +
                                  vxhl::testing::match_note(b));
                if (vxhl::testing::must_match(b)) bad += m;
            }
        }
    }
    vxhl::set_max_slab_bytes(0);
    vxhl::set_backend(saved);
    return bad;
}
}  // namespace

UTEST(VoxlideChain, slabs_equal_whole_image) {
    EXPECT_EQ(0LL, chain_sweep<unsigned char>("u8"));
    EXPECT_EQ(0LL, chain_sweep<unsigned short>("u16"));
    EXPECT_EQ(0LL, chain_sweep<int>("i32"));
    EXPECT_EQ(0LL, chain_sweep<float>("f32"));
}

// Each slab is uploaded once and nothing is downloaded until the chain ends (GPU).
UTEST(VoxlideChain, one_upload_per_slab) {
    const auto bs = vxhl::available_backends();
    if (std::find(bs.begin(), bs.end(), vxhl::Backend::Gpu) == bs.end()) return;
    const auto saved = vxhl::get_backend();
    vxhl::set_backend(vxhl::Backend::Gpu);
    const int3 n(40, 30, 50);
    const auto img = vxhl::testing::labelled_volume<unsigned char>(n, 4, 0.3, 5, 3);
    const auto src = source_of<unsigned char>(n);
    auto chain = test_chain<unsigned char>(src, false);
    int slabs = 0;
    bool ok = true;
    chain.then(0, [&](DeviceImage<unsigned char>& d) {
        ++slabs;
        ok = ok && d.transfers().up == 1 && d.transfers().down == 0;
    });
    for (int interior : {0, 4}) {
        vxhl::set_max_slab_bytes(interior ? std::size_t((2 * chain.halo() + interior) * n.x * n.y) : 0);
        slabs = 0;
        auto got = img;
        chain.run(got);
        EXPECT_EQ(slabs, interior ? (n.z + interior - 1) / interior : 1);
    }
    EXPECT_TRUE(ok);
    vxhl::set_max_slab_bytes(0);
    vxhl::set_backend(saved);
}

UTEST(VoxlideChain, errors) {
    const int3 n(8, 8, 40);
    auto img = vxhl::testing::labelled_volume<unsigned char>(n, 3, 0.1, 3, 3);
    vxhl::Chain<unsigned char> deep;
    deep.then(10, [](DeviceImage<unsigned char>& d) { vxhl::median(d, 10); });
    vxhl::set_max_slab_bytes(std::size_t(15 * n.x * n.y));  // 15 layers < 2·10 + 1
    EXPECT_EXCEPTION(deep.run(img), std::invalid_argument);
    vxhl::set_max_slab_bytes(0);
    vxhl::Chain<unsigned char> grows;
    grows.then(1, [](DeviceImage<unsigned char>& d) { d.growBox(1); });
    EXPECT_EXCEPTION(grows.run(img), std::logic_error);
}
