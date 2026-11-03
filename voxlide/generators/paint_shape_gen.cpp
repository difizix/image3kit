// Halide port of paint / paint_add (voxlib/shapeToVoxel.h shape::setIn / shape::addTo) for the
// sphere, cylinder and cube (kube) shapes: every voxel centre p = (index + 0.5)·dx + X0 inside the
// shape is set to the shape's value (`mode` 0) or has it added (`mode` 1, in the voxel type: int
// arithmetic with wrapping for integer voxels, float for f32).
// All double arithmetic in voxlib's order (strict_float). The shape's parameters arrive as 7 numbers:
//   sphere   (kind 0): p1.x p1.y p1.z r2
//   cylinder (kind 1): p1.x p1.y p1.z p2.x p2.y p2.z r2mag2p12   inside if |(p-p1) x (p-p2)|^2 <= r2mag2p12
//   cube     (kind 2): p1 p2 (lower corner inclusive, upper corner exclusive)

#include "vxhl_gen.h"

namespace {

using namespace Halide;

Expr dbl(double v) { return Internal::make_const(Float(64), v); }

class PaintShape : public Generator<PaintShape> {
public:
    VXHL_STENCIL_INPUTS
    Input<int32_t> kind{"kind"}, mode{"mode"}, insidev{"insidev"};
    Input<double[3]> dx{"dx"}, x0{"x0"};
    Input<double[7]> prm{"prm"};
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        const Type t = input.type();
        const Expr v = input(x, y, z);
        const Expr px = (cast<double>(x) + dbl(0.5)) * dx[0] + x0[0], py = (cast<double>(y) + dbl(0.5)) * dx[1] + x0[1],
                   pz = (cast<double>(z) + dbl(0.5)) * dx[2] + x0[2];
        // sphere: magSqr(p - p1) < r2
        const Expr sx = px - prm[0], sy = py - prm[1], sz = pz - prm[2];
        const Expr sphere = (sx * sx + sy * sy) + sz * sz < prm[3];
        // cylinder: magSqr((p - p1) ^ (p - p2)) <= r2mag2p12;  a ^ b = (ay bz - az by, az bx - ax bz, ax by - ay bx)
        const Expr ax = sx, ay = sy, az = sz, bx = px - prm[3], by = py - prm[4], bz = pz - prm[5];
        const Expr cx = ay * bz - az * by, cy = az * bx - ax * bz, cz = ax * by - ay * bx;
        const Expr cylinder = (cx * cx + cy * cy) + cz * cz <= prm[6];
        // cube: p >= p1 && p < p2
        const Expr cube = px >= prm[0] && py >= prm[1] && pz >= prm[2] && px < prm[3] && py < prm[4] && pz < prm[5];
        const Expr inside = select(kind == 0, sphere, select(kind == 1, cylinder, cube));
        const Expr added = t.is_float() ? v + cast<float>(insidev) : cast(t, cast<int32_t>(v) + insidev);
        output(x, y, z) = select(inside, select(mode == 0, cast(t, insidev), added), v);
    }
    void schedule() { vxhl_gen::schedule_stencil(output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(PaintShape, paintShape)
