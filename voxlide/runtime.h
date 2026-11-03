#pragma once
//! voxlide internals shared by every op: backend resolution, Halide buffer views,
//! the slab/iteration driver and the Auto fallback. Ops call detail::run_stencil and nothing else.
//! This is the only header that switches on IMAGE3KIT_HAS_HALIDE.

#include <array>
#include <atomic>
#include <tuple>
#include <vector>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "voxlide.h"
#include "ops_table.h"
#include "voxelBox.h"

#ifdef IMAGE3KIT_HAS_HALIDE
#include "HalideBuffer.h"
#include "HalideRuntime.h"
#endif

namespace vxhl::detail {

struct Choice {
    Backend backend;
    bool strict;  //!< the user asked for this backend explicitly: failures throw
};

//! The only place the requested backend (set_backend / $IMAGE3KIT_BACKEND) is read.
Choice resolve(const OpTraits& op, bool hasCpu, bool hasGpu, bool gpuBroken);
std::size_t max_slab_bytes(Backend b);
std::string take_halide_error();  //!< message captured by voxlide's Halide error handler

//! Kernel argument (passed among an op's scalars) that becomes a read-only image buffer
//! holding the image as it was at the start of every `every` kernel passes, e.g. the
//! original image of each outer iteration of biLateralIterative. Costs no copy.
struct Snapshot {
    int every;
};

//! How run_stencil applies the kernel: n passes, each reading `radius` voxels around the
//! output (-1: the op's traits radius). Converts from an int pass count.
struct Passes {
    int n = 1;
    int radius = -1;
    Passes(int n_, int r = -1) : n(n_), radius(r) {}
};
template <typename S> constexpr int snapshot_every(const S&) { return 0; }
constexpr int snapshot_every(const Snapshot& s) { return s.every; }

#ifdef IMAGE3KIT_HAS_HALIDE
template <typename T> using HBuf = Halide::Runtime::Buffer<T, 3>;
using CBuf = Halide::Runtime::Buffer<int32_t, 1>;  //!< per-z change counts
#endif

//! Kernel argument (passed among an op's scalars) that becomes a read-only image buffer
//! holding another image of the same size and type (e.g. shrink's source image, a mask, a
//! gradient). Each kernel call gets the z-layers its `input` covers (slab + halo). Within one
//! run_stencil call the buffer is reused, so on the GPU it is uploaded once for all passes.
//! Construct it in the wrapper's run_stencil call so it lives until that call returns.
template <typename T>
struct Aux {
    const VoxelImageT<T>& img;
#ifdef IMAGE3KIT_HAS_HALIDE
    mutable HBuf<T> buf{};
#endif
};

#ifdef IMAGE3KIT_HAS_HALIDE

//! z-layers [kBgn,kEnd) of `f` as a Halide buffer in global coordinates (zero-copy).
template <typename T>
HBuf<T> zview(VoxelField<T>& f, int kBgn, int kEnd) {
    const int3 n = f.size3();
    HBuf<T> b(f.data() + (long long)kBgn * n.x * n.y, n.x, n.y, kEnd - kBgn);
    b.set_min(0, 0, kBgn);
    return b;
}

//! The view of `a` covering the z-layers of `in` (cached: the same layers reuse the buffer).
template <typename T>
halide_buffer_t* aux_view(const Aux<T>& a, const HBuf<T>& in, int3 g) {
    if (a.img.size3() != g)
        throw std::invalid_argument("vxhl: auxiliary image size differs from the image");
    const int kBgn = in.dim(2).min(), kEnd = kBgn + in.dim(2).extent();
    if (!a.buf.data() || a.buf.dim(2).min() != kBgn || a.buf.dim(2).extent() != kEnd - kBgn) {
        a.buf = zview(const_cast<VoxelImageT<T>&>(a.img), kBgn, kEnd);  // read only
        a.buf.set_host_dirty();  // uploaded on the first GPU use; ignored on the CPU
    }
    return a.buf.raw_buffer();
}

template <typename T, typename S>
auto kernel_arg(const HBuf<T>& in, int3 g, HBuf<T>* snap, const S& s) {
    if constexpr (std::is_same_v<S, Snapshot>) return snap->raw_buffer();
    else if constexpr (std::is_same_v<S, Aux<T>>) return aux_view(s, in, g);
    else return s;
}

//! Call kernel `fn` (ABI: generators/vxhl_gen.h) reading `in` (+ `snap`), writing `out`.
//! Count: the kernel also fills `cnt` (vxhl_gen::ChangeCounts, one int32 per z of `out`).
template <bool Count, typename Fn, typename T, typename... S>
void call_kernel(Fn fn, HBuf<T>& in, HBuf<T>* snap, HBuf<T>& out, CBuf& cnt, int3 g, const S&... s) {
    int rc;
    if constexpr (Count) rc = fn(in.raw_buffer(), g.x, g.y, g.z, kernel_arg(in, g, snap, s)..., out.raw_buffer(), cnt.raw_buffer());
    else                 rc = fn(in.raw_buffer(), g.x, g.y, g.z, kernel_arg(in, g, snap, s)..., out.raw_buffer());
    if (rc != 0) throw std::runtime_error(take_halide_error() + " (halide error " + std::to_string(rc) + ")");
}

//! Add the per-z counts a kernel wrote to `total`.
inline void add_counts(CBuf& cb, bool gpu, long long& total) {
    if (gpu && cb.copy_to_host() != 0) throw std::runtime_error(take_halide_error());
    for (int z = cb.dim(0).min(); z <= cb.dim(0).max(); ++z) total += cb(z);
}

//! One box of one pass: read box+halo of `in` (and `snap`), write the box interior of `out`.
template <bool Count, typename Fn, typename T, typename... S>
void invoke_box(Fn fn, bool gpu, VoxelField<T>& in, VoxelField<T>* snap, VoxelField<T>& out, const Box& b,
                long long* total, const S&... s) {
    auto ib = zview(in, b.kBgnRead(), b.kEndRead());
    auto ob = zview(out, b.kBgn(), b.kEnd());
    HBuf<T> sb = snap ? zview(*snap, b.kBgnRead(), b.kEndRead()) : HBuf<T>();
    if (gpu) { ib.set_host_dirty(); if (snap) sb.set_host_dirty(); }
    CBuf cb;
    if constexpr (Count) { cb = CBuf(b.kEnd() - b.kBgn()); cb.set_min(b.kBgn()); }
    call_kernel<Count>(fn, ib, snap ? &sb : nullptr, ob, cb, b.nGlobal, s...);
    if (gpu && ob.copy_to_host() != 0) throw std::runtime_error(take_halide_error());
    if constexpr (Count) add_counts(cb, gpu, *total);
}

//! Run kernel `fn` nIter times over the whole image. Images rotate between `img` (read
//! only, so a failure leaves it intact for the Auto fallback) and up to 3 work buffers,
//! so Snapshot arguments never copy.
//!  - one piece: on the GPU all buffers stay on the device: one upload, one download;
//!  - image larger than max_slab_bytes(): every pass is a pass over z-slabs with a halo
//!    of the op's radius, through host memory.
template <typename Op, bool Count, typename Fn, typename T, typename... S>
void run_kernel(Fn fn, Backend b, VoxelImageT<T>& img, int nIter, int radius, long long* total, const S&... s) {
    *total = 0;
    const int3 n = img.size3();
    const bool gpu = b == Backend::Gpu;
    int every = 0;
    ((every = std::max(every, snapshot_every(s))), ...);
    const long long bytes = (long long)n.x * n.y * n.z * (long long)sizeof(T);
    const long long lim = (long long)max_slab_bytes(b);
    const int nSlabs = int((bytes + lim - 1) / lim);

    VoxelField<T> work[3];
    VoxelField<T>* f[4] = {&img, &work[0], &work[1], &work[2]};
    auto pick = [&](int a, int c) {  // a work buffer that is neither the current image nor the snapshot
        int i = 1;
        while (i == a || i == c) ++i;
        if (f[i]->size3() != n) f[i]->reset(n);
        return i;
    };
    int cur = 0, snap = 0;
    if (nSlabs <= 1) {
        HBuf<T> v[4];
        auto view = [&](int i) -> HBuf<T>& {
            if (!v[i].data()) v[i] = zview(*f[i], 0, n.z);
            return v[i];
        };
        if (gpu) view(0).set_host_dirty();
        for (int p = 0; p < nIter; ++p) {
            if (every && p % every == 0) snap = cur;
            const int out = pick(cur, snap);
            CBuf cb;
            if constexpr (Count) cb = CBuf(n.z);
            call_kernel<Count>(fn, view(cur), every ? &view(snap) : nullptr, view(out), cb, n, s...);
            if constexpr (Count) add_counts(cb, gpu, *total);
            cur = out;
            if (gpu && snap != 0) v[0].device_free();  // the input is not read again
        }
        if (gpu && view(cur).copy_to_host() != 0) throw std::runtime_error(take_halide_error());
    } else {
        const auto boxes = splitZ(n, nSlabs, radius);
        for (int p = 0; p < nIter; ++p) {
            if (every && p % every == 0) snap = cur;
            const int out = pick(cur, snap);
            for (const Box& box : boxes) invoke_box<Count>(fn, gpu, *f[cur], every ? f[snap] : nullptr, *f[out], box, total, s...);
            cur = out;
        }
    }
    img.data_ = std::move(f[cur]->data_);
}
#endif

//! Per-z partial results of a reduction kernel: N outputs, each one double per z-layer.
//! Partials merge exactly across slabs (and later MPI ranks); the op combines them on the host.
template <std::size_t N> using PerZ = std::array<std::vector<double>, N>;

#ifdef IMAGE3KIT_HAS_HALIDE
using DBuf = Halide::Runtime::Buffer<double, 1>;

//! One reduction kernel call on the z-layers of `in`; ABI (generators/vxhl_gen.h):
//!     int <op>_<dev>_<type>(input, gnx, gny, gnz, <scalars...>, out_0, …, out_{N-1})
template <std::size_t N, typename Fn, typename T, typename... S>
void reduce_box(Fn fn, bool gpu, HBuf<T>& in, int3 g, PerZ<N>& out, const S&... s) {
    const int k0 = in.dim(2).min(), nk = in.dim(2).extent();
    std::array<DBuf, N> ob;
    for (auto& b : ob) { b = DBuf(nk); b.set_min(k0); }
    const int rc = std::apply([&](auto&... b) { return fn(in.raw_buffer(), g.x, g.y, g.z, s..., b.raw_buffer()...); }, ob);
    if (rc != 0) throw std::runtime_error(take_halide_error() + " (halide error " + std::to_string(rc) + ")");
    for (std::size_t i = 0; i < N; ++i) {
        if (gpu && ob[i].copy_to_host() != 0) throw std::runtime_error(take_halide_error());
        for (int k = k0; k < k0 + nk; ++k) out[i][k] = ob[i](k);
    }
}
#endif

template <typename Op, typename K> std::atomic<bool>& gpu_broken() {  // per op × type
    static std::atomic<bool> broken{false};
    return broken;
}

//! Backend selection + Auto fallback shared by all drivers: runs attempt(kernel, backend) on
//! the resolved Halide backend; a failure throws for explicit backends and, under Auto, marks
//! the GPU broken for this op × type and moves on (GPU → Halide CPU → voxlib). ref(): voxlib.
template <typename Op, typename K, typename Ref, typename Attempt>
auto dispatch(Ref&& ref, [[maybe_unused]] Attempt&& attempt) -> decltype(ref()) {
    constexpr bool hasCpu = !std::is_null_pointer_v<std::remove_cv_t<decltype(K::cpu)>>;
    constexpr bool hasGpu = !std::is_null_pointer_v<std::remove_cv_t<decltype(K::gpu)>>;
    [[maybe_unused]] Choice c = resolve(Op::traits, hasCpu, hasGpu, gpu_broken<Op, K>());
#ifdef IMAGE3KIT_HAS_HALIDE
    while (c.backend != Backend::Voxlib) {
        try {
            if constexpr (hasGpu) if (c.backend == Backend::Gpu) return attempt(K::gpu, c.backend);
            if constexpr (hasCpu) if (c.backend == Backend::HalideCpu) return attempt(K::cpu, c.backend);
            throw std::logic_error("no kernel for resolved backend");
        } catch (const std::exception& e) {
            if (c.strict) throw std::runtime_error(std::string("vxhl::") + Op::traits.name + " on " +
                                                   backend_name(c.backend) + ": " + e.what());
            std::cerr << "voxlide: " << Op::traits.name << " " << backend_name(c.backend)
                      << " failed, falling back: " << e.what() << std::endl;
            if (c.backend == Backend::Gpu) {
                gpu_broken<Op, K>() = true;
                c.backend = hasCpu ? Backend::HalideCpu : Backend::Voxlib;
            } else {
                c.backend = Backend::Voxlib;
            }
        }
    }
#endif
    return ref();
}

//! The one implementation every ported stencil op goes through.
//! `count` (run_stencil_count): the sum, over all passes, of the kernel's changed-voxel counts.
//! Op: vxhl::op::<name> (traits); K: kernels::<name><T> (generated, cpu/gpu may be nullptr);
//! ps: kernel passes (iterated ops; 1 otherwise) and, for parameter-dependent radii, the halo; ref: the voxlib reference,
//! applied in place, doing all nIter iterations itself; s: the kernel's op scalars and
//! Snapshot{} / Aux<T>{} image arguments, in the generator's Input order.
template <typename Op, typename K, bool Count, typename T, typename Ref, typename... S>
void run_stencil_impl(VoxelImageT<T>& img, Passes ps, long long* count, Ref&& ref, const S&... s) {
    if (ps.n <= 0) return;
    dispatch<Op, K>([&] { ref(img); }, [&]([[maybe_unused]] auto fn, [[maybe_unused]] Backend b) {
#ifdef IMAGE3KIT_HAS_HALIDE
        run_kernel<Op, Count>(fn, b, img, ps.n, ps.radius >= 0 ? ps.radius : Op::traits.radius, count, s...);
#else
        (void)count, ((void)s, ...);
#endif
    });
}

//! Reductions (class D): `body(pass)` computes the op's result, calling `pass(scalars...)`
//! (one kernel run over the whole image, slab by slab) as often as it needs; it returns the
//! kernel's N outputs per z (PerZ<N>), which `body` combines. On the GPU a one-piece image is
//! uploaded once for all passes. ref(): the voxlib function. The image is never modified.
template <typename Op, typename K, std::size_t N, typename T, typename Ref, typename Body>
auto run_reduce(const VoxelImageT<T>& img, Ref&& ref, [[maybe_unused]] Body&& body) -> decltype(ref()) {
    const int3 n = img.size3();
    if ((long long)n.x * n.y * n.z == 0) return ref();
    return dispatch<Op, K>(ref, [&]([[maybe_unused]] auto fn, [[maybe_unused]] Backend b) -> decltype(ref()) {
#ifdef IMAGE3KIT_HAS_HALIDE
        const bool gpu = b == Backend::Gpu;
        auto& f = const_cast<VoxelImageT<T>&>(img);  // read only
        const long long bytes = (long long)n.x * n.y * n.z * (long long)sizeof(T);
        const long long lim = (long long)max_slab_bytes(b);
        const auto boxes = splitZ(n, int((bytes + lim - 1) / lim), 0);
        HBuf<T> whole;  // kept (and on the GPU, resident) across passes when there is one box
        auto pass = [&](const auto&... s) {
            PerZ<N> out;
            for (auto& v : out) v.assign(n.z, 0.0);
            for (const Box& box : boxes) {
                HBuf<T> local;
                HBuf<T>& in = boxes.size() == 1 ? whole : local;
                if (!in.data()) { in = zview(f, box.kBgn(), box.kEnd()); if (gpu) in.set_host_dirty(); }
                reduce_box<N>(fn, gpu, in, n, out, s...);
            }
            return out;
        };
        return body(pass);
#else
        return ref();
#endif
    });
}

template <typename Op, typename K, typename T, typename Ref, typename... S>
void run_stencil(VoxelImageT<T>& img, Passes ps, Ref&& ref, const S&... s) {
    long long unused = 0;
    run_stencil_impl<Op, K, false>(img, ps, &unused, ref, s...);
}
//! Ops that also return how many voxels changed (voxlib: `long long`): the kernel has a second
//! output from vxhl_gen::ChangeCounts. `ref` must set `count` (it runs only on the voxlib path).
template <typename Op, typename K, typename T, typename Ref, typename... S>
long long run_stencil_count(VoxelImageT<T>& img, Passes ps, Ref&& ref, const S&... s) {
    long long count = 0;
    run_stencil_impl<Op, K, true>(img, ps, &count, [&](auto& m) { count = ref(m); }, s...);
    return count;
}

}  // namespace vxhl::detail
