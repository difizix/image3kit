// Halide port of ::segmentVote(vImg, origimg, grad, meds, capmin, capmax, krnl, localF, flatnes,
// gradFactor, resolutionSqr, avgGradInv, noisv), voxlib/voxelImageML.h (one segment2 voting pass).
// Kernel args: input = vImg (read as voxlib's copy vxls), orig/grad = Aux images, med[0..nSegs) =
// meds (nSegs <= kMaxSegs; the wrapper falls back to voxlib otherwise). Semantics reproduced
// exactly (in voxlib's f32_t: generator param ftype; strict_float):
//  - only voxels >= krnl from every face with capmin <= orig <= capmax are examined;
//  - seg(v) = the last s with meds[s] == v, else -1 (voxlib's midIs);
//  - per segment s: meanSum = 0, sumW = 1e-15; if seg(centre) == s: meanSum -= 0.999·orig(c),
//    sumW -= 0.999; then over the (2krnl+1)³ neighbours n (k, j, i order) with seg(n) == s:
//    w = rg / ((d² + r1) + (gF·gj)·gj), gj = avgGradInv·grad(n), rg = r1 + (gF·g)·g,
//    g = grad(c), r1 = resolutionSqr − 0.99: meanSum += w·orig(n), sumW += w;
//  - vid = first argmin over s with sumW > 1e-9 of
//    ((|meanSum/sumW − ov|·localF + (1 − localF)·|meds[s] − ov|) + noisv) / min(1 + flatnes·sumW, krnlCub),
//    starting from vid = 0, prob = max(f32_t) (the thresholds compare in double, as C++ does);
//  - the voxel becomes meds[vid] if sumW[vid] > 0.01 and meds[vid] != vxls(c).
// Second output: per-z change counts (voxlib returns nChange).

#include "vxhl_gen.h"

namespace {

using namespace Halide;
constexpr int kMaxSegs = 8;

class SegmentVote : public Generator<SegmentVote> {
public:
    GeneratorParam<Type> ftype{"ftype", Float(64)};  // voxlib f32_t (F32T)
    VXHL_STENCIL_INPUTS
    Input<Buffer<void, 3>> orig{"orig"}, grad{"grad"};  // Aux images (IMAGES)
    Input<int32_t> krnl{"krnl"}, nSegs{"nSegs"};
    Input<int32_t[kMaxSegs]> med{"med"};
    Input<int32_t> capmin{"capmin"}, capmax{"capmax"};
    Input<Expr> localF{"localF"}, flatnes{"flatnes"}, gradFactor{"gradFactor"}, resolSqr_1{"resolSqr_1"},
        avgGradInv{"avgGradInv"}, noisv{"noisv"}, krnlCub{"krnlCub"};  // ftype (FLOATS)
    Output<Buffer<void, 3>> output{"output"};
    Output<Buffer<int32_t, 1>> changes{"changes"};

    Var x{"x"}, y{"y"}, z{"z"}, s{"s"};
    Func seg{"seg"}, acc{"acc"}, best{"best"};
    vxhl_gen::ChangeCounts cc;

    Expr flt(double v) { return vxhl_gen::fconst(ftype, v); }
    //! f32 GPU division is a·rcp(b): corrected (vxhl_gen::divF); f64 keeps the native division (the f64 port's
    //! choice: correcting it would slow the f64 vote further, and its error has not changed a label in the tests).
    Expr div(Expr a, Expr b) { return a.type().bits() == 32 ? vxhl_gen::divF(a, b, get_target()) : a / b; }
    Expr medOf(Expr i) {
        std::vector<Expr> m;
        for (int k = 0; k < kMaxSegs; ++k) m.push_back(med[k]);
        return mux(i, m);
    }

    void generate() {
        const Type ft = ftype;
        Func in = BoundaryConditions::repeat_edge(input);
        Func og = BoundaryConditions::repeat_edge(orig), gr = BoundaryConditions::repeat_edge(grad);

        Expr idx = Expr(int32_t(-1));
        const Expr v = cast<int32_t>(in(x, y, z));
        for (int k = 0; k < kMaxSegs; ++k) idx = select(k < nSegs && v == med[k], k, idx);
        seg(x, y, z) = idx;

        const Expr ovI = cast<int32_t>(og(x, y, z));
        const Expr ovD = cast(ft, og(x, y, z));
        const Expr g = cast(ft, gr(x, y, z));
        const Expr rg = resolSqr_1 + (gradFactor * g) * g;

        const Expr isC = seg(x, y, z) == s;
        acc(x, y, z, s) = {select(isC, flt(0.0) - flt(0.999) * ovD, flt(0.0)), select(isC, flt(1e-15) - flt(0.999), flt(1e-15))};
        RDom r(0, 2 * krnl + 1, 0, 2 * krnl + 1, 0, 2 * krnl + 1);  // r.x innermost = voxlib's i loop
        const Expr di = r.x - krnl, dj = r.y - krnl, dk = r.z - krnl;
        const Expr nx = x + di, ny = y + dj, nz = z + dk;
        const Expr gj = avgGradInv * cast(ft, gr(nx, ny, nz));
        const Expr w = div(rg, (cast(ft, dk * dk + dj * dj + di * di) + resolSqr_1) + (gradFactor * gj) * gj);
        const Expr sn = seg(nx, ny, nz);
        if (get_target().has_gpu_feature()) {
            // GPU (f64 is slow there, so the division per neighbour dominates): w does not depend on the segment,
            // compute it once per neighbour and add it to the slot of the neighbour's segment (slot kMaxSegs takes
            // the neighbours in no segment). Each segment still sums its neighbours in (k, j, i) order, so the sums
            // are those of voxlib's per-segment loops. max: bounds inference sees sn >= -1.
            const Expr slot = select(sn >= 0, max(sn, 0), Expr(int32_t(kMaxSegs)));
            acc(x, y, z, slot) = {acc(x, y, z, slot)[0] + w * cast(ft, og(nx, ny, nz)), acc(x, y, z, slot)[1] + w};
        } else {
            // CPU: one vectorised pass per segment (a data-dependent slot would not vectorise: 1.5x slower)
            const Expr hit = sn == s;
            acc(x, y, z, s) = {acc(x, y, z, s)[0] + select(hit, w * cast(ft, og(nx, ny, nz)), flt(0.0)),
                               acc(x, y, z, s)[1] + select(hit, w, flt(0.0))};
        }

        best(x, y, z) = {Expr(int32_t(0)), ft.max(), acc(x, y, z, 0)[1]};  // vid, prob, sumW[vid]
        RDom rs(0, nSegs);
        const Expr mS = acc(x, y, z, rs)[0], sW = acc(x, y, z, rs)[1];
        const Expr prob = div((abs(div(mS, sW) - ovD) * localF + (flt(1.0) - localF) * cast(ft, abs(medOf(rs) - ovI))) + noisv,
                              min(flt(1.0) + flatnes * sW, krnlCub));
        const Expr better = vxhl_gen::gt_double(sW, 1e-9) && prob < best(x, y, z)[1];
        best(x, y, z) = {select(better, rs, best(x, y, z)[0]), select(better, prob, best(x, y, z)[1]),
                         select(better, sW, best(x, y, z)[2])};

        const Expr vid = best(x, y, z)[0], nv = medOf(vid);
        const Expr change = vxhl_gen::interior(x, y, z, gnx, gny, gnz, krnl) && capmin <= ovI && ovI <= capmax &&
                            vxhl_gen::gt_double(best(x, y, z)[2], 0.01) && nv != v;
        output(x, y, z) = select(change, cast(input.type(), nv), in(x, y, z));
        cc.define(changes, output, input, gnx, gny);
    }

    void schedule() {
        const Target t = get_target();
        seg.compute_root();
        if (t.has_gpu_feature()) {
            Var xi{"xi"}, yi{"yi"};
            seg.gpu_tile(x, y, xi, yi, 16, 8, TailStrategy::GuardWithIf);
        } else {
            seg.parallel(z).vectorize(x, t.natural_vector_size(Int(32)), TailStrategy::GuardWithIf);
        }
        if (t.has_gpu_feature()) acc.bound(s, 0, kMaxSegs + 1).bound_storage(s, kMaxSegs + 1);
        else acc.bound_storage(s, kMaxSegs);
        vxhl_gen::schedule_stencil(gnx, output, t, Type(ftype), {acc, best});
        cc.schedule(changes, t);
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(SegmentVote, segmentVote)
