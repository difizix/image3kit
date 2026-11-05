// Halide port of VoxelImageT<T>::growLabel(vl), voxlib/voxelImageI.h:835 (Python grow_label;
// free-function form ::growLabel). voxlib scatters: every voxel equal to vl sets its 6 face
// neighbours that differ from vl to vl, all reading a copy. As a gather: a voxel becomes vl if it or
// any face neighbour is vl. (At the image faces voxlib's padded copy repeats the edge layer, so
// a missing neighbour equals the voxel itself: repeat_edge.)

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class GrowLabel : public Generator<GrowLabel> {
public:
    VXHL_STENCIL_INPUTS
    Input<Expr> vl{"vl"};  // voxel type (VALUES)
    Output<Buffer<void, 3>> output{"output"};

    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        Func in = BoundaryConditions::repeat_edge(input);
        const Expr vv = in(x, y, z);
        const Expr any = vv == vl || in(x - 1, y, z) == vl || in(x + 1, y, z) == vl || in(x, y - 1, z) == vl ||
                         in(x, y + 1, z) == vl || in(x, y, z - 1) == vl || in(x, y, z + 1) == vl;
        output(x, y, z) = select(any, vl, vv);
    }

    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(GrowLabel, growLabel)
