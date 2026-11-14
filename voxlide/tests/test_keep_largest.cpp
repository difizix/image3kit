// vxhl::keepLargest / keepLargestvv (gather-only connected components) against voxlib voxelRegions.h,
// and voxlib's labelling against an independent flood fill.
#include "utest.h"
#include "parity_harness.h"
#include "voxelRegions.h"
#include <algorithm>
#include <queue>
#include <random>

namespace {
using vxhl::testing::labelled_volume;

//! Random 0/1 sites near the 3D site-percolation threshold (~0.31): many components with tortuous
//! shapes (long scan chains, many ties among the small ones), values 10 and 20.
auto sites = [](auto tag, int3 n) {
    using T = decltype(tag);
    VoxelImageT<T> img(n, T(10));
    std::mt19937 rng(91 + n.x * 7 + n.z);
    std::bernoulli_distribution on(0.33);
    for (auto& v : img.data_) v = on(rng) ? T(20) : T(10);
    return img;
};
auto blocks = [](auto tag, int3 n) { return labelled_volume<decltype(tag)>(n, 3, 0.3, 313, 3); };

template <typename T> VoxelImageT<T> grad_of(const VoxelImageT<T>& m) {  // a second, independent image
    VoxelImageT<T> g = sites(T(0), m.size3());
    std::mt19937 rng(5);
    for (auto& v : g.data_) if (rng() % 7 == 0) v = T(15);
    return g;
}
template <typename T> vxhl::DeviceImage<T> on_device(const VoxelImageT<T>& m) { return vxhl::DeviceImage<T>(m); }

//! 6-connected component sizes by flood fill: the largest one's voxel set, the first in raster order on ties.
template <typename T> std::vector<char> largest_by_flood(const VoxelImageT<T>& img, T lo, T hi) {
    const int3 n = img.size3();
    const long long nv = (long long)n.x * n.y * n.z;
    std::vector<int> comp(nv, -1);
    std::vector<long long> size;
    auto in = [&](long long i) { return lo <= img.data_[i] && img.data_[i] <= hi; };
    for (long long s = 0; s < nv; ++s) {
        if (!in(s) || comp[s] >= 0) continue;
        const int c = int(size.size());
        size.push_back(0);
        std::queue<long long> q;
        q.push(s), comp[s] = c;
        while (!q.empty()) {
            const long long v = q.front();
            q.pop(), ++size[c];
            const int x = int(v % n.x), y = int((v / n.x) % n.y), z = int(v / ((long long)n.x * n.y));
            const int d[6][3] = {{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}};
            for (auto& e : d) {
                const int a = x + e[0], b = y + e[1], cz = z + e[2];
                if (a < 0 || b < 0 || cz < 0 || a >= n.x || b >= n.y || cz >= n.z) continue;
                const long long u = a + (long long)n.x * (b + (long long)n.y * cz);
                if (in(u) && comp[u] < 0) comp[u] = c, q.push(u);
            }
        }
    }
    std::vector<char> keep(nv, 0);
    if (size.empty()) return keep;
    const int best = int(std::max_element(size.begin(), size.end()) - size.begin());
    for (long long i = 0; i < nv; ++i) keep[i] = comp[i] == best;
    return keep;
}
}  // namespace

// keepLargestvv (Python keep_largest): the components of the image's own range
VXHL_PARITY_TEST(keep_largest_vv_sites, sites, [](auto& m) { keepLargestvv(m, std::decay_t<decltype(m(0, 0, 0))>(15), std::decay_t<decltype(m(0, 0, 0))>(25)); },
                 [](auto& m) { vxhl::keepLargestvv(m, std::decay_t<decltype(m(0, 0, 0))>(15), std::decay_t<decltype(m(0, 0, 0))>(25)); })
VXHL_PARITY_TEST(keep_largest_vv_blocks, blocks, [](auto& m) { keepLargestvv(m, std::decay_t<decltype(m(0, 0, 0))>(0), std::decay_t<decltype(m(0, 0, 0))>(15)); },
                 [](auto& m) { vxhl::keepLargestvv(m, std::decay_t<decltype(m(0, 0, 0))>(0), std::decay_t<decltype(m(0, 0, 0))>(15)); })
// keepLargest (meanWide): components of a second image, counted in the first
VXHL_PARITY_TEST(keep_largest_grad, blocks,
                 [](auto& m) { using T = std::decay_t<decltype(m(0, 0, 0))>; keepLargest(m, grad_of(m), T(0), T(15), T(12), T(25)); },
                 [](auto& m) { using T = std::decay_t<decltype(m(0, 0, 0))>; vxhl::keepLargest(m, grad_of(m), T(0), T(15), T(12), T(25)); })
VXHL_PARITY_TEST(keep_largest_grad_dev, blocks,
                 [](auto& m) { using T = std::decay_t<decltype(m(0, 0, 0))>; keepLargest(m, grad_of(m), T(0), T(15), T(12), T(25)); },
                 [](auto& m) {
                     using T = std::decay_t<decltype(m(0, 0, 0))>;
                     auto d = on_device(m);
                     const auto g = on_device(grad_of(m));
                     vxhl::keepLargest(d, g, T(0), T(15), T(12), T(25));
                     m = d.host();
                 })
// nothing counted (every count 0) and nothing labelled: voxlib keeps no component
UTEST(VoxlideKeepLargest, no_counts_no_labels) {
    const vxhl::Backend saved = vxhl::get_backend();
    for (vxhl::Backend b : vxhl::available_backends()) {
        vxhl::set_backend(b);
        for (int3 lim : {int3(100, 101, 0), int3(0, 50, 1)}) {  // (minvv, maxvv, labels in range?)
            const auto img = blocks((unsigned short)0, int3(17, 23, 31));
            const auto g = grad_of(img);
            const unsigned short lo = lim.z ? 100 : 12, hi = lim.z ? 101 : 25;
            auto r = img, t = img;
            keepLargest(r, g, (unsigned short)lim.x, (unsigned short)lim.y, lo, hi);
            vxhl::keepLargest(t, g, (unsigned short)lim.x, (unsigned short)lim.y, lo, hi);
            EXPECT_TRUE(std::equal(r.data_.begin(), r.data_.end(), t.data_.begin()));
        }
    }
    vxhl::set_backend(saved);
}

// Two components of the same size: the first in raster order wins (voxlib's label order).
UTEST(VoxlideKeepLargest, tie_first_in_raster_order) {
    const vxhl::Backend saved = vxhl::get_backend();
    for (vxhl::Backend b : vxhl::available_backends()) {
        vxhl::set_backend(b);
        VoxelImageT<unsigned char> a(int3(9, 7, 5), 0);
        for (int i : {1, 2, 3}) a(i, 1, 1) = 20, a(i + 4, 5, 3) = 20;  // two 3-voxel rods
        a(0, 6, 0) = 20;                                                  // a smaller, earlier one
        auto r = a, g = a;
        keepLargestvv(r, (unsigned char)15, (unsigned char)25);
        vxhl::keepLargestvv(g, (unsigned char)15, (unsigned char)25);
        EXPECT_TRUE(std::equal(r.data_.begin(), r.data_.end(), g.data_.begin()));
        EXPECT_EQ(int(g(1, 1, 1)), 20);
        EXPECT_EQ(int(g(5, 5, 3)), 254);
    }
    vxhl::set_backend(saved);
}

// voxlib's labelling (and so the port) against a flood fill, on tortuous percolation clusters.
UTEST(VoxlideKeepLargest, matches_flood_fill) {
    const vxhl::Backend saved = vxhl::get_backend();
    for (int3 n : {int3(30, 31, 32), int3(64, 64, 64), int3(5, 80, 7)}) {
        const auto img = sites((unsigned char)0, n);
        const auto keep = largest_by_flood(img, (unsigned char)15, (unsigned char)25);
        for (vxhl::Backend b : vxhl::available_backends()) {
            vxhl::set_backend(b);
            auto g = img;
            vxhl::keepLargestvv(g, (unsigned char)15, (unsigned char)25);
            long long bad = 0;
            for (std::size_t i = 0; i < keep.size(); ++i)
                bad += img.data_[i] == 20 && ((g.data_[i] == 20) != bool(keep[i]));
            EXPECT_EQ(bad, 0LL);
        }
    }
    vxhl::set_backend(saved);
}

// On the GPU the two DeviceImages are uploaded once and nothing is downloaded.
UTEST(VoxlideKeepLargest, device_no_download) {
    const auto bs = vxhl::available_backends();
    if (std::find(bs.begin(), bs.end(), vxhl::Backend::Gpu) == bs.end()) return;
    const vxhl::Backend saved = vxhl::get_backend();
    vxhl::set_backend(vxhl::Backend::Gpu);
    const auto img = blocks((unsigned char)0, int3(40, 41, 42));
    auto d = on_device(img);
    const auto g = on_device(grad_of(img));
    vxhl::keepLargest(d, g, (unsigned char)0, (unsigned char)15, (unsigned char)12, (unsigned char)25);
    EXPECT_EQ(d.transfers().up, 1LL);
    EXPECT_EQ(d.transfers().down, 0LL);
    EXPECT_EQ(g.transfers().down, 0LL);
    vxhl::set_backend(saved);
}
