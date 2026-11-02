// Halide port of ::mode26(VoxelImageT<T>& vImg, short minDif), voxlib/voxelImageML.h:75.
// Semantics reproduced exactly:
//  - only voxels >= 1 from every image face are changed (forAllkji_1_); the rest are copied;
//  - nSames counts the 27-voxel neighbourhood including the centre;
//  - a voxel is examined only if nSames + minDif/2 < 14, with C++ truncating division;
//  - the most frequent neighbour value != centre wins if its count > nSames + minDif;
//  - ties go to the smaller value (std::map order + std::max_element keeps the first max).

#include <vector>
#include "vxhl_gen.h"

namespace {

using namespace Halide;

class Mode26 : public Generator<Mode26> {
public:
    VXHL_STENCIL_INPUTS
    Input<int32_t> min_dif{"min_dif"};
    Output<Buffer<void, 3>> output{"output"};

    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        Func in = BoundaryConditions::repeat_edge(input);
        const Expr p = in(x, y, z);

        std::vector<Expr> v;  // the 26 neighbours
        for (int dk = -1; dk <= 1; ++dk)
            for (int dj = -1; dj <= 1; ++dj)
                for (int di = -1; di <= 1; ++di)
                    if (di || dj || dk) v.push_back(in(x + di, y + dj, z + dk));

        Expr nSames = Expr(int32_t(1));
        for (const Expr& e : v) nSames += select(e == p, 1, 0);

        // Count of v[m]'s value among v[m..25]: equals the full count at a value's first
        // occurrence and is smaller later, so later occurrences never win or tie.
        Expr bestCnt = Expr(int32_t(0)), bestVal = p;
        for (size_t m = 0; m < v.size(); ++m) {
            Expr cnt = Expr(int32_t(1));
            for (size_t k = m + 1; k < v.size(); ++k) cnt += select(v[k] == v[m], 1, 0);
            cnt = select(v[m] == p, 0, cnt);
            Expr better = cnt > bestCnt || (cnt == bestCnt && cnt > 0 && v[m] < bestVal);
            bestCnt = select(better, cnt, bestCnt);
            bestVal = select(better, v[m], bestVal);
        }

        const Expr examine = vxhl_gen::interior(x, y, z, gnx, gny, gnz, 1) &&
                             nSames + vxhl_gen::cdiv(min_dif, 2) < 14;
        output(x, y, z) = select(examine && bestCnt > nSames + min_dif, bestVal, p);
    }

    void schedule() { vxhl_gen::schedule_stencil(output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(Mode26, mode26)
