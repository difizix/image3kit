// Halide port of ::shrink(VoxelImageT<T>& seged, T min, T maks, const VoxelImageT<T>& vImg),
// voxlib/voxelImageML.h:484 (used by segment2). Kernel args: lo = min, hi = maks, src = vImg.
// voxlib makes 3 passes (x, y, z face pairs) that read a copy of seged and, for every face pair
// whose copy values differ, set each voxel of the pair to vImg if its *current* seged value is
// in [min, maks]. A voxel is only ever set to vImg(p), so the passes are idempotent and the
// result is a gather:
//   out(p) = vImg(p) if min <= seged(p) <= maks and some face neighbour inside the image
//            differs from seged(p); else seged(p).
// All voxels are processed, including the image faces (pairs never leave the image).

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class Shrink : public Generator<Shrink> {
public:
    VXHL_STENCIL_INPUTS
    Input<Expr> lo{"lo"};  // voxel type (voxlide_add_op VALUES)
    Input<Expr> hi{"hi"};
    Input<Buffer<void, 3>> src{"src"};  // Aux image (voxlide_add_op IMAGES)
    Output<Buffer<void, 3>> output{"output"};

    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        Func in = BoundaryConditions::repeat_edge(input);
        const Expr p = in(x, y, z);
        // Neighbours outside the image are not compared (not "equal": NaN != NaN).
        const Expr differs = (x > 0 && in(x - 1, y, z) != p) || (x < gnx - 1 && in(x + 1, y, z) != p) ||
                             (y > 0 && in(x, y - 1, z) != p) || (y < gny - 1 && in(x, y + 1, z) != p) ||
                             (z > 0 && in(x, y, z - 1) != p) || (z < gnz - 1 && in(x, y, z + 1) != p);
        output(x, y, z) = select(lo <= p && p <= hi && differs, src(x, y, z), p);
    }

    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(Shrink, shrink)
