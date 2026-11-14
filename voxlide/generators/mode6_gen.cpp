// Halide port of ::modeNSames(VoxelImageT<T>& vImage, short nSameNei), voxlib/voxelImageI.h:1285
// (Python mode6). Returns the number of changed voxels as a second output (ChangeCounts).
// Semantics reproduced exactly:
//  - only voxels >= 1 from every image face are changed (forAllkji_1_); the rest are copied;
//  - Jacobi: every voxel reads the input image (voxlib reads a copy);
//  - nSames counts the 6 face neighbours equal to the centre;
//  - a voxel is examined only if nSames <= nSameNei; the most frequent face-neighbour value
//    != centre wins if its count > nSames; ties go to the smaller value (std::map order +
//    std::max_element keeps the first max). Every changed voxel changes value, so
//    #changes = #voxels where output != input.
//  - voxlib dereferences end() when all 6 neighbours equal the centre and nSameNei >= 6 (UB);
//    this port leaves such voxels unchanged.

#include <vector>
#include "vxhl_gen.h"

namespace {

using namespace Halide;

class Mode6 : public Generator<Mode6> {
public:
    VXHL_STENCIL_INPUTS
    Input<int32_t> n_same_nei{"n_same_nei"};
    Output<Buffer<void, 3>> output{"output"};
    Output<Buffer<int32_t, 1>> changes{"changes"};

    Var x{"x"}, y{"y"}, z{"z"};
    vxhl_gen::ChangeCounts cc;

    void generate() {
        Func in = BoundaryConditions::repeat_edge(input);
        const Expr p = in(x, y, z);
        const std::vector<Expr> v = {in(x - 1, y, z), in(x + 1, y, z), in(x, y - 1, z),
                                     in(x, y + 1, z), in(x, y, z - 1), in(x, y, z + 1)};

        Expr nSames = Expr(int32_t(0));
        for (const Expr& e : v) nSames += select(e == p, 1, 0);

        // Count of v[m]'s value among v[m..5]: the full count at a value's first occurrence.
        Expr bestCnt = Expr(int32_t(0)), bestVal = p;
        for (size_t m = 0; m < v.size(); ++m) {
            Expr cnt = Expr(int32_t(1));
            for (size_t k = m + 1; k < v.size(); ++k) cnt += select(v[k] == v[m], 1, 0);
            cnt = select(v[m] == p, 0, cnt);
            Expr better = cnt > bestCnt || (cnt == bestCnt && cnt > 0 && v[m] < bestVal);
            bestCnt = select(better, cnt, bestCnt);
            bestVal = select(better, v[m], bestVal);
        }

        const Expr examine = vxhl_gen::interior(x, y, z, gnx, gny, gnz, 1) && nSames <= n_same_nei;
        output(x, y, z) = select(examine && bestCnt > nSames, bestVal, p);
        cc.define(changes, output, input, gnx, gny);
    }

    void schedule() {
        vxhl_gen::schedule_stencil(gnx, output, get_target(), input.type());
        cc.schedule(changes, get_target());
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(Mode6, mode6)
