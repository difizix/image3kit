// Halide port of ::bilateralX(VoxelImageT<T>&, kernRad, Xstp, sigmaSqr, sharpFact, sigmaGaussSqr,
// capmin, capmax), voxlib/voxelImageML.h:243, one call (= one pass). Kernel args: kr = radius
// (voxlib's kernRad), stp = sampling step (voxlib's Xstp).
// vxhl::_bilateralX (voxlide/ops.cpp) reproduces ::_bilateralX's growBox and per-iteration radii.
// Semantics reproduced exactly (float32, strict_float target: no FMA, no reassociation):
//  - only voxels >= kr from every face of the image change, and only if capmin <= v <= capmax;
//  - neighbours at offsets -kr + m·stp <= kr per axis (forAllNei_m_), summed in voxlib's
//    loop order (k outer, then j, then i) into 6 float accumulators initialised to
//    0, 1e-15, 0, 1e-15, 0, -0.9 as in voxlib;
//  - `nv >= -1e-32` (float vs double) is compared against the smallest float >= -1e-32;
//  - neu = v + S/W·(1+sharp) − sharp·G/GW + 0.49, clamped to [capmin, capmax] and then to
//    [v + (S−M)/(1+W−MW), v + M/MW]; stored with C++ float→T truncation.

#include <cmath>
#include "vxhl_gen.h"

namespace {

using namespace Halide;

class BilateralX : public Generator<BilateralX> {
public:
    VXHL_STENCIL_INPUTS
    Input<int32_t> kr{"kr"};
    Input<int32_t> stp{"stp"};
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

        const Expr nOff = 2 * kr / stp + 1;
        RDom r(0, nOff, 0, nOff, 0, nOff);  // r.x innermost = voxlib's i loop
        const Expr di = r.x * stp - kr, dj = r.y * stp - kr, dk = r.z * stp - kr;
        const Expr nv = cast<float>(in(x + di, y + dj, z + dk)) - v;
        const Expr dist2 = cast<float>(dk * dk + dj * dj + di * di);
        const Expr w = 1.0f / (1.0f + nv * nv * p1rSigma + dist2 * p1rGSigma);
        float negTiny = -1e-32f;  // smallest float >= -1e-32 (double)
        if (double(negTiny) < -1e-32) negTiny = std::nextafter(negTiny, 0.0f);
        const Expr wM = select(nv >= negTiny, w, 0.0f);
        const Expr wG = min(nv * nv, sigmaSqr) / (0.01f + dist2 * p1rGSigma);

        acc(x, y, z) = {0.0f, float(1e-15), 0.0f, float(-0.9), 0.0f, float(1e-15)};  // S W M MW G GW
        acc(x, y, z) = {acc(x, y, z)[0] + w * nv, acc(x, y, z)[1] + w,  acc(x, y, z)[2] + wM * nv,
                        acc(x, y, z)[3] + wM,     acc(x, y, z)[4] + wG * nv, acc(x, y, z)[5] + wG};
        const Expr S = acc(x, y, z)[0], W = acc(x, y, z)[1], M = acc(x, y, z)[2], MW = acc(x, y, z)[3],
                   G = acc(x, y, z)[4], GW = acc(x, y, z)[5];

        Expr neu = v + S / W * (1.0f + sharp) - sharp * G / GW + 0.49f;
        neu = max(capmin, min(capmax, neu));
        neu = max(v + (S - M) / (1.0f + W - MW), min(v + M / MW, neu));

        const Expr change = vxhl_gen::interior(x, y, z, gnx, gny, gnz, kr) && capmin <= v && v <= capmax;
        // Via int32: C++ truncates float->T; RADV's direct f32->u8 conversion does not (it is off
        // by one in ~50% of voxels), while f32->i32 truncates.
        const Expr stored = input.type().is_float() ? neu : cast(input.type(), cast<int32_t>(neu));
        output(x, y, z) = select(change, stored, in(x, y, z));
    }

    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), Float(32), {acc}); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(BilateralX, bilateralX)
