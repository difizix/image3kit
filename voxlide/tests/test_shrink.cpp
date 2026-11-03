#include "utest.h"
#include "parity_harness.h"
#include "voxelImageML.h"

namespace {
// seged: 4 labels (5, 15, 25, 35) in blocks with noise; src: an unrelated image, 20 labels.
auto make = [](auto tag, int3 n) { return vxhl::testing::labelled_volume<decltype(tag)>(n, 4, 0.3, 808, 4); };
template <typename T> VoxelImageT<T> src_for(int3 n) { return vxhl::testing::labelled_volume<T>(n, 20, 0.5, 809, 3); }
}  // namespace

// The source image is a second, independent image (detail::Aux), sliced per slab with the halo.
#define SHRINK_PARITY(NAME, LO, HI)                                                                           \
    VXHL_PARITY_TEST(NAME, make,                                                                              \
                     [](auto& m) { using T = std::decay_t<decltype(m(0, 0, 0))>;                               \
                                   ::shrink(m, T(LO), T(HI), src_for<T>(m.size3())); },                       \
                     [](auto& m) { using T = std::decay_t<decltype(m(0, 0, 0))>;                               \
                                   vxhl::shrink(m, T(LO), T(HI), src_for<T>(m.size3())); })

SHRINK_PARITY(shrink_one_label, 25, 25)
SHRINK_PARITY(shrink_range, 15, 35)
SHRINK_PARITY(shrink_all, 0, 250)
SHRINK_PARITY(shrink_empty_range, 35, 15)

// A source image of another size is an error on explicit backends (never silently misread).
UTEST(VoxlideParity, shrink_size_mismatch_throws) {
    const auto saved = vxhl::get_backend();
    for (vxhl::Backend b : vxhl::available_backends()) {
        if (b == vxhl::Backend::Voxlib) continue;
        vxhl::set_backend(b);
        auto img = make((unsigned char)0, int3(9, 9, 9));
        const auto src = src_for<unsigned char>(int3(9, 9, 8));
        bool threw = false;
        try { vxhl::shrink(img, (unsigned char)5, (unsigned char)35, src); } catch (const std::exception&) { threw = true; }
        EXPECT_TRUE(threw);
    }
    vxhl::set_backend(saved);
}
