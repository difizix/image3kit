#include "utest.h"
#include "parity_harness.h"
#include "voxelImageI.h"

namespace {
// Values 0..4 on a grid with non-trivial voxel size and origin, so the geometry (index + 0.5)·dx + X0 matters.
auto make = [](auto tag, int3 n) {
    using T = decltype(tag);
    auto img = vxhl::testing::labelled_volume<T>(n, 5, 0.5, 951, 3);
    for (auto& v : img.data_) v = T((int(v) - 5) / 10);
    img.dxCh() = dbl3(0.5, 0.7, 0.9);
    img.X0Ch() = dbl3(-1.0, 2.0, 0.25);
    return img;
};
template <typename M> using VoxT = std::decay_t<decltype(std::declval<M&>()(0, 0, 0))>;
}  // namespace

// The shapes in physical coordinates (the grid spans about 0..8 x 2..18 x 0..28 for the larger test shapes)
#define PAINT_PARITY(NAME, SHAPE, VAL, FN)                                                                            \
    VXHL_PARITY_TEST(NAME, make, [](auto& m) { const auto sh = SHAPE; ::VoxLib::FN(m, sh); },                       \
                     [](auto& m) { const auto sh = SHAPE; vxhl::FN(m, sh); })
#define SPHERE(VAL) VoxLib::sphere(dbl3(3.0, 8.0, 10.0), 4.5, VAL)
#define CYL(VAL) VoxLib::cylinder(dbl3(-1.0, 6.0, 5.0), dbl3(9.0, 11.0, 16.0), 2.7, VAL)
#define CUBE(VAL) VoxLib::kube(dbl3(0.5, 4.0, 3.0), dbl3(3.3, 9.9, 11.1), VAL)
PAINT_PARITY(paint_sphere, SPHERE(7), 7, paint)
PAINT_PARITY(paint_sphere_negative_value, SPHERE(-5), -5, paint)
PAINT_PARITY(paint_add_sphere, SPHERE(3), 3, paintAdd)
PAINT_PARITY(paint_add_sphere_wrapping, SPHERE(250), 250, paintAdd)
PAINT_PARITY(paint_cylinder, CYL(9), 9, paint)
PAINT_PARITY(paint_add_cylinder, CYL(-2), -2, paintAdd)
PAINT_PARITY(paint_cube, CUBE(6), 6, paint)
PAINT_PARITY(paint_add_cube, CUBE(1), 1, paintAdd)
// shapes sharing the image: painted one after another (the Python idiom)
VXHL_PARITY_TEST(paint_sequence, make,
                 [](auto& m) { ::VoxLib::paint(m, SPHERE(7)); ::VoxLib::paintAdd(m, CYL(2)); ::VoxLib::paint(m, CUBE(1)); },
                 [](auto& m) { vxhl::paint(m, SPHERE(7)); vxhl::paintAdd(m, CYL(2)); vxhl::paint(m, CUBE(1)); })

// triangular: CPU only (Halide's Vulkan backend has no f64 cos/sin), so loop over the other backends
UTEST(VoxlideParity, paint_triangular_cpu) {
    long long bad = 0;
    const auto saved = vxhl::get_backend();
    auto tri = [](int val) { return VoxLib::triangular(dbl3(0.0, 14.0, 6.0), 6.0, 6.5, 9.0, 12.0, 3.2, 1.6, val); };
    for (int3 n : vxhl::testing::parity_shapes()) {
        const auto input = make((unsigned char)0, n);
        auto ref = input, ref_add = input;
        ::VoxLib::paint(ref, tri(7));
        ::VoxLib::paintAdd(ref_add, tri(3));
        for (auto b : vxhl::available_backends()) {
            if (b == vxhl::Backend::Gpu) continue;
            vxhl::set_backend(b);
            auto got = input, got_add = input;
            vxhl::paint(got, tri(7));
            vxhl::paintAdd(got_add, tri(3));
            bad += vxhl::testing::count_mismatches(ref, got, std::string("paint triangular ") + vxhl::backend_name(b));
            bad += vxhl::testing::count_mismatches(ref_add, got_add, std::string("paint_add triangular ") + vxhl::backend_name(b));
        }
    }
    vxhl::set_backend(saved);
    EXPECT_EQ(0LL, bad);
    // the shape does something on the 64^3 grid
    auto img = make((unsigned char)0, int3(64, 64, 64)), orig = img;
    ::VoxLib::paint(img, tri(7));
    EXPECT_GT(vxhl::testing::count_mismatches(orig, img, "triangular paints something"), 100LL);
}
