// Halide port of VoxelImageT<T>::shrinkPore(), voxlib/voxelImageI.h:1201 (Python shrink0; free-function
// form ::shrinkPore): zero voxels next to nonzero ones become 1. Everything reads a copy (Jacobi).
// Rules reproduced exactly, by how many of the three axes the voxel lies on the image boundary
// (x == 0 or nx-1, ...):
//  - none (interior): a 0 voxel becomes 1 if sum6Nei != 0, the SUM of the VALUES of its 6 face
//    neighbours (int, or float for f32 images: values may cancel for signed types);
//  - exactly one (a face voxel): a 0 voxel becomes 1 if any of its 4 neighbours TANGENTIAL to that
//    face is nonzero (truthiness; the inward neighbour is not looked at);
//  - two or three (edges, corners): unchanged.

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class ShrinkPore : public Generator<ShrinkPore> {
public:
    VXHL_STENCIL_INPUTS
    Output<Buffer<void, 3>> output{"output"};

    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        Func in = BoundaryConditions::repeat_edge(input);
        const Type t = input.type();
        const Type acc = t.is_float() ? Float(32) : Int(32);
        const Expr zero = cast(t, 0), one = cast(t, 1);
        const Expr vv = in(x, y, z);
        auto nz = [&](Expr e) { return e != zero; };

        Expr s = cast(acc, in(x - 1, y, z));
        for (Expr e : {in(x + 1, y, z), in(x, y - 1, z), in(x, y + 1, z), in(x, y, z - 1), in(x, y, z + 1)}) s = s + cast(acc, e);
        const Expr tx = nz(in(x, y - 1, z)) || nz(in(x, y + 1, z)) || nz(in(x, y, z - 1)) || nz(in(x, y, z + 1));
        const Expr ty = nz(in(x - 1, y, z)) || nz(in(x + 1, y, z)) || nz(in(x, y, z - 1)) || nz(in(x, y, z + 1));
        const Expr tz = nz(in(x - 1, y, z)) || nz(in(x + 1, y, z)) || nz(in(x, y - 1, z)) || nz(in(x, y + 1, z));

        const Expr bx = x == 0 || x == gnx - 1, by = y == 0 || y == gny - 1, bz = z == 0 || z == gnz - 1;
        const Expr grow = (!bx && !by && !bz && s != cast(acc, 0)) || (bx && !by && !bz && tx) ||
                          (by && !bx && !bz && ty) || (bz && !bx && !by && tz);
        output(x, y, z) = select(vv == zero && grow, one, vv);
    }

    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(ShrinkPore, shrinkPore)
