// Halide port of VoxelImageT<T>::PointMedian032(nAdj0, nAdj1, lbl0, lbl1), voxlib/voxelImageI.h:1052
// (Python median032; voxlib free-function form ::median032). Semantics reproduced exactly:
//  - only voxels >= 1 from every image face and equal to lbl0 or lbl1 are examined; the rest are copied;
//  - Jacobi: neighbours come from a copy of the input (voxlib's vxls);
//  - neiSum0/1 count the neighbours equal to lbl0/lbl1 over the 27-voxel cube (centre included)
//    plus the 6 face neighbours again;
//  - a lbl1 voxel becomes lbl0 if neiSum0 >= nAdj0 && neiSum0 > neiSum1; else a lbl0 voxel becomes lbl1
//    if neiSum1 >= nAdj1 && neiSum1 > neiSum0.
// voxlib prints the number of changes (it returns void); the Halide path doesn't.

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class Median032 : public Generator<Median032> {
public:
    VXHL_STENCIL_INPUTS
    Input<int32_t> nAdj0{"nAdj0"}, nAdj1{"nAdj1"};
    Input<Expr> lbl0{"lbl0"}, lbl1{"lbl1"};  // voxel type (voxlide_add_op VALUES)
    Output<Buffer<void, 3>> output{"output"};

    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        Func in = BoundaryConditions::repeat_edge(input);
        const Expr vv = in(x, y, z);
        Expr n0 = Expr(int32_t(0)), n1 = Expr(int32_t(0));
        auto add = [&](Expr e) { n0 += select(e == lbl0, 1, 0); n1 += select(e == lbl1, 1, 0); };
        for (int dk = -1; dk <= 1; ++dk)
            for (int dj = -1; dj <= 1; ++dj)
                for (int di = -1; di <= 1; ++di) add(in(x + di, y + dj, z + dk));
        add(in(x - 1, y, z)); add(in(x + 1, y, z)); add(in(x, y - 1, z)); add(in(x, y + 1, z));
        add(in(x, y, z - 1)); add(in(x, y, z + 1));

        const Expr examine = vxhl_gen::interior(x, y, z, gnx, gny, gnz, 1) && (vv == lbl0 || vv == lbl1);
        const Expr to0 = n0 >= nAdj0 && n0 > n1 && vv == lbl1;
        const Expr to1 = n1 >= nAdj1 && n1 > n0 && vv == lbl0;
        output(x, y, z) = select(examine && to0, lbl0, select(examine && to1, lbl1, vv));
    }

    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(Median032, median032)
