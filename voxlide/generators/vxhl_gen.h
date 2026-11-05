#pragma once
//! Shared pieces for voxlide Halide generators (build-time only).
//!
//! Kernel ABI, fixed for every stencil op so runtime.h can call any kernel the same way:
//!     int <op>_<cpu|gpu>_<type>(input, gnx, gny, gnz, <op scalars and images...>, output[, changes])
//! `input` covers the piece being processed plus its halo, `output` only the piece's
//! interior, both in GLOBAL coordinates (buffer mins may be non-zero). gnx/gny/gnz are the
//! whole image's size, so image-boundary rules never use the buffer extents: a z-slab's
//! edges are not image edges.

#include <cmath>
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

//! Halide's Vulkan backend ignores strict_float: its SPIR-V has no NoContraction decorations, so the driver
//! may fuse an f32 a*b + c into one fma (RADV does; it does not fuse f64), which rounds once where voxlib
//! rounds twice. The mutator routes every f32 product feeding an add or subtract through an integer OR with
//! `zero`, a runtime 0 the driver cannot fold, so the operand is no longer a multiply and cannot be fused.
class NoContraction : public Internal::IRMutator {
    Expr zero;  // UInt(32), 0 at run time
    Expr guard(const Expr& e) const {
        if (!e.as<Internal::Mul>() || !e.type().is_float() || e.type().bits() != 32) return e;
        const Type u = UInt(32, e.type().lanes());
        return reinterpret(e.type(), reinterpret(u, e) | cast(u, zero));
    }
public:
    explicit NoContraction(Expr z) : zero(std::move(z)) {}
    using IRMutator::visit;
    Expr visit(const Internal::Add* op) override {
        Expr a = mutate(op->a), b = mutate(op->b);
        return op->type.is_float() ? Internal::Add::make(guard(a), guard(b)) : Internal::Add::make(a, b);
    }
    Expr visit(const Internal::Sub* op) override {
        Expr a = mutate(op->a), b = mutate(op->b);
        return op->type.is_float() ? Internal::Sub::make(guard(a), guard(b)) : Internal::Sub::make(a, b);
    }
};
//! Applies NoContraction to the definitions of `fs` on GPU targets; `zero` is any runtime expression
//! that is 0 and that Halide cannot fold (min(gnx, 0): a buffer extent would not do, Halide knows it is >= 0).
inline void no_contraction(const std::vector<Func>& fs, const Target& t, Expr zero) {
    if (!t.has_gpu_feature()) return;
    NoContraction m(std::move(zero));
    for (Func f : fs) f.function().mutate(&m);
}

//! Default schedule for a stencil: CPU vectorises x (tails guarded, so any nx works) and
//! parallelises over chunks of 32 fused y·z rows (one row per task is too fine for cheap ops);
//! GPU uses 2D x–y tiles with z as a block dim. `perVoxel`: Funcs with update definitions
//! (RDom reductions over the neighbourhood) computed per output vector / GPU thread.
//! `gnx`: the kernel's gnx input, for no_contraction's runtime zero.
inline void schedule_stencil(Expr gnx, Func out, const Target& t, Type vtype, const std::vector<Func>& perVoxel = {}) {
    Var x = out.args()[0], y = out.args()[1], z = out.args()[2];
    if (t.has_gpu_feature()) {
        std::vector<Func> fs = perVoxel;
        fs.push_back(out);
        no_contraction(fs, t, min(gnx, 0));
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

//! A constant of float type `t` (the double `v` rounded to it, as C++'s conversion does).
inline Expr fconst(Type t, double v) { return Internal::make_const(t, v); }

//! {ulp above v, ulp below v} for a positive normal float32 / float64 `v` (float64: 1e-38 < v < 3e38).
inline std::pair<Expr, Expr> ulps(Expr v) {
    const Type t = v.type();
    Expr p;  // 2^floor(log2 v)
    if (t.bits() == 32) {
        p = reinterpret(t, reinterpret(UInt(32), v) & 0x7f800000u);
    } else {
        p = cast(t, reinterpret(Float(32), reinterpret(UInt(32), cast<float>(v)) & 0x7f800000u));
        p = select(p > v, p * fconst(t, 0.5), p);  // cast<float> may round up a binade
    }
    const Expr u = p * fconst(t, std::ldexp(1.0, -(t.bits() == 32 ? 23 : 52)));
    return {u, select(v == p, u * fconst(t, 0.5), u)};
}

//! Correctly rounded a / b for Float(32) or Float(64) expressions. Halide's Vulkan backend divides
//! with an error of an ulp or two on this driver (f64: 103/103 != 1, which breaks truncation of exact
//! quotients; f32 is a * rcp(b)), so on GPU targets the quotient is corrected: with the exact residual
//! e = fma(-c, |b|, |a|) a candidate c is moved by one ulp while |e| exceeds half an ulp of c times |b|
//! (three rounds; sign handled separately). Valid for quotients in about [1e-30, 1e30]; exact ties are
//! not rounded to even. CPU targets divide natively.
inline Expr divF(Expr a, Expr b, const Target& t) {
    if (!t.has_gpu_feature()) return a / b;
    const Type ft = a.type();
    const Expr aa = abs(a), bb = abs(b), h = fconst(ft, 0.5), zero = fconst(ft, 0.0);
    Expr c = aa / bb;
    for (int i = 0; i < 3; ++i) {
        const auto [u, um] = ulps(c);
        const Expr e = fma(-c, bb, aa);  // exact: aa - c*bb
        c = select(e > h * u * bb, c + u, select(e < -(h * um * bb), c - um, c));
    }
    const Expr q = select((a < zero) != (b < zero), -c, c);
    return select(aa == zero || bb == zero, a / b, q);
}
inline Expr div64(Expr a, Expr b, const Target& t) { return divF(a, b, t); }

//! Correctly rounded sqrt(s) for a Float(32) or Float(64) s >= 0. GPU targets: Halide's Vulkan backend
//! has no f64 sqrt and its f32 sqrt is not correctly rounded, so an estimate (f64: f32 sqrt + 2 Newton
//! steps) is corrected exactly: with e = fma(-c, c, s), c is the correctly rounded root iff
//! -c·u⁻ < e <= c·u (u, u⁻: the ulps above / below c; sqrt never hits a rounding midpoint).
inline Expr sqrtF(Expr s, const Target& t) {
    if (!t.has_gpu_feature()) return sqrt(s);
    const Type ft = s.type();
    const Expr h = fconst(ft, 0.5);
    Expr c;
    if (ft.bits() == 32) {
        c = sqrt(s);
    } else {
        c = cast(ft, sqrt(cast<float>(s)));
        for (int i = 0; i < 2; ++i) c = select(c > 0, h * (c + s / c), c);
    }
    for (int i = 0; i < 2; ++i) {
        const auto [u, um] = ulps(c);
        const Expr e = fma(-c, c, s);
        c = select(e > c * u, c + u, select(e <= -(c * um), c - um, c));
    }
    return select(s > 0, c, fconst(ft, 0.0));
}

//! x > t for a float `x` of type ft and a double t, as C++ compares a float with a double (promotes x):
//! x >= the smallest ft value above t, so no f64 is needed for ft = float32.
inline Expr gt_double(Expr x, double t) {
    if (x.type().bits() == 64) return x > fconst(x.type(), t);
    float a = float(t);
    if (double(a) <= t) a = std::nextafter(a, INFINITY);
    return x >= fconst(x.type(), a);
}

//! Second output of ops that return a change count: changes(z) = number of voxels of slice z
//! where `out` differs from `in` (the per-z partials are summed on the host, so they merge
//! across slabs). ABI: the kernel's last argument, after `output`. Partial sums per (x, z)
//! keep GPU reads coalesced (per-row sums were ~10× slower on the GPU).
//! Call define() from generate() and schedule() after schedule_stencil(). Valid only for ops where every changed voxel changes value.
struct ChangeCounts {
    Func cols{"change_cols"};  // per (x, z), summed over y: GPU threads read along x (coalesced)
    Var x{"cx"}, z{"cz"};
    void define(Func changes, Func out, Func in, Expr gnx, Expr gny) {
        RDom rx(0, gnx), ry(0, gny);
        cols(x, z) = 0;
        cols(x, z) += select(out(x, ry, z) != in(x, ry, z), 1, 0);
        changes(z) = sum(cols(rx, z));
    }
    void schedule(Func changes, const Target& t) {
        cols.compute_root();
        if (t.has_gpu_feature()) {
            Var xi{"cxi"}, zi{"czi"};
            cols.gpu_tile(x, z, xi, zi, 64, 1, TailStrategy::GuardWithIf);
            cols.update().gpu_tile(x, z, xi, zi, 64, 1, TailStrategy::GuardWithIf);
            changes.gpu_tile(z, zi, 32, TailStrategy::GuardWithIf);
        } else {
            const int vec = t.natural_vector_size(Int(32));
            cols.parallel(z).vectorize(x, vec, TailStrategy::GuardWithIf);
            cols.update().parallel(z).vectorize(x, vec, TailStrategy::GuardWithIf);
        }
    }
};

}  // namespace vxhl_gen
