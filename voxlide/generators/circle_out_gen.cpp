// Halide port of ::circleOut(vImage, X0, Y0, R, dir, outVal), voxlib/voxelImageI.h:1597 (Python
// circle_out): voxels outside the circle of radius R get outVal.
//  dir 0 ('z'): (x-X0)² + (y-Y0)² > R² for every z;
//  dir 1 ('x'): (y-X0)² + (z-Y0)² > R² for every x (voxlib reuses X0 for y and Y0 for z).
// int32 arithmetic as in voxlib (R² must fit in an int).

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class CircleOut : public Generator<CircleOut> {
public:
    VXHL_STENCIL_INPUTS
    Input<int32_t> X0{"X0"}, Y0{"Y0"}, R{"R"}, dir{"dir"};
    Input<Expr> outVal{"outVal"};  // voxel type (VALUES)
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};
    void generate() {
        const Expr rlim = R * R;
        const Expr outZ = (x - X0) * (x - X0) + (y - Y0) * (y - Y0) > rlim;
        const Expr outX = (y - X0) * (y - X0) + (z - Y0) * (z - Y0) > rlim;
        output(x, y, z) = select(select(dir == 0, outZ, outX), outVal, input(x, y, z));
    }
    void schedule() { vxhl_gen::schedule_stencil(output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(CircleOut, circleOut)
