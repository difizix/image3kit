// Reduction kernel for voxlib's histogram (voxlib/voxelImageML.h histogram, HistBins::bin). Per z-layer of
// the input it returns the counts of the voxels v with lo <= v < hi in each of nBins equal bins over
// [shift, shift+span):
//   stats(b, z) = #{ v : bin(v) == b },  bin(v) = clamp(floor((v - shift) * nBins / span), 0, nBins - 1)
// (double, exact: a layer has at most nx·ny voxels). b runs over 0..255, entries b >= nBins are 0.
// The host sums the per-z partials (vxhl ops.cpp), so slabs/ranks combine exactly.
// NaN voxels are outside every range, as in voxlib's comparisons.
// The GPU counts with int32 atomics (Vulkan has no f64 atomics); the CPU keeps one private histogram per
// z-layer and needs none.

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class Histogram : public Generator<Histogram> {
public:
    VXHL_STENCIL_INPUTS
    Input<double> lo{"lo"}, hi{"hi"}, shift{"shift"}, span{"span"};
    Input<int32_t> nBins{"nBins"};
    Output<Buffer<double, 2>> stats{"stats"};  // run_reduce ABI: stats(i, z)

    Var b{"b"}, l{"l"}, z{"z"}, c{"c"};
    Func hist{"hist"};
    RDom r;

    void generate() {
        // GPU: thread (l, z) owns the private histogram hist(., l, z) and visits the voxels x = k·L + l of the
        // layer, so neighbouring threads read neighbouring voxels; no atomics (Halide's Vulkan backend has none).
        // CPU: one lane, one private histogram per layer.
        const int L = get_target().has_gpu_feature() ? kLanes : 1;
        r = RDom(0, (gnx + (L - 1)) / L, 0, gny);
        const Expr x = r.x * L + l;
        const Expr inImg = x < gnx;
        const Expr v = cast<double>(input(min(x, gnx - 1), r.y, z));
        const Expr in = inImg && lo <= v && v < hi;
        // voxlib: (v - shift) * nBins / span; the quotient is correctly rounded on every target (div64)
        const Expr q = vxhl_gen::div64((v - shift) * cast<double>(nBins), span, get_target());
        // clamp(floor(q), 0, nBins-1) == trunc(clamp(q, 0, nBins-1)) (q clamped is >= 0); Vulkan has no f64 floor
        const Expr bin = cast<int32_t>(clamp(q, Internal::make_const(Float(64), 0.0), cast<double>(nBins - 1)));

        hist(b, l, z) = cast<int32_t>(0);
        hist(select(in, bin, 0), l, z) += select(in, 1, 0);
        RDom rl(0, L);
        stats(c, z) = cast<double>(sum(hist(c, rl, z)));
        lanes = L;
    }

    void schedule() {
        hist.bound(b, 0, 256).bound(l, 0, lanes);
        stats.bound(c, 0, 256);
        if (get_target().has_gpu_feature()) {
            // one block per layer; its kLanes threads each own a private histogram in shared memory (no atomics
            // needed, and the serial read-modify-write chain runs at shared-memory instead of DRAM latency)
            Var ci{"ci"};
            stats.gpu_blocks(z).gpu_threads(c);
            hist.compute_at(stats, z).store_in(MemoryType::GPUShared).gpu_threads(l);
            hist.update().gpu_threads(l);
        } else {
            hist.compute_root();
            hist.parallel(z);
            hist.update().parallel(z);
        }
    }

private:
    static constexpr int kLanes = 32;
    int lanes = 1;
};

}  // namespace

HALIDE_REGISTER_GENERATOR(Histogram, histogram)
