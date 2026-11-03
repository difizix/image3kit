// Halide port of VoxelImageT<T>::growPore(), voxlib/voxelImageI.h:1326 (Python grow0; free-function
// form ::growPore): nonzero voxels next to a zero voxel become 0. Everything reads a copy (Jacobi).
// Rules reproduced exactly, by how many of the three axes the voxel lies on the image boundary:
//  - none or exactly one (interior and face voxels): a nonzero voxel becomes 0 if any of its face
//    neighbours INSIDE the image is 0 (on a face that is the 5 neighbours other than the outward one;
//    with repeat_edge the outward neighbour equals the voxel, which is nonzero, so it never counts);
//  - two or three (edges, corners): unchanged.
// voxlib reads out of range when a dimension is 1 ("buggy for nz=1"); not reproduced: use
// dimensions >= 2.

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class GrowPore : public Generator<GrowPore> {
public:
    VXHL_STENCIL_INPUTS
    Output<Buffer<void, 3>> output{"output"};

    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        Func in = BoundaryConditions::repeat_edge(input);
        const Expr zero = cast(input.type(), 0);
        const Expr vv = in(x, y, z);
        const Expr anyZero = in(x - 1, y, z) == zero || in(x + 1, y, z) == zero || in(x, y - 1, z) == zero ||
                             in(x, y + 1, z) == zero || in(x, y, z - 1) == zero || in(x, y, z + 1) == zero;
        const Expr nBoundary = select(x == 0 || x == gnx - 1, 1, 0) + select(y == 0 || y == gny - 1, 1, 0) +
                               select(z == 0 || z == gnz - 1, 1, 0);
        output(x, y, z) = select(vv != zero && anyZero && nBoundary <= 1, zero, vv);
    }

    void schedule() { vxhl_gen::schedule_stencil(output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(GrowPore, growPore)
