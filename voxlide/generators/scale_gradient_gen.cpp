// Halide port of ::scaleGradient(VoxelImageT<T>& grad, double avgGrad), voxlib/voxelImageML.h
// (segment2): v < avgGrad -> 10, else T(min(f32_t(maxT), v·10/avgGrad)) in voxlib's f32_t (generator
// param ftype; C++ truncation, via int32). Integer types only (segment2's), so float NaN semantics don't arise.

#include <limits>
#include "vxhl_gen.h"

namespace {

using namespace Halide;

class ScaleGradient : public Generator<ScaleGradient> {
public:
    GeneratorParam<Type> ftype{"ftype", Float(64)};  // voxlib f32_t (F32T)
    VXHL_STENCIL_INPUTS
    Input<Expr> avg{"avg"};  // ftype (FLOATS)
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        const Type t = input.type(), ft = ftype;
        const Expr v = cast(ft, input(x, y, z));
        const Expr dmax = cast(ft, t.max());
        const Expr scaled = min(dmax, vxhl_gen::divF(v * vxhl_gen::fconst(ft, 10.0), avg, get_target()));
        output(x, y, z) = select(v < avg, cast(t, 10), cast(t, cast<int32_t>(scaled)));
    }
    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(ScaleGradient, scaleGradient)
