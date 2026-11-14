// Halide port of ::bilateralGauss(VoxelImageT<T>&, kernRad, sigmaSqr, sharpFact, sigmaGaussSqr,
// capmin, capmax), voxlib/voxelImageML.h:313, one call (= one pass). Kernel arg kr = kernRad.
// vxhl::_bilateralGauss (voxlide/ops.cpp) reproduces ::_bilateralGauss's growBox and iteration loop.
// Semantics reproduced exactly, in voxlib's f32_t (generator param ftype; strict IEEE, GPU f32 divisions corrected):
//  - only voxels >= kr from every face of the image change, and only if capmin <= v <= capmax;
//  - neighbours at all offsets -kr..kr per axis, summed in voxlib's loop order (k outer, then j,
//    then i) into 4 accumulators initialised to 0, 1e-18, 0, 1e-18;
//  - weit = 1 / ((1 + nv*nv*p1rSigma) + dist2*p1rGSigma);
//  - Gauss weight = min(nv*nv, sigmaSqr) * dist2;
//  - result = max(capmin, min(capmax, v + S/W·(1+sharp) − sharp·G/GW + 0.5)), stored with
//    C++ f32_t→T conversion (truncation for integer T).

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class BilateralGauss : public Generator<BilateralGauss> {
public:
    GeneratorParam<Type> ftype{"ftype", Float(64)};  // voxlib f32_t (F32T)
    VXHL_STENCIL_INPUTS
    Input<int32_t> kr{"kr"};
    // ftype (FLOATS); p1rSigma = f32_t(1./sigmaSqr), p1rGSigma = f32_t(1./sigmaGaussSqr), computed on the host as voxlib does
    Input<Expr> sigmaSqr{"sigmaSqr"}, p1rSigma{"p1rSigma"}, p1rGSigma{"p1rGSigma"}, sharp{"sharp"}, capmin{"capmin"},
        capmax{"capmax"};
    Output<Buffer<void, 3>> output{"output"};

    Var x{"x"}, y{"y"}, z{"z"};
    Func acc{"acc"};

    void generate() {
        Func in = BoundaryConditions::repeat_edge(input);
        const Type ft = ftype;
        auto K = [&](double c) { return vxhl_gen::fconst(ft, c); };
        auto div = [&](Expr a, Expr b) { return vxhl_gen::divF32(a, b, get_target()); };
        const Expr v = cast(ft, in(x, y, z));

        const Expr nOff = 2 * kr + 1;
        RDom r(0, nOff, 0, nOff, 0, nOff);  // r.x innermost = voxlib's i loop
        const Expr di = r.x - kr, dj = r.y - kr, dk = r.z - kr;
        const Expr nv = cast(ft, in(x + di, y + dj, z + dk)) - v;
        const Expr dist2 = cast(ft, dk * dk + dj * dj + di * di);
        const Expr w = div(K(1.0), (K(1.0) + nv * nv * p1rSigma) + dist2 * p1rGSigma);
        const Expr wG = min(nv * nv, sigmaSqr) * dist2;

        acc(x, y, z) = {K(0.0), K(1e-18), K(0.0), K(1e-18)};  // S W G GW
        acc(x, y, z) = {acc(x, y, z)[0] + w * nv, acc(x, y, z)[1] + w, acc(x, y, z)[2] + wG * nv,
                        acc(x, y, z)[3] + wG};
        const Expr S = acc(x, y, z)[0], W = acc(x, y, z)[1], G = acc(x, y, z)[2], GW = acc(x, y, z)[3];

        Expr neu = v + div(S, W) * (K(1.0) + sharp) - div(sharp * G, GW) + K(0.5);
        neu = max(capmin, min(capmax, neu));

        const Expr change = vxhl_gen::interior(x, y, z, gnx, gny, gnz, kr) && capmin <= v && v <= capmax;
        // Via int32: C++ truncates float->T; RADV's direct f32->u8 conversion does not (see bilateral_x_gen.cpp).
        const Expr stored = input.type().is_float() ? cast(input.type(), neu) : cast(input.type(), cast<int32_t>(neu));
        output(x, y, z) = select(change, stored, in(x, y, z));
    }

    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), Type(ftype), {acc}); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(BilateralGauss, bilateralGauss)
