// Halide port of ::FaceMedGrowToFrom(vImg, lblTo, lblFrm, ndif), voxlib/voxelImageI.h:1094
// (Python median06_grow, which swaps the order of from/to). Semantics reproduced exactly:
//  - only voxels >= 1 from every image face and equal to lblFrm are examined; the rest are copied;
//  - Jacobi: the 6 face neighbours come from a copy of the input;
//  - neiSumTo/neiSum1 = number of face neighbours equal to lblTo / lblFrm; the voxel becomes lblTo
//    if neiSumTo != 0 && neiSumTo > neiSum1 + ndif.
// voxlib prints the number of changes (it returns void); the Halide path doesn't.

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class Median06Grow : public Generator<Median06Grow> {
public:
    VXHL_STENCIL_INPUTS
    Input<Expr> lblTo{"lblTo"}, lblFrm{"lblFrm"};  // voxel type (VALUES)
    Input<int32_t> ndif{"ndif"};
    Output<Buffer<void, 3>> output{"output"};

    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        Func in = BoundaryConditions::repeat_edge(input);
        const Expr vv = in(x, y, z);
        Expr nTo = Expr(int32_t(0)), nFrm = Expr(int32_t(0));
        for (Expr e : {in(x - 1, y, z), in(x + 1, y, z), in(x, y - 1, z), in(x, y + 1, z), in(x, y, z - 1), in(x, y, z + 1)}) {
            nTo += select(e == lblTo, 1, 0);
            nFrm += select(e == lblFrm, 1, 0);
        }
        const Expr change = vxhl_gen::interior(x, y, z, gnx, gny, gnz, 1) && vv == lblFrm && nTo != 0 && nTo > nFrm + ndif;
        output(x, y, z) = select(change, lblTo, vv);
    }

    void schedule() { vxhl_gen::schedule_stencil(output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(Median06Grow, median06Grow)
