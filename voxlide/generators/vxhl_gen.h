#pragma once
//! Shared pieces for voxlide Halide generators (build-time only).
//!
//! Kernel ABI, fixed for every stencil op so runtime.h can call any kernel the same way:
//!     int <op>_<cpu|gpu>_<type>(input, gnx, gny, gnz, <op scalars and images...>, output[, changes])
//! `input` covers the piece being processed plus its halo, `output` only the piece's
//! interior, both in GLOBAL coordinates (buffer mins may be non-zero). gnx/gny/gnz are the
//! whole image's size, so image-boundary rules never use the buffer extents: a z-slab's
//! edges are not image edges.

#include <vector>

#include "Halide.h"

//! Declare as the first members of every stencil generator, before the op's own Inputs.
#define VXHL_STENCIL_INPUTS                                  \
    Input<Buffer<void, 3>> input{"input"};                   \
    Input<int32_t> gnx{"gnx"}, gny{"gny"}, gnz{"gnz"};

namespace vxhl_gen {

using namespace Halide;

//! True where (x,y,z) is at least `r` voxels from every global image face,
//! i.e. the region voxlib's forAllkji_m_seq(r, …) loops visit.
inline Expr interior(Expr x, Expr y, Expr z, Expr gnx, Expr gny, Expr gnz, Expr r) {
    return x >= r && x < gnx - r && y >= r && y < gny - r && z >= r && z < gnz - r;
}

//! C++ integer division (truncates toward zero) for a runtime int32 `a` and positive constant `b`.
inline Expr cdiv(Expr a, int b) {
    return select(a >= 0, a / b, -((-a) / b));
}

//! Default schedule for a stencil: CPU vectorises x (tails guarded, so any nx works) and
//! parallelises over chunks of 32 fused y·z rows (one row per task is too fine for cheap ops);
//! GPU uses 2D x–y tiles with z as a block dim. `perVoxel`: Funcs with update definitions
//! (RDom reductions over the neighbourhood) computed per output vector / GPU thread.
inline void schedule_stencil(Func out, const Target& t, Type vtype, const std::vector<Func>& perVoxel = {}) {
    Var x = out.args()[0], y = out.args()[1], z = out.args()[2];
    if (t.has_gpu_feature()) {
        Var xi{"xi"}, yi{"yi"}, zi{"zi"};
        out.gpu_tile(x, y, z, xi, yi, zi, 8, 8, 1, TailStrategy::GuardWithIf);
        for (Func f : perVoxel) f.compute_at(out, xi);
    } else {
        const int vec = t.natural_vector_size(vtype);
        Var yz{"yz"}, yzo{"yzo"}, yzi{"yzi"}, xo{"xo"}, xi{"xi"};
        out.fuse(y, z, yz).split(yz, yzo, yzi, 32, TailStrategy::GuardWithIf).parallel(yzo)
            .split(x, xo, xi, vec, TailStrategy::GuardWithIf).vectorize(xi);
        for (Func f : perVoxel) {
            const Var fx = f.args()[0];
            f.compute_at(out, xo).vectorize(fx, vec, TailStrategy::GuardWithIf);
            for (int u = 0; u < f.num_update_definitions(); ++u)
                f.update(u).vectorize(fx, vec, TailStrategy::GuardWithIf);
        }
    }
}

//! Second output of ops that return a change count: changes(z) = number of voxels of slice z
//! where `out` differs from `in` (the per-z partials are summed on the host, so they merge
//! across slabs). ABI: the kernel's last argument, after `output`. Uses per-row partial sums
//! so the GPU reduction is parallel over (y,z). Call define() from generate() and schedule()
//! after schedule_stencil(). Valid only for ops where every changed voxel changes value.
struct ChangeCounts {
    Func rows{"change_rows"};
    Var y{"cy"}, z{"cz"};
    void define(Func changes, Func out, Func in, Expr gnx, Expr gny) {
        RDom rx(0, gnx), ry(0, gny);
        rows(y, z) = 0;
        rows(y, z) += select(out(rx, y, z) != in(rx, y, z), 1, 0);
        changes(z) = sum(rows(ry, z));
    }
    void schedule(Func changes, const Target& t) {
        rows.compute_root();
        if (t.has_gpu_feature()) {
            Var yi{"cyi"}, zi{"czi"};
            rows.gpu_tile(y, z, yi, zi, 8, 8, TailStrategy::GuardWithIf);
            rows.update().gpu_tile(y, z, yi, zi, 8, 8, TailStrategy::GuardWithIf);
            changes.gpu_tile(z, zi, 32, TailStrategy::GuardWithIf);
        } else {
            rows.parallel(z);
            rows.update().parallel(z);
        }
    }
};

}  // namespace vxhl_gen
