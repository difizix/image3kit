// Line-scan part of VoxLib::growOutSideValue(vImg, vo, vnew, nHoleSize) (voxelImageProcess.h:371). From each of
// the six faces voxlib walks along a line while the ORIGINAL voxels equal vo and sets those to vnew. A voxel is
// set iff it equals vo and all voxels between it and a face (on its line, in one of the six directions) equal
// vo. Each direction is a serial scan along one axis, parallel over the other two (F9).
// The scans along z need the whole image: the driver gives this op a halo of gnz layers.

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class GrowOutside : public Generator<GrowOutside> {
public:
    VXHL_STENCIL_INPUTS
    Input<Expr> vo{"vo"}, vnew{"vnew"};  // voxel type (VALUES)
    Output<Buffer<void, 3>> output{"output"};

    Var x{"x"}, y{"y"}, z{"z"};
    Func lowX{"lowX"}, highX{"highX"}, lowY{"lowY"}, highY{"highY"}, lowZ{"lowZ"}, highZ{"highZ"};
    RDom rx, ry, rz;

    static Expr yes(Expr c) { return select(c, cast<uint8_t>(1), cast<uint8_t>(0)); }

    void generate() {
        const Expr isVo = input(x, y, z) == vo;
        rx = RDom(1, gnx - 1);
        ry = RDom(1, gny - 1);
        rz = RDom(1, gnz - 1);
        // low*: all voxels from the low face up to this one equal vo; high*: from this one up to the high face
        lowX(x, y, z) = yes(isVo);
        lowX(rx, y, z) = yes(lowX(rx - 1, y, z) != 0 && input(rx, y, z) == vo);
        highX(x, y, z) = yes(isVo);
        highX(gnx - 1 - rx, y, z) = yes(highX(gnx - rx, y, z) != 0 && input(gnx - 1 - rx, y, z) == vo);
        lowY(x, y, z) = yes(isVo);
        lowY(x, ry, z) = yes(lowY(x, ry - 1, z) != 0 && input(x, ry, z) == vo);
        highY(x, y, z) = yes(isVo);
        highY(x, gny - 1 - ry, z) = yes(highY(x, gny - ry, z) != 0 && input(x, gny - 1 - ry, z) == vo);
        lowZ(x, y, z) = yes(isVo);
        lowZ(x, y, rz) = yes(lowZ(x, y, rz - 1) != 0 && input(x, y, rz) == vo);
        highZ(x, y, z) = yes(isVo);
        highZ(x, y, gnz - 1 - rz) = yes(highZ(x, y, gnz - rz) != 0 && input(x, y, gnz - 1 - rz) == vo);

        const Expr marked = lowX(x, y, z) != 0 || highX(x, y, z) != 0 || lowY(x, y, z) != 0 || highY(x, y, z) != 0 ||
                            lowZ(x, y, z) != 0 || highZ(x, y, z) != 0;
        output(x, y, z) = select(marked, vnew, input(x, y, z));
    }

    void schedule() {
        const Target t = get_target();
        const bool gpu = t.has_gpu_feature();
        const int vec = t.natural_vector_size(UInt(8));
        // Serial axis: `r`; the two others (a, b) run in parallel. GPU: a block per 32x4 lines, r inside the thread.
        // mode: 0 x-scan, 1 y-scan, 2 z-scan
        auto scan = [&](Func f, RDom r, Var a, Var b, int mode) {
            f.compute_root();
            if (gpu) {
                Var xi{"xi"}, yi{"yi"}, zi{"zi"}, ao{"ao"}, ai{"ai"}, bo{"bo"}, bi{"bi"};
                f.gpu_tile(x, y, z, xi, yi, zi, 8, 8, 1, TailStrategy::GuardWithIf);
                f.update().split(a, ao, ai, 32, TailStrategy::GuardWithIf).split(b, bo, bi, 4, TailStrategy::GuardWithIf)
                    .reorder(r.x, ai, bi, ao, bo).gpu_blocks(ao, bo).gpu_threads(ai, bi);
            } else {
                f.parallel(z).vectorize(x, vec, TailStrategy::GuardWithIf);
                if (mode == 0) f.update().reorder(r.x, a, b).parallel(b);       // line by line, layers in parallel
                else if (mode == 1) f.update().reorder(a, r.x, b).parallel(b);  // x contiguous inside, layers in parallel
                else f.update().reorder(a, b, r.x).parallel(b);                 // layer by layer, rows in parallel
            }
        };
        scan(lowX, rx, y, z, 0);
        scan(highX, rx, y, z, 0);
        scan(lowY, ry, x, z, 1);
        scan(highY, ry, x, z, 1);
        scan(lowZ, rz, x, y, 2);
        scan(highZ, rz, x, y, 2);
        vxhl_gen::schedule_stencil(output, t, input.type());
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(GrowOutside, growOutside)
