// Halide port of ::scaleGradient(VoxelImageT<T>& grad, double avgGrad), voxlib/voxelImageML.h
// (segment2): v < avgGrad -> 10, else T(min(double(maxT), v·10./avgGrad)) (C++ truncation,
// via int32). Integer types only (segment2's), so float NaN semantics don't arise.

#include <limits>
#include "vxhl_gen.h"

namespace {

using namespace Halide;

class ScaleGradient : public Generator<ScaleGradient> {
public:
    VXHL_STENCIL_INPUTS
    Input<double> avg{"avg"};
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        const Type t = input.type();
        const Expr v = cast<double>(input(x, y, z));
        const Expr dmax = cast<double>(t.max());
        const Expr scaled = min(dmax, v * Internal::make_const(Float(64), 10.0) / avg);
        output(x, y, z) = select(v < avg, cast(t, 10), cast(t, cast<int32_t>(scaled)));
    }
    void schedule() { vxhl_gen::schedule_stencil(output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(ScaleGradient, scaleGradient)
