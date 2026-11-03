#include "utest.h"
#include "parity_harness.h"
#include "voxelImageI.h"

namespace {
// Values 0..4 (many zeros) so truthiness, equality and arithmetic all matter.
auto make = [](auto tag, int3 n) {
    using T = decltype(tag);
    auto img = vxhl::testing::labelled_volume<T>(n, 5, 0.5, 901, 3);
    for (auto& v : img.data_) v = T((int(v) - 5) / 10);
    return img;
};
template <typename T> VoxelImageT<T> other_for(int3 n) {
    auto img = vxhl::testing::labelled_volume<T>(n, 4, 0.6, 902, 2);
    for (auto& v : img.data_) v = T((int(v) - 5) / 10);
    return img;
}
// 8-bit-range values for the arithmetic operate tests: 0, 40, 100, 250 ...
template <typename T> VoxelImageT<T> wide(int3 n, uint32_t seed) {
    auto img = vxhl::testing::labelled_volume<T>(n, 12, 0.5, seed, 2);
    for (auto& v : img.data_) v = T(int(v) * 2 % 256);  // 10..250
    return img;
}
template <typename M> using VoxT = std::decay_t<decltype(std::declval<M&>()(0, 0, 0))>;
}  // namespace

// ---- and_/or_/xor_/not_/+=/-= with a second image (VoxelImageT and DeviceImage sources)
#define PAIR(NAME, FN)                                                                                                   \
    VXHL_PARITY_TEST(NAME, make, [](auto& m) { ::FN(m, other_for<VoxT<decltype(m)>>(m.size3())); },                       \
                     [](auto& m) { vxhl::FN(m, other_for<VoxT<decltype(m)>>(m.size3())); })
PAIR(logic_and, and_)
PAIR(logic_or, or_)
PAIR(logic_xor, xor_)
PAIR(logic_not, not_)
PAIR(image_add, addEq)  // wraps for 8/16-bit
PAIR(image_sub, subEq)
VXHL_PARITY_TEST(image_ops_device_second_image, make,
                 [](auto& m) { using T = VoxT<decltype(m)>; ::and_(m, other_for<T>(m.size3())); ::addEq(m, other_for<T>(m.size3())); },
                 [](auto& m) { using T = VoxT<decltype(m)>; vxhl::and_(m, vxhl::DeviceImage<T>(other_for<T>(m.size3())));
                               vxhl::DeviceImage<T> d(m); vxhl::addEq(d, vxhl::DeviceImage<T>(other_for<T>(m.size3()))); m = d.host(); })
UTEST(VoxlideParity, image_ops_size_mismatch_throws) {
    VoxelImageT<unsigned char> a(int3(4, 4, 4), 1), b(int3(4, 4, 5), 1);
    for (auto be : vxhl::available_backends()) {
        vxhl::set_backend(be);
        bool t1 = false, t2 = false;
        try { vxhl::and_(a, b); } catch (const std::runtime_error&) { t1 = true; }
        try { vxhl::addEq(a, b); } catch (const std::runtime_error&) { t2 = true; }
        EXPECT_TRUE(t1 && t2);
    }
    vxhl::set_backend(vxhl::Backend::Auto);
}

// ---- threshold101 (all types)
VXHL_PARITY_TEST(threshold101_mid, make, [](auto& m) { using T = VoxT<decltype(m)>; ::threshold101(m, T(1), T(2)); },
                 [](auto& m) { using T = VoxT<decltype(m)>; vxhl::threshold101(m, T(1), T(2)); })
VXHL_PARITY_TEST(threshold101_empty, make, [](auto& m) { using T = VoxT<decltype(m)>; ::threshold101(m, T(3), T(1)); },
                 [](auto& m) { using T = VoxT<decltype(m)>; vxhl::threshold101(m, T(3), T(1)); })

// ---- circle_out (both directions, centre outside the image too)
#define CIRCLE(NAME, X0, Y0, R, DIR)                                                                                    \
    VXHL_PARITY_TEST(NAME, make, [](auto& m) { using T = VoxT<decltype(m)>; ::circleOut(m, X0, Y0, R, DIR, T(9)); },    \
                     [](auto& m) { using T = VoxT<decltype(m)>; vxhl::circleOut(m, X0, Y0, R, DIR, T(9)); })
CIRCLE(circle_out_z, 8, 11, 7, 'z')
CIRCLE(circle_out_x, 11, 15, 9, 'x')
CIRCLE(circle_out_far_centre, -20, 40, 30, 'z')
CIRCLE(circle_out_r0, 3, 3, 0, 'x')
UTEST(VoxlideParity, circle_out_bad_direction_throws) {
    VoxelImageT<unsigned char> a(int3(4, 4, 4), 1);
    for (auto be : vxhl::available_backends()) {
        vxhl::set_backend(be);
        bool threw = false;
        try { vxhl::circleOut(a, 1, 1, 1, 'q', (unsigned char)9); } catch (const std::exception&) { threw = true; }
        EXPECT_TRUE(threw);
    }
    vxhl::set_backend(vxhl::Backend::Auto);
}

// ---- operate (8/16-bit only)
#define SMALL(m, STMT) if constexpr (sizeof(VoxT<decltype(m)>) <= 2) { STMT; }  // 8/16-bit images only
#define OPU(NAME, OPR)                                                                                                  \
    VXHL_PARITY_TEST(NAME, [](auto tag, int3 n) { return wide<decltype(tag)>(n, 903); },                               \
                     [](auto& m) { SMALL(m, ::operateUnary(m, OPR)) }, [](auto& m) { SMALL(m, vxhl::operateUnary(m, OPR)) })
OPU(operate_unary_bitnot, "~")
OPU(operate_unary_lognot, "!")
OPU(operate_unary_neg, "-")
#define OPI(NAME, OPR, SHIFT)                                                                                           \
    VXHL_PARITY_TEST(NAME, [](auto tag, int3 n) { return wide<decltype(tag)>(n, 904); },                               \
                     [](auto& m) { SMALL(m, ::operateImg(m, OPR, wide<VoxT<decltype(m)>>(m.size3(), 905), SHIFT)) },   \
                     [](auto& m) { SMALL(m, vxhl::operateImg(m, OPR, wide<VoxT<decltype(m)>>(m.size3(), 905), SHIFT)) })
OPI(operate_img_add, "+", 0.0)
OPI(operate_img_add_shift, "+", 130.0)
OPI(operate_img_sub, "-", 0.0)
OPI(operate_img_sub_shift, "-", 40.5)
OPI(operate_img_mul, "*", 0.0)
OPI(operate_img_mul_shift, "*", 0.25)
OPI(operate_img_div, "/", 0.0)
OPI(operate_img_div_shift, "/", -0.5)
OPI(operate_img_and, "&", 0.0)
OPI(operate_img_or, "|", 0.0)
OPI(operate_img_mod, "%", 0.0)
#define OPS(NAME, OPR, VAL)                                                                                             \
    VXHL_PARITY_TEST(NAME, [](auto tag, int3 n) { return wide<decltype(tag)>(n, 906); },                               \
                     [](auto& m) { SMALL(m, ::operateScalar(m, OPR, VAL)) }, [](auto& m) { SMALL(m, vxhl::operateScalar(m, OPR, VAL)) })
OPS(operate_scalar_set, "=", 77.0)
OPS(operate_scalar_set_clamped, "=", 1e9)
OPS(operate_scalar_add, "+", 90.0)
OPS(operate_scalar_add_neg, "+", -120.0)
OPS(operate_scalar_sub, "-", 33.7)
OPS(operate_scalar_mul, "*", 1.9)
OPS(operate_scalar_div, "/", 3.0)
OPS(operate_scalar_div_zero, "/", 0.0)
OPS(operate_scalar_and, "&", 60.0)
OPS(operate_scalar_or, "|", 5.0)
OPS(operate_scalar_mod, "%", 7.0)
OPS(operate_scalar_mod_zero, "%", 0.0)
OPS(operate_scalar_lower_bound, "b", 120.0)
OPS(operate_scalar_upper_bound, "e", 100.0)
UTEST(VoxlideParity, operate_bad_operations_throw) {
    VoxelImageT<unsigned char> a(int3(4, 4, 4), 1), b(int3(4, 4, 4), 1), c(int3(4, 4, 5), 1);
    for (auto be : vxhl::available_backends()) {
        vxhl::set_backend(be);
        int n = 0;
        try { vxhl::operateUnary(a, "?"); } catch (const std::runtime_error&) { ++n; }
        try { vxhl::operateUnary(a, ""); } catch (const std::runtime_error&) { ++n; }
        try { vxhl::operateScalar(a, "?", 1.0); } catch (const std::runtime_error&) { ++n; }
        try { vxhl::operateImg(a, "?", b, 0.0); } catch (const std::runtime_error&) { ++n; }
        try { vxhl::operateImg(a, "+", c, 0.0); } catch (const std::runtime_error&) { ++n; }
        EXPECT_EQ(5, n);
    }
    vxhl::set_backend(vxhl::Backend::Auto);
}

// ---- map_from on the same grid; scale > 0 applies T(v*scale + shift) to every voxel
#define MAPF(NAME, VMIN, VMAX, SCALE, SHIFT)                                                                            \
    VXHL_PARITY_TEST(NAME, make,                                                                                        \
        [](auto& m) { using T = VoxT<decltype(m)>; ::mapToFrom(m, other_for<T>(m.size3()), T(VMIN), T(VMAX), SCALE, SHIFT); }, \
        [](auto& m) { using T = VoxT<decltype(m)>; vxhl::mapToFrom(m, other_for<T>(m.size3()), T(VMIN), T(VMAX), SCALE, SHIFT); })
MAPF(map_from_replace, 1, 2, 0.0, 0.5)
MAPF(map_from_replace_scale, 0, 1, 3.0, 0.5)
MAPF(map_from_scale_only, 9, 9, 2.0, 1.25)
// a source on another grid (different size) runs voxlib on every backend
UTEST(VoxlideParity, map_from_other_grid_uses_voxlib) {
    VoxelImageT<unsigned char> a(int3(8, 8, 8), 5), b(int3(6, 8, 8), 7);
    auto ref = a;
    ::mapToFrom(ref, b, (unsigned char)0, (unsigned char)9, 0.0, 0.5);
    for (auto be : vxhl::available_backends()) {
        vxhl::set_backend(be);
        auto got = a;
        vxhl::mapToFrom(got, b, (unsigned char)0, (unsigned char)9, 0.0, 0.5);
        EXPECT_EQ(0LL, vxhl::testing::count_mismatches(ref, got, "map_from_other_grid"));
    }
    vxhl::set_backend(vxhl::Backend::Auto);
}

// Exhaustive u8 division: every (m, d) pair. Exact quotients (103/103, 196/98) must stay exact: the GPU's
// native f64 division returned 0.99999.. for some, which truncated to 0 (vxhl_gen::div64 corrects it).
UTEST(VoxlideParity, operate_division_exhaustive_u8) {
    const int3 n(256, 255, 1);
    VoxelImageT<unsigned char> m(n, 0), d(n, 0);
    for (int j = 0; j < 255; ++j) for (int i = 0; i < 256; ++i) { m(i, j, 0) = (unsigned char)i; d(i, j, 0) = (unsigned char)(j + 1); }
    for (const std::string op : {"/"}) for (double shift : {0.0, 0.25, -0.5}) {
        auto ref = m;
        ::operateImg(ref, op, d, shift);
        for (auto be : vxhl::available_backends()) {
            vxhl::set_backend(be);
            auto got = m;
            vxhl::operateImg(got, op, d, shift);
            EXPECT_EQ(0LL, vxhl::testing::count_mismatches(ref, got, std::string("operate / ") + vxhl::backend_name(be) + " shift " + std::to_string(shift)));
        }
    }
    // and with a scalar: m / val for scalars that divide many values exactly
    for (double val : {3.0, 7.0, 0.5, 103.0, 255.0}) {
        auto ref = m;
        ::operateScalar(ref, "/", val);
        for (auto be : vxhl::available_backends()) {
            vxhl::set_backend(be);
            auto got = m;
            vxhl::operateScalar(got, "/", val);
            EXPECT_EQ(0LL, vxhl::testing::count_mismatches(ref, got, std::string("operateScalar / ") + vxhl::backend_name(be) + " " + std::to_string(val)));
        }
    }
    vxhl::set_backend(vxhl::Backend::Auto);
}
