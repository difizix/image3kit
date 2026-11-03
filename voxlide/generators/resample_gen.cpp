// voxlib resampleMean and resliceZ (voxelImageI.h; Python resample, resample_mean, reslice_z): outputs of
// another size (run_remap), one kernel per branch so slabs read only the input layers they need.
//   resampleUp     (factor < 1, n = int(1/factor+0.5)): out(x,y,z) = in(int((0.5+x)/n), …) = in((2x+1)/(2n), …)
//   resampleMeanDn (factor > 1, n = int(factor+0.5)):  out = T(0.5 + double(sum)/n³), the n³ block summed into
//                  an int in voxlib's (k, j, i) order (float voxels: int(float(sum) + v) at every step)
//   resliceUp      (factor < 1): out(x,y,z) = in(x, y, int((0.5+z)·factor))
//   resliceDn      (factor > 1, integer voxels): out = T(Σ_kk in(x, y, z·n+kk) / n), C++ integer division

#include "vxhl_gen.h"

namespace {

using namespace Halide;

#define VXHL_RESAMPLE_GEN(NAME, BODY, ...)                                         \
    class NAME : public Generator<NAME> {                                          \
    public:                                                                        \
        VXHL_STENCIL_INPUTS                                                        \
        __VA_ARGS__                                                                \
        Output<Buffer<void, 3>> output{"output"};                                  \
        Var x{"x"}, y{"y"}, z{"z"};                                                \
        Func acc{"acc"}; /* per-voxel reduction, if any */                         \
        void generate() { BODY }                                                   \
        void schedule() {                                                          \
            std::vector<Func> per;                                                 \
            if (acc.defined()) per.push_back(acc);                                 \
            vxhl_gen::schedule_stencil(output, get_target(), output.type(), per);  \
        }                                                                          \
    };

VXHL_RESAMPLE_GEN(ResampleUp, {
    output(x, y, z) = input((2 * x + 1) / (2 * n), (2 * y + 1) / (2 * n), (2 * z + 1) / (2 * n));
}, Input<int32_t> n{"n"};)

VXHL_RESAMPLE_GEN(ResampleMeanDn, {
    RDom r(0, n, 0, n, 0, n);  // r.x innermost: voxlib's forAllNei (k, j, i) order
    const Expr v = input(x * n + r.x, y * n + r.y, z * n + r.z);
    acc(x, y, z) = 0;
    if (input.type().is_float()) acc(x, y, z) = cast<int32_t>(cast<float>(acc(x, y, z)) + v);
    else acc(x, y, z) = acc(x, y, z) + cast<int32_t>(v);
    const Expr n3 = cast<double>(n * n * n);
    output(x, y, z) = cast(output.type(), Internal::make_const(Float(64), 0.5) +
                                              vxhl_gen::div64(cast<double>(acc(x, y, z)), n3, get_target()));
}, Input<int32_t> n{"n"};)

VXHL_RESAMPLE_GEN(ResliceUp, {
    output(x, y, z) = input(x, y, cast<int32_t>((Internal::make_const(Float(64), 0.5) + cast<double>(z)) * factor));
}, Input<double> factor{"factor"};)

VXHL_RESAMPLE_GEN(ResliceDn, {
    RDom r(0, n);
    const Expr s = sum(cast<int32_t>(input(x, y, z * n + r)));
    output(x, y, z) = cast(output.type(), select(s >= 0, s / n, -((-s) / n)));  // C++ division truncates
}, Input<int32_t> n{"n"};)

}  // namespace

HALIDE_REGISTER_GENERATOR(ResampleUp, resampleUp)
HALIDE_REGISTER_GENERATOR(ResampleMeanDn, resampleMeanDn)
HALIDE_REGISTER_GENERATOR(ResliceUp, resliceUp)
HALIDE_REGISTER_GENERATOR(ResliceDn, resliceDn)
