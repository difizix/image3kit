// Halide port of one inner pass of ::biLateralIterative(VoxelImageT<T>&, kernRad, sigmad, sharpFact),
// voxlib/voxelImageML.h:412. vxhl::smoothBilateral (voxlide/ops.cpp) reproduces
// VoxLib::smoothBilateral (voxlib/voxelImageProcess.h:189): growBox(kernRad+2), then nItrs outer
// iterations of kernRad passes each, where `orig` is the image at the start of the outer
// iteration (Snapshot), then shrinkBox. Semantics reproduced exactly:
//  - only voxels >= 1 from every face of the (grown) image change; the rest are copied;
//  - double arithmetic in voxlib's operation order (strict_float target: no FMA, no reassociation):
//      nv = n - vO; w = 1/(1 + nv*nv/sigma); nv += vO - v; sum += w*nv; wsum (from 1e-12) += w
//    over the 6 face neighbours in the order i-1, i+1, j-1, j+1, k-1, k+1;
//  - new = clamp(int(v) + int((1+sharp)*sum/wsum + 0.5), 0, imaxT(T)), with C++ truncation
//    toward zero (also of float voxels).

#include <climits>
#include "vxhl_gen.h"

namespace {

using namespace Halide;

class SmoothBilateral : public Generator<SmoothBilateral> {
public:
    VXHL_STENCIL_INPUTS
    Input<Buffer<void, 3>> orig{"orig"};  // Snapshot: image at the start of the outer iteration
    Input<double> sigma{"sigma"};
    Input<double> sharp{"sharp"};
    Output<Buffer<void, 3>> output{"output"};

    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        Func in = BoundaryConditions::repeat_edge(input);
        const Expr p = in(x, y, z);
        const Expr v = cast<double>(p), vO = cast<double>(orig(x, y, z));
        static constexpr int nei[6][3] = {{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}};
        auto f64 = [](double c) { return Internal::make_const(Float(64), c); };
        Expr sum = f64(0.0), wsum = f64(1e-12);
        for (const auto& d : nei) {
            Expr nv = cast<double>(in(x + d[0], y + d[1], z + d[2])) - vO;
            const Expr w = f64(1.0) / (f64(1.0) + nv * nv / sigma);
            nv = nv + (vO - v);
            sum = sum + w * nv;
            wsum = wsum + w;
        }
        const Expr delta = (f64(1.0) + sharp) * sum / wsum;
        const Type t = input.type();
        const int imax = (t.is_float() || t.bits() >= 32) ? INT_MAX  // imaxT(T)
                                                          : int((1LL << (t.bits() - t.is_int())) - 1);
        const Expr nu = clamp(cast<int32_t>(p) + cast<int32_t>(delta + f64(0.5)), 0, imax);
        output(x, y, z) = select(vxhl_gen::interior(x, y, z, gnx, gny, gnz, 1), cast(t, nu), p);
    }

    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(SmoothBilateral, smoothBilateral)
