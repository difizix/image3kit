// Halide port of one inner pass of ::biLateralIterative(VoxelImageT<T>&, kernRad, sigmad, sharpFact),
// voxlib/voxelImageML.h:412. vxhl::smoothBilateral (voxlide/ops.cpp) reproduces
// VoxLib::smoothBilateral (voxlib/voxelImageProcess.h:189): growBox(kernRad+2), then nItrs outer
// iterations of kernRad passes each, where `orig` is the image at the start of the outer
// iteration (Snapshot), then shrinkBox. Semantics reproduced exactly:
//  - only voxels >= 1 from every face of the (grown) image change; the rest are copied;
//  - arithmetic in voxlib's f32_t (generator param ftype) and operation order (strict_float target: no
//    FMA, no reassociation; GPU: corrected f32 divisions, vxhl_gen::divF32):
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
    GeneratorParam<Type> ftype{"ftype", Float(64)};  // voxlib f32_t (F32T)
    VXHL_STENCIL_INPUTS
    Input<Buffer<void, 3>> orig{"orig"};  // Snapshot: image at the start of the outer iteration
    Input<Expr> sigma{"sigma"};  // ftype (FLOATS)
    Input<Expr> sharp{"sharp"};
    Output<Buffer<void, 3>> output{"output"};

    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        Func in = BoundaryConditions::repeat_edge(input);
        const Expr p = in(x, y, z);
        const Type ft = ftype;
        const Expr v = cast(ft, p), vO = cast(ft, orig(x, y, z));
        static constexpr int nei[6][3] = {{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}};
        auto f64 = [&](double c) { return vxhl_gen::fconst(ft, c); };
        auto div = [&](Expr a, Expr b) { return vxhl_gen::divF32(a, b, get_target()); };
        Expr sum = f64(0.0), wsum = f64(1e-12);
        for (const auto& d : nei) {
            Expr nv = cast(ft, in(x + d[0], y + d[1], z + d[2])) - vO;
            const Expr w = div(f64(1.0), f64(1.0) + div(nv * nv, sigma));
            nv = nv + (vO - v);
            sum = sum + w * nv;
            wsum = wsum + w;
        }
        const Expr delta = div((f64(1.0) + sharp) * sum, wsum);
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
