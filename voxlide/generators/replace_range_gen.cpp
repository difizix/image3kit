// Halide port of ::replaceRange(VoxelImageT<T>&, T minvi, T maxvi, T midvi), voxlib/voxelImageI.h:1510
// (Python replace_range / range_to): every voxel v with min <= v <= max becomes mid (pointwise;
// NaN voxels are never in range).

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class ReplaceRange : public Generator<ReplaceRange> {
public:
    VXHL_STENCIL_INPUTS
    Input<Expr> lo{"lo"}, hi{"hi"}, mid{"mid"};  // voxel type (VALUES)
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        const Expr v = input(x, y, z);
        output(x, y, z) = select(lo <= v && v <= hi, mid, v);
    }
    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(ReplaceRange, replaceRange)
