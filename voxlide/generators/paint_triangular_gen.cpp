// Halide port of paint / paint_add for the `triangular` shape (voxlib/shapeToVoxel.h, triangular::value):
// CPU only (Halide's Vulkan backend has no f64 cos/sin). Parameters: po.x po.y po.z L1 L2 h hInv LtInv ch ctsInv.
// Same voxel-centre geometry, set/add modes and voxel-type arithmetic as paint_shape_gen.cpp.

#include <cmath>
#include "vxhl_gen.h"

namespace {

using namespace Halide;

Expr dbl(double v) { return Internal::make_const(Float(64), v); }

class PaintTriangular : public Generator<PaintTriangular> {
public:
    VXHL_STENCIL_INPUTS
    Input<int32_t> mode{"mode"}, insidev{"insidev"};
    Input<double[3]> dx{"dx"}, x0{"x0"};
    Input<double[10]> prm{"prm"};
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        const Type t = input.type();
        const Expr v = input(x, y, z);
        const Expr px = (cast<double>(x) + dbl(0.5)) * dx[0] + x0[0], py = (cast<double>(y) + dbl(0.5)) * dx[1] + x0[1],
                   pz = (cast<double>(z) + dbl(0.5)) * dx[2] + x0[2];
        const Expr pox = prm[0], poy = prm[1], poz = prm[2], L1 = prm[3], L2 = prm[4], h = prm[5], hInv = prm[6],
                   LtInv = prm[7], ch = prm[8], ctsInv = prm[9];
        const double PI = 3.141592653589793;
        const Expr ddx = LtInv * (px - pox);
        const Expr inY = !(py < poy - h || py > poy);
        const Expr dyx = cos(dbl(0.5 * PI) * ddx);
        const Expr dy_mid = dyx * sin(dbl(0.25 * PI) * ddx);
        const Expr dy = hInv * (((poy - py) - (ch * dyx) * dyx) - (ctsInv * dy_mid) * dy_mid);
        const Expr dz = pz - poz;
        const Expr inside = inY && !(dz < L1 * dy || dz > L2 * dy);
        const Expr added = t.is_float() ? v + cast<float>(insidev) : cast(t, cast<int32_t>(v) + insidev);
        output(x, y, z) = select(inside, select(mode == 0, cast(t, insidev), added), v);
    }
    void schedule() { vxhl_gen::schedule_stencil(output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(PaintTriangular, paintTriangular)
