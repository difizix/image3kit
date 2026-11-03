// Halide port of VoxelImageT<T>::FaceMedian06(nAdj0, nAdj1), voxlib/voxelImageI.h:1130
// (Python median06; free-function form ::median06). Returns the number of changed voxels (second
// output, ChangeCounts). Semantics reproduced exactly:
//  - every voxel is processed (voxlib works on a growBox(1) copy = repeat_edge);
//  - neiSum = the SUM OF THE VALUES of the 6 face neighbours (-x, +x, -y, +y, -z, +z; binary
//    images: the number of ones), in int (8/16/32-bit) or float (f32) arithmetic;
//  - a nonzero voxel becomes 0 if neiSum <= nAdj0; else a zero voxel becomes 1 if neiSum >= nAdj1.
// Every change alters the value, so #changes = #(output != input).

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class Median06 : public Generator<Median06> {
public:
    VXHL_STENCIL_INPUTS
    Input<int32_t> nAdj0{"nAdj0"}, nAdj1{"nAdj1"};
    Output<Buffer<void, 3>> output{"output"};
    Output<Buffer<int32_t, 1>> changes{"changes"};

    Var x{"x"}, y{"y"}, z{"z"};
    vxhl_gen::ChangeCounts cc;

    void generate() {
        Func in = BoundaryConditions::repeat_edge(input);
        const Type t = input.type();
        const Type acc = t.is_float() ? Float(32) : Int(32);
        const Expr vv = in(x, y, z);
        Expr s = cast(acc, in(x - 1, y, z));
        for (Expr e : {in(x + 1, y, z), in(x, y - 1, z), in(x, y + 1, z), in(x, y, z - 1), in(x, y, z + 1)}) s = s + cast(acc, e);
        const Expr zero = cast(t, 0), one = cast(t, 1);
        const Expr nz = vv != zero;
        output(x, y, z) = select(s <= cast(acc, nAdj0) && nz, zero, select(s >= cast(acc, nAdj1) && !nz, one, vv));
        cc.define(changes, output, input, gnx, gny);
    }

    void schedule() {
        vxhl_gen::schedule_stencil(output, get_target(), input.type());
        cc.schedule(changes, get_target());
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(Median06, median06)
