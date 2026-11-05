// Connected components without atomics (Halide's Vulkan backend has none), for voxlib keepLargest
// (voxlib/voxelRegions.h): labels of the 6-connected components of the voxels in range, the number
// of counted voxels in each, the largest. Gather-only; all intermediate images are int32 and packed:
//   P(v) = -1 outside the range, else (label << 3) | code; label = the smallest raster index
//   x + nx·(y + ny·z) of v's component (the component order of voxlib's labelImage), code = 0 at the
//   root (label == own index), else the direction of v's parent, the 6-neighbour v took its label
//   from: 1 + 2·axis for −axis, 2 + 2·axis for +axis. Parents form a forest rooted at the roots:
//   a voxel only takes a label from a neighbour that already had it, so following parents always
//   moves back in time. (Indices < 2^28: the host checks.)
// Kernels:
//   cclInit<T>   (remap → i32) P = lo <= v <= hi ? index << 3 : -1
//   cclWeight<T> (remap → i32) W = lo <= v <= hi ? 1 : 0
//   cclLabel     (i32, change counts): one round of line scans in ±x, ±y, ±z (one kernel call): a
//                voxel takes the smaller label of its predecessor on the line (updated earlier in the
//                same scan); rounds repeat until nothing changes
//   cclSum       (i32, Aux par, wgt; change counts): S(v) = W(v) + Σ S(children of v) in the same
//                six scans; from S = W, rounds repeat until nothing changes: S(root) = the count
//   cclRoots     (i32, Aux par) R = S at the roots, else -1
//   cclBest      (reduce) stats(0, z) = max R of layer z, stats(1, z) = the smallest index with it
//   cclApply<T>  (Aux par) outside component `best`: mode 0 every voxel, mode 1 those in [lo, hi] become `value`

#include "vxhl_gen.h"

namespace {

using namespace Halide;

Expr lbl(Expr p) { return p >> 3; }  // arithmetic shift: -1 stays -1

//! Coordinates (x, y, z) with axis A replaced by p.
std::vector<Expr> on_axis(int A, Var x, Var y, Var z, Expr p) {
    std::vector<Expr> c{x, y, z};
    c[A] = p;
    return c;
}

//! Schedule of a line-scan stage f along axis A: CPU parallel over the outermost other axis; GPU one
//! thread per line.
void schedule_scan(int A, Func f, const Target& t, Var x, Var y, Var z) {
    const Var other1 = A == 0 ? y : x, other2 = A == 2 ? y : z;
    f.compute_root();
    if (t.has_gpu_feature()) {
        Var xi{"xi"}, yi{"yi"}, zi{"zi"}, ai{"ai"}, bi{"bi"};
        f.gpu_tile(x, y, z, xi, yi, zi, 8, 8, 1, TailStrategy::GuardWithIf);
        f.update().gpu_tile(other1, other2, ai, bi, 16, 4, TailStrategy::GuardWithIf);
    } else {
        f.parallel(z);
        f.update().parallel(other2);
    }
}
const int kAxis[6] = {0, 0, 1, 1, 2, 2}, kDir[6] = {1, -1, 1, -1, 1, -1};  // one round: ±x, ±y, ±z

//! One round of label scans (±x, ±y, ±z, each reading the previous one): a voxel takes the smaller
//! label of its predecessor on the line, updated earlier in the same scan.
class CclLabel : public Generator<CclLabel> {
public:
    VXHL_STENCIL_INPUTS
    Output<Buffer<void, 3>> output{"output"};  // int32 (output.type set by voxlide_add_op)
    Output<Buffer<int32_t, 1>> changes{"changes"};
    Var x{"x"}, y{"y"}, z{"z"};
    std::vector<Func> st;
    vxhl_gen::ChangeCounts cc;

    void generate() {
        const Expr g[3] = {gnx, gny, gnz};
        Func f0{"f0"};
        f0(x, y, z) = cast<int32_t>(input(x, y, z));
        Func prevF = f0;
        for (int k = 0; k < 6; ++k) {
            const int A = kAxis[k], d = kDir[k];
            Func f{"scan" + std::to_string(k)};
            f(x, y, z) = prevF(x, y, z);
            RDom r(0, g[A]);
            const Expr pos = d > 0 ? Expr(r) : g[A] - 1 - r, prev = pos - d;
            const Expr cur = f(on_axis(A, x, y, z, pos)), nb = f(on_axis(A, x, y, z, clamp(prev, 0, g[A] - 1)));
            const Expr take = prev >= 0 && prev < g[A] && cur >= 0 && nb >= 0 && lbl(nb) < lbl(cur);
            f(on_axis(A, x, y, z, pos)) = select(take, (lbl(nb) << 3) | (d > 0 ? 1 + 2 * A : 2 + 2 * A), cur);
            st.push_back(f);
            prevF = f;
        }
        output(x, y, z) = prevF(x, y, z);
        cc.define(changes, output, input, gnx, gny);
    }
    void schedule() {
        for (int k = 0; k < 6; ++k) schedule_scan(kAxis[k], st[k], get_target(), x, y, z);
        vxhl_gen::schedule_stencil(output, get_target(), Int(32));
        cc.schedule(changes, get_target());
    }
};

//! One round of subtree-sum scans: S(v) = W(v) + Σ S(children of v), the predecessor on the line read
//! after its own update in the same scan, the other neighbours from the previous scan.
class CclSum : public Generator<CclSum> {
public:
    VXHL_STENCIL_INPUTS
    Input<Buffer<int32_t, 3>> par{"par"}, wgt{"wgt"};  // Aux images (int32 for every op type)
    Output<Buffer<void, 3>> output{"output"};          // int32
    Output<Buffer<int32_t, 1>> changes{"changes"};
    Var x{"x"}, y{"y"}, z{"z"};
    std::vector<Func> st;
    vxhl_gen::ChangeCounts cc;

    void generate() {
        const Expr g[3] = {gnx, gny, gnz};
        Func f0{"f0"};
        f0(x, y, z) = cast<int32_t>(input(x, y, z));
        Func prevF = f0;
        for (int k = 0; k < 6; ++k) {
            const int A = kAxis[k], d = kDir[k];
            Func f{"sum" + std::to_string(k)};
            f(x, y, z) = prevF(x, y, z);
            RDom r(0, g[A]);
            const Expr pos = d > 0 ? Expr(r) : g[A] - 1 - r;
            const std::vector<Expr> c = on_axis(A, x, y, z, pos);
            Expr s = wgt(c[0], c[1], c[2]);
            for (int B = 0; B < 3; ++B)
                for (int sg : {-1, 1}) {  // the neighbour n = v + sg·e_B is a child if its parent is v
                    std::vector<Expr> nc = c;
                    const Expr q = c[B] + sg;
                    nc[B] = clamp(q, 0, g[B] - 1);
                    const Expr pn = par(nc[0], nc[1], nc[2]);
                    const Expr isChild = q >= 0 && q < g[B] && pn >= 0 && (pn & 7) == (sg > 0 ? 1 + 2 * B : 2 + 2 * B);
                    const Expr sn = (B == A && sg == -d) ? f(nc[0], nc[1], nc[2]) : prevF(nc[0], nc[1], nc[2]);
                    s += select(isChild, sn, 0);
                }
            f(c[0], c[1], c[2]) = select(par(c[0], c[1], c[2]) >= 0, s, 0);
            st.push_back(f);
            prevF = f;
        }
        output(x, y, z) = prevF(x, y, z);
        cc.define(changes, output, input, gnx, gny);
    }
    void schedule() {
        for (int k = 0; k < 6; ++k) schedule_scan(kAxis[k], st[k], get_target(), x, y, z);
        vxhl_gen::schedule_stencil(output, get_target(), Int(32));
        cc.schedule(changes, get_target());
    }
};

class CclInit : public Generator<CclInit> {
public:
    VXHL_STENCIL_INPUTS
    Input<Expr> lo{"lo"}, hi{"hi"};
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};
    void generate() {
        const Expr v = input(x, y, z);
        output(x, y, z) = select(lo <= v && v <= hi, (x + gnx * (y + gny * z)) << 3, -1);
    }
    void schedule() { vxhl_gen::schedule_stencil(output, get_target(), Int(32)); }
};

class CclWeight : public Generator<CclWeight> {
public:
    VXHL_STENCIL_INPUTS
    Input<Expr> lo{"lo"}, hi{"hi"};
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};
    void generate() {
        const Expr v = input(x, y, z);
        output(x, y, z) = select(lo <= v && v <= hi, 1, 0);
    }
    void schedule() { vxhl_gen::schedule_stencil(output, get_target(), Int(32)); }
};

class CclRoots : public Generator<CclRoots> {
public:
    VXHL_STENCIL_INPUTS
    Input<Buffer<int32_t, 3>> par{"par"};
    Output<Buffer<void, 3>> output{"output"};  // int32 (output.type set by voxlide_add_op)
    Var x{"x"}, y{"y"}, z{"z"};
    void generate() {
        const Expr p = par(x, y, z);
        output(x, y, z) = select(p >= 0 && (p & 7) == 0, cast<int32_t>(input(x, y, z)), -1);
    }
    void schedule() { vxhl_gen::schedule_stencil(output, get_target(), Int(32)); }
};

class CclBest : public Generator<CclBest> {
public:
    VXHL_STENCIL_INPUTS
    Output<Buffer<double, 2>> stats{"stats"};  // run_reduce ABI: stats(i, z)
    Var x{"x"}, z{"z"}, c{"c"};
    Func colMax{"colMax"}, layerMax{"layerMax"}, colIdx{"colIdx"}, layerIdx{"layerIdx"};
    void generate() {
        RDom rx(0, gnx), ry(0, gny);
        const Expr big = Int(32).max();
        colMax(x, z) = Int(32).min();
        colMax(x, z) = max(colMax(x, z), cast<int32_t>(input(x, ry, z)));
        layerMax(z) = Int(32).min();
        layerMax(z) = max(layerMax(z), colMax(rx, z));
        colIdx(x, z) = big;
        colIdx(x, z) = select(cast<int32_t>(input(x, ry, z)) == layerMax(z), min(colIdx(x, z), x + gnx * (ry + gny * z)), colIdx(x, z));
        layerIdx(z) = big;
        layerIdx(z) = min(layerIdx(z), colIdx(rx, z));
        stats(c, z) = cast<double>(select(c == 0, layerMax(z), layerIdx(z)));
    }
    void schedule() {
        stats.bound(c, 0, 2).unroll(c);
        for (Func f : {colMax, layerMax, colIdx, layerIdx}) f.compute_root();
        if (get_target().has_gpu_feature()) {
            Var xi{"xi"}, zi{"zi"};
            for (Func f : {colMax, colIdx}) {
                f.gpu_tile(x, z, xi, zi, 64, 1, TailStrategy::GuardWithIf);
                f.update().gpu_tile(x, z, xi, zi, 64, 1, TailStrategy::GuardWithIf);
            }
            for (Func f : {layerMax, layerIdx}) {
                f.gpu_tile(z, zi, 32, TailStrategy::GuardWithIf);
                f.update().gpu_tile(z, zi, 32, TailStrategy::GuardWithIf);
            }
            stats.gpu_tile(z, zi, 32, TailStrategy::GuardWithIf);
        } else {
            for (Func f : {colMax, colIdx}) f.parallel(z), f.update().parallel(z);
        }
    }
};

class CclApply : public Generator<CclApply> {
public:
    VXHL_STENCIL_INPUTS
    Input<Buffer<int32_t, 3>> par{"par"};
    Input<int32_t> best{"best"}, mode{"mode"};
    Input<Expr> lo{"lo"}, hi{"hi"}, value{"value"};
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};
    void generate() {  // mode 0 (keepLargest): every other voxel; 1 (keepLargestvv): the others in [lo, hi]
        const Expr p = par(x, y, z), v = input(x, y, z);
        const Expr reset = !(p >= 0 && lbl(p) == best) && (mode == 0 || (lo <= v && v <= hi));
        output(x, y, z) = select(reset, value, v);
    }
    void schedule() { vxhl_gen::schedule_stencil(output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(CclInit, cclInit)
HALIDE_REGISTER_GENERATOR(CclWeight, cclWeight)
HALIDE_REGISTER_GENERATOR(CclLabel, cclLabel)
HALIDE_REGISTER_GENERATOR(CclSum, cclSum)
HALIDE_REGISTER_GENERATOR(CclRoots, cclRoots)
HALIDE_REGISTER_GENERATOR(CclBest, cclBest)
HALIDE_REGISTER_GENERATOR(CclApply, cclApply)
