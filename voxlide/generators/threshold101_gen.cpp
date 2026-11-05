// Halide port of VoxelImageT<T>::threshold101(Bgn, End), voxlib/voxelImageI.h:1420 (Python
// threshold101; free-function form ::threshold101): v -> (v < Bgn || End < v) ? 1 : 0.

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class Threshold101 : public Generator<Threshold101> {
public:
    VXHL_STENCIL_INPUTS
    Input<Expr> bgn{"bgn"}, end{"end"};  // voxel type (VALUES)
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};
    void generate() {
        const Expr v = input(x, y, z);
        output(x, y, z) = select(v < bgn || end < v, cast(input.type(), 1), cast(input.type(), 0));
    }
    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(Threshold101, threshold101)
