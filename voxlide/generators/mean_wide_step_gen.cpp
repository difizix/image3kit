// Halide port of ::meanWide(vImg, nW, lowerB, upperB, grad, gradthresh) (voxlib/voxelImageML.h:666), one meanWide
// iteration. In the interior nW layers in from the faces, a voxel v outside [lowerB, upperB] with grad <= gradthresh
// takes (Σ + n/2)/n over the neighbours n in [lowerB, upperB] of two sets of 27 samples (radius nW and nW/2):
// 9 centres {(0,0), (±w in j), (±w in k), the four (±w, ±w) in (k, j)}, each with x-1, x, x+1; if n > 5.
// For nW = 1 both sets coincide and every sample counts twice, as in voxlib. voxlib reads a copy: a gather.

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class MeanWideStep : public Generator<MeanWideStep> {
public:
    VXHL_STENCIL_INPUTS
    Input<Buffer<void, 3>> grad{"grad"};  // Aux image
    Input<int32_t> nW{"nW"};
    Input<Expr> lowerB{"lowerB"}, upperB{"upperB"}, gradthresh{"gradthresh"};  // voxel type (VALUES)
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        Func in = BoundaryConditions::repeat_edge(input);
        const Expr vv = in(x, y, z);
        Expr nNeis = Expr(0), sum = Expr(0);
        const Expr half = nW / 2;
        for (const Expr w : std::vector<Expr>{Expr(nW), half})
            for (const auto& [dj, dk] : std::vector<std::pair<int, int>>{{0, 0}, {-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {1, 1}, {1, -1}, {-1, 1}})
                for (int di = -1; di <= 1; ++di) {
                    const Expr s = in(x + di, y + dj * w, z + dk * w);
                    const Expr ok = lowerB <= s && s <= upperB;
                    nNeis = nNeis + select(ok, 1, 0);
                    sum = sum + select(ok, cast<int32_t>(s), 0);
                }
        const Expr num = sum + nNeis / 2;
        const Expr mean = select(num >= 0, num / nNeis, -((-num) / nNeis));  // C++ division
        const Expr change = vxhl_gen::interior(x, y, z, gnx, gny, gnz, nW) && (vv < lowerB || upperB < vv) &&
                            grad(x, y, z) <= gradthresh && nNeis > 5;
        output(x, y, z) = select(change, cast(output.type(), mean), vv);
    }
    void schedule() { vxhl_gen::schedule_stencil(output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(MeanWideStep, meanWideStep)
