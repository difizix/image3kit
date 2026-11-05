// Halide port of ::bilateralGauss(VoxelImageT<T>&, kernRad, sigmaSqr, sharpFact, sigmaGaussSqr,
// capmin, capmax), voxlib/voxelImageML.h:313, one call (= one pass). Kernel arg kr = kernRad.
// vxhl::_bilateralGauss (voxlide/ops.cpp) reproduces ::_bilateralGauss's growBox and iteration loop.
// Semantics reproduced exactly (voxlib mixes float32 and double; built with FP64 = strict IEEE):
//  - only voxels >= kr from every face of the image change, and only if capmin <= v <= capmax;
//  - neighbours at all offsets -kr..kr per axis, summed in voxlib's loop order (k outer, then j,
//    then i) into 4 float accumulators initialised to 0, float(1e-18), 0, float(1e-18);
//  - weit = float(1. / (1. + nv*nv*p1rSigma + dist2*p1rGSigma)): `1.` is a double, so the two
//    additions and the division are in double; nv*nv*p1rSigma and dist2*p1rGSigma are float;
//  - Gauss weight = min(nv*nv, sigmaSqr) * dist2 (float);
//  - result = max(capmin, min(capmax, v + S/W·(1+sharp) − sharp·G/GW + 0.5f)), stored with
//    C++ float→T truncation.

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class BilateralGauss : public Generator<BilateralGauss> {
public:
    VXHL_STENCIL_INPUTS
    Input<int32_t> kr{"kr"};
    Input<float> sigmaSqr{"sigmaSqr"};
    Input<float> p1rSigma{"p1rSigma"};    // float(1./sigmaSqr), computed on the host as voxlib does
    Input<float> p1rGSigma{"p1rGSigma"};  // float(1./sigmaGaussSqr)
    Input<float> sharp{"sharp"};
    Input<float> capmin{"capmin"};
    Input<float> capmax{"capmax"};
    Output<Buffer<void, 3>> output{"output"};

    Var x{"x"}, y{"y"}, z{"z"};
    Func acc{"acc"};

    void generate() {
        Func in = BoundaryConditions::repeat_edge(input);
        const Expr v = cast<float>(in(x, y, z));

        const Expr nOff = 2 * kr + 1;
        RDom r(0, nOff, 0, nOff, 0, nOff);  // r.x innermost = voxlib's i loop
        const Expr di = r.x - kr, dj = r.y - kr, dk = r.z - kr;
        const Expr nv = cast<float>(in(x + di, y + dj, z + dk)) - v;
        const Expr dist2 = cast<float>(dk * dk + dj * dj + di * di);
        const Expr one = Internal::make_const(Float(64), 1.0);
        const Expr w = cast<float>(one / ((one + cast<double>(nv * nv * p1rSigma)) + cast<double>(dist2 * p1rGSigma)));
        const Expr wG = min(nv * nv, sigmaSqr) * dist2;

        acc(x, y, z) = {0.0f, float(1e-18), 0.0f, float(1e-18)};  // S W G GW
        acc(x, y, z) = {acc(x, y, z)[0] + w * nv, acc(x, y, z)[1] + w, acc(x, y, z)[2] + wG * nv,
                        acc(x, y, z)[3] + wG};
        const Expr S = acc(x, y, z)[0], W = acc(x, y, z)[1], G = acc(x, y, z)[2], GW = acc(x, y, z)[3];

        Expr neu = v + S / W * (1.0f + sharp) - sharp * G / GW + 0.5f;
        neu = max(capmin, min(capmax, neu));

        const Expr change = vxhl_gen::interior(x, y, z, gnx, gny, gnz, kr) && capmin <= v && v <= capmax;
        // Via int32: C++ truncates float->T; RADV's direct f32->u8 conversion does not (see bilateral_x_gen.cpp).
        const Expr stored = input.type().is_float() ? neu : cast(input.type(), cast<int32_t>(neu));
        output(x, y, z) = select(change, stored, in(x, y, z));
    }

    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), Float(32), {acc}); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(BilateralGauss, bilateralGauss)
