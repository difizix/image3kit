// Halide port of ::grow(vImg, midv, minv, maxv) (voxlib/voxelImageML.h): in the interior (one layer in from
// every face, forAllkji_1_) a voxel v != midv with minv <= v <= maxv becomes midv if any of its 26
// neighbours equals midv. voxlib reads a copy, so it is a gather.

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class GrowRange : public Generator<GrowRange> {
public:
    VXHL_STENCIL_INPUTS
    Input<Expr> midv{"midv"}, minv{"minv"}, maxv{"maxv"};  // voxel type (VALUES)
    Output<Buffer<void, 3>> output{"output"};

    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        Func in = BoundaryConditions::repeat_edge(input);
        const Expr vv = in(x, y, z);
        Expr anyMid = Internal::const_false();
        for (int dk = -1; dk <= 1; ++dk)
            for (int dj = -1; dj <= 1; ++dj)
                for (int di = -1; di <= 1; ++di) anyMid = anyMid || in(x + di, y + dj, z + dk) == midv;
        const Expr change = vxhl_gen::interior(x, y, z, gnx, gny, gnz, 1) && vv != midv && minv <= vv && vv <= maxv && anyMid;
        output(x, y, z) = select(change, midv, vv);
    }

    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(GrowRange, growRange)
