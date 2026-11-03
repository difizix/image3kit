// meanWide's border fix (VoxLib::meanWide, voxelImageProcess.h): after growBox(nW+5), for i = 1..3
//   replaceyLayer(ny-1-i, 4); replaceyLayer(i, ny-5); replacexLayer(nx-1-i, 4); replacexLayer(i, nx-5);
//   setLayer z likewise,
// i.e. the planes ny-4..ny-2 take plane 4 and planes 1..3 take plane ny-5 (per axis). The source planes are never
// destinations, so the copies commute and the result is the gather out(x,y,z) = in(m(x), m(y), m(z)) with
// m(j; n) = 4 for j in [n-4, n-2], n-5 for j in [1, 3], j otherwise. Needs n >= 10 per axis (guaranteed by growBox(>=6)).
// Reads planes anywhere in z: the driver gives the op a halo of gnz layers.

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class PadEdges : public Generator<PadEdges> {
public:
    VXHL_STENCIL_INPUTS
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};

    static Expr m(Expr j, Expr n) { return select(j >= n - 4 && j <= n - 2, Expr(4), select(j >= 1 && j <= 3, n - 5, j)); }
    void generate() { output(x, y, z) = input(m(x, gnx), m(y, gny), m(z, gnz)); }
    void schedule() { vxhl_gen::schedule_stencil(output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(PadEdges, padEdges)
