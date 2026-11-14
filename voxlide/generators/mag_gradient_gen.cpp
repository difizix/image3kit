// Halide port of ::magGradient(const VoxelImageT<T>&, double diffuseLSqr), voxlib/voxelImageML.h:219
// (segment2), in place. Semantics reproduced exactly:
//  - every voxel; neighbours from a growBox(1) copy (= repeat_edge);
//  - central differences, squared and summed in voxlib's f32_t (generator param ftype; f64 is
//    exact for integer voxels);
//  - T(min(sqrt(sum·diffuseLSqr) + 0.5, maxT)): the clamp comes before the conversion (the old
//    code converted first, so the strongest edges wrapped); the conversion truncates (via int32).
// Float images are not ported (their T(double) conversion rounds, not truncates).
// GPU: sqrt is corrected to the correctly rounded root (vxhl_gen::sqrtF).

#include <cmath>
#include "vxhl_gen.h"

namespace {

using namespace Halide;

class MagGradient : public Generator<MagGradient> {
public:
    GeneratorParam<Type> ftype{"ftype", Float(64)};  // voxlib f32_t (F32T)
    VXHL_STENCIL_INPUTS
    Input<Expr> diffuse{"diffuse"};  // ftype (FLOATS)
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        Func in = BoundaryConditions::repeat_edge(input);
        const Type ft = ftype;
        auto d = [&](Expr a, Expr b) { return cast(ft, a) - cast(ft, b); };
        const Expr gx = d(in(x + 1, y, z), in(x - 1, y, z)), gy = d(in(x, y + 1, z), in(x, y - 1, z)),
                   gz = d(in(x, y, z + 1), in(x, y, z - 1));
        const Expr sd = (gx * gx + gy * gy + gz * gz) * diffuse;
        const Expr r = vxhl_gen::sqrtF(sd, get_target());  // fl(sqrt(sd))
        // voxlib: T(min(r + 0.5, f32_t(maxT))); via int32 (exact for f64: maxT <= 2^31-1)
        const Expr k = cast<int32_t>(min(r + vxhl_gen::fconst(ft, 0.5), cast(ft, input.type().max())));
        output(x, y, z) = cast(input.type(), k);
    }
    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(MagGradient, magGradient)
