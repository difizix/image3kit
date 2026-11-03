// Halide port of ::magGradient(const VoxelImageT<T>&, double diffuseLSqr), voxlib/voxelImageML.h:219
// (segment2), in place. Semantics reproduced exactly:
//  - every voxel; neighbours from a growBox(1) copy (= repeat_edge);
//  - central differences as `unsigned int` (wrap), squared and summed in uint32 (wraps for
//    large u16/i32 gradients, as in voxlib);
//  - T(sqrt(double(sum)·diffuseLSqr) + 0.5): the conversion happens before voxlib's
//    min(…, maxT), so values above maxT(T) wrap (x86/gcc: double→int32, low bits kept). voxlib
//    UB, reproduced via int32 (exact while < 2^31).
// Float images are not ported (voxlib converts negative float differences to unsigned: UB).
// GPU: Halide's Vulkan backend has no f64 sqrt, so r = fl(sqrt(s)) (correctly rounded) is built
// from an estimate (f32 sqrt + 2 f64 Newton steps) and exact corrections: with the exact residual
// e = fma(−c, c, s), c is the correctly rounded root iff −c·u⁻ < e <= c·u (u, u⁻: the ulps above /
// below c; sqrt never hits a rounding midpoint). Then fl(r + 0.5) and truncation as voxlib.

#include <cmath>
#include "vxhl_gen.h"

namespace {

using namespace Halide;

class MagGradient : public Generator<MagGradient> {
public:
    VXHL_STENCIL_INPUTS
    Input<double> diffuse{"diffuse"};
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        Func in = BoundaryConditions::repeat_edge(input);
        auto d = [&](Expr a, Expr b) { return cast<uint32_t>(a) - cast<uint32_t>(b); };
        const Expr gx = d(in(x + 1, y, z), in(x - 1, y, z)), gy = d(in(x, y + 1, z), in(x, y - 1, z)),
                   gz = d(in(x, y, z + 1), in(x, y, z - 1));
        const Expr sum = gx * gx + gy * gy + gz * gz;
        const Expr sd = cast<double>(sum) * diffuse;
        const Expr half = Internal::make_const(Float(64), 0.5);
        Expr r;  // fl(sqrt(sd))
        if (!get_target().has_gpu_feature()) {
            r = sqrt(sd);
        } else {
            Expr c = cast<double>(sqrt(cast<float>(sd)));
            for (int i = 0; i < 2; ++i) c = select(c > 0, half * (c + sd / c), c);
            auto ulps = [&](Expr v) {  // {ulp above v, ulp below v}, for 1e-38 < v < 3e38
                Expr p = cast<double>(reinterpret<float>(reinterpret<uint32_t>(cast<float>(v)) & 0x7f800000u));
                p = select(p > v, p * half, p);  // 2^floor(log2 v): cast<float> may round up a binade
                const Expr u = p * Internal::make_const(Float(64), std::ldexp(1.0, -52));
                return std::make_pair(u, select(v == p, u * half, u));
            };
            for (int i = 0; i < 2; ++i) {
                const auto [u, um] = ulps(c);
                const Expr e = fma(-c, c, sd);
                c = select(e > c * u, c + u, select(e <= -(c * um), c - um, c));
            }
            r = select(sd > 0, c, Internal::make_const(Float(64), 0.0));
        }
        const Expr k = cast<int32_t>(r + half);  // C++ T(sqrt(..)+0.5), via int32 (exact while < 2^31)
        output(x, y, z) = cast(input.type(), k);
    }
    void schedule() { vxhl_gen::schedule_stencil(output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(MagGradient, magGradient)
