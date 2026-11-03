// Reduction kernel shared by voxlib's statistics (voxlib/voxelImageI.h:119-166: varianceDbl,
// stdevDbl, meanDbl, minDbl, maxDbl, porosityDbl). Per z-layer of the input it returns, over
// the voxels v with lo <= v <= hi (all voxels when flags & 2):
//   count, Σ(v−center), Σ(v−center)², min v, max v          (5 outputs, double per z)
// with v = double(voxel), or double(int(voxel)) when flags & 1 (varianceDbl's `int vv=*vp`).
// The host merges the per-z partials (vxhl ops.cpp), so slabs/ranks combine exactly.
// min/max start from DBL_MAX/-DBL_MAX as voxlib does. NaN voxels are outside every range,
// as in voxlib's comparisons (but meanDbl, flags & 2, sums them).
// Integer-valued sums are exact in double; others (f32 images, Σ(v−mean)²) differ from
// voxlib's OpenMP summation order only by rounding.

#include <cfloat>
#include "vxhl_gen.h"

namespace {

using namespace Halide;

class Moments : public Generator<Moments> {
public:
    VXHL_STENCIL_INPUTS
    Input<double> lo{"lo"}, hi{"hi"}, center{"center"};
    Input<int32_t> flags{"flags"};
    Output<Buffer<double, 1>> count{"count"}, sum{"sum"}, sumsq{"sumsq"}, vmin{"vmin"}, vmax{"vmax"};

    Var x{"x"}, z{"z"};
    Func cols{"cols"}, slices{"slices"};  // partials per (x, z) over y: GPU threads read along x

    void generate() {
        const Expr zero = Internal::make_const(Float(64), 0.0), one = Internal::make_const(Float(64), 1.0);
        const Expr big = Internal::make_const(Float(64), DBL_MAX), low = Internal::make_const(Float(64), -DBL_MAX);
        RDom rx(0, gnx), ry(0, gny);
        const Expr raw = input(x, ry, z);
        const Expr v = select((flags & 1) != 0, cast<double>(cast<int32_t>(raw)), cast<double>(raw));
        const Expr in = (flags & 2) != 0 || (lo <= v && v <= hi);
        const Expr d = v - center;

        cols(x, z) = {zero, zero, zero, big, low};
        cols(x, z) = {cols(x, z)[0] + select(in, one, zero), cols(x, z)[1] + select(in, d, zero),
                      cols(x, z)[2] + select(in, d * d, zero), select(in, min(cols(x, z)[3], v), cols(x, z)[3]),
                      select(in, max(cols(x, z)[4], v), cols(x, z)[4])};
        slices(z) = {zero, zero, zero, big, low};
        slices(z) = {slices(z)[0] + cols(rx, z)[0], slices(z)[1] + cols(rx, z)[1], slices(z)[2] + cols(rx, z)[2],
                     min(slices(z)[3], cols(rx, z)[3]), max(slices(z)[4], cols(rx, z)[4])};
        count(z) = slices(z)[0];
        sum(z) = slices(z)[1];
        sumsq(z) = slices(z)[2];
        vmin(z) = slices(z)[3];
        vmax(z) = slices(z)[4];
    }

    void schedule() {
        cols.compute_root();
        slices.compute_root();
        if (get_target().has_gpu_feature()) {
            Var xi{"xi"}, zi{"zi"};
            cols.gpu_tile(x, z, xi, zi, 64, 1, TailStrategy::GuardWithIf);
            cols.update().gpu_tile(x, z, xi, zi, 64, 1, TailStrategy::GuardWithIf);
            slices.gpu_tile(z, zi, 32, TailStrategy::GuardWithIf);
            slices.update().gpu_tile(z, zi, 32, TailStrategy::GuardWithIf);
            for (Func o : std::vector<Func>{Func(count), Func(sum), Func(sumsq), Func(vmin), Func(vmax)}) o.gpu_tile(z, zi, 32, TailStrategy::GuardWithIf);
        } else {
            const int vec = get_target().natural_vector_size(Float(64));
            cols.parallel(z).vectorize(x, vec, TailStrategy::GuardWithIf);
            cols.update().parallel(z).vectorize(x, vec, TailStrategy::GuardWithIf);
        }
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(Moments, moments)
