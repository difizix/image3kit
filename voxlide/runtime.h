#pragma once
//! voxlide internals shared by every op: backend resolution, Halide buffer views,
//! the slab/iteration driver and the Auto fallback. Ops call detail::run_stencil and nothing else.
//! This is the only header that switches on IMAGE3KIT_HAS_HALIDE.

#include <atomic>
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
template <typename S> constexpr int snapshot_every(const S&) { return 0; }
constexpr int snapshot_every(const Snapshot& s) { return s.every; }

#ifdef IMAGE3KIT_HAS_HALIDE
template <typename T> using HBuf = Halide::Runtime::Buffer<T, 3>;

//! z-layers [kBgn,kEnd) of `f` as a Halide buffer in global coordinates (zero-copy).
template <typename T>
HBuf<T> zview(VoxelField<T>& f, int kBgn, int kEnd) {
    const int3 n = f.size3();
    HBuf<T> b(f.data() + (long long)kBgn * n.x * n.y, n.x, n.y, kEnd - kBgn);
    b.set_min(0, 0, kBgn);
    return b;
}

template <typename T, typename S>
auto kernel_arg(HBuf<T>* snap, const S& s) {
    if constexpr (std::is_same_v<S, Snapshot>) return snap->raw_buffer();
    else return s;
}

//! Call kernel `fn` (ABI: generators/vxhl_gen.h) reading `in` (+ `snap`), writing `out`.
template <typename Fn, typename T, typename... S>
void call_kernel(Fn fn, HBuf<T>& in, HBuf<T>* snap, HBuf<T>& out, int3 g, const S&... s) {
    const int rc = fn(in.raw_buffer(), g.x, g.y, g.z, kernel_arg(snap, s)..., out.raw_buffer());
    if (rc != 0) throw std::runtime_error(take_halide_error() + " (halide error " + std::to_string(rc) + ")");
}

//! One box of one pass: read box+halo of `in` (and `snap`), write the box interior of `out`.
template <typename Fn, typename T, typename... S>
void invoke_box(Fn fn, bool gpu, VoxelField<T>& in, VoxelField<T>* snap, VoxelField<T>& out, const Box& b,
                const S&... s) {
    auto ib = zview(in, b.kBgnRead(), b.kEndRead());
    auto ob = zview(out, b.kBgn(), b.kEnd());
    HBuf<T> sb = snap ? zview(*snap, b.kBgnRead(), b.kEndRead()) : HBuf<T>();
    if (gpu) { ib.set_host_dirty(); if (snap) sb.set_host_dirty(); }
    call_kernel(fn, ib, snap ? &sb : nullptr, ob, b.nGlobal, s...);
    if (gpu && ob.copy_to_host() != 0) throw std::runtime_error(take_halide_error());
}

//! Run kernel `fn` nIter times over the whole image. Images rotate between `img` (read
//! only, so a failure leaves it intact for the Auto fallback) and up to 3 work buffers,
//! so Snapshot arguments never copy.
//!  - one piece: on the GPU all buffers stay on the device: one upload, one download;
//!  - image larger than max_slab_bytes(): every pass is a pass over z-slabs with a halo
//!    of the op's radius, through host memory.
template <typename Op, typename Fn, typename T, typename... S>
void run_kernel(Fn fn, Backend b, VoxelImageT<T>& img, int nIter, const S&... s) {
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
            call_kernel(fn, view(cur), every ? &view(snap) : nullptr, view(out), n, s...);
            cur = out;
            if (gpu && snap != 0) v[0].device_free();  // the input is not read again
        }
        if (gpu && view(cur).copy_to_host() != 0) throw std::runtime_error(take_halide_error());
    } else {
        const auto boxes = splitZ(n, nSlabs, Op::traits.radius);
        for (int p = 0; p < nIter; ++p) {
            if (every && p % every == 0) snap = cur;
            const int out = pick(cur, snap);
            for (const Box& box : boxes) invoke_box(fn, gpu, *f[cur], every ? f[snap] : nullptr, *f[out], box, s...);
            cur = out;
        }
    }
    img.data_ = std::move(f[cur]->data_);
}
#endif

//! The one implementation every ported op goes through.
//! Op: vxhl::op::<name> (traits); K: kernels::<name><T> (generated, cpu/gpu may be nullptr);
//! nIter: kernel applications (iterated ops; 1 otherwise); ref: the voxlib reference,
//! applied in place, doing all nIter iterations itself; s: the kernel's op scalars and
//! Snapshot{} image arguments, in the generator's Input order.
template <typename Op, typename K, typename T, typename Ref, typename... S>
void run_stencil(VoxelImageT<T>& img, int nIter, Ref&& ref, const S&... s) {
    if (nIter <= 0) return;
    constexpr bool hasCpu = !std::is_null_pointer_v<std::remove_cv_t<decltype(K::cpu)>>;
    constexpr bool hasGpu = !std::is_null_pointer_v<std::remove_cv_t<decltype(K::gpu)>>;
    static std::atomic<bool> gpuBroken{false};  // per op × type
    Choice c = resolve(Op::traits, hasCpu, hasGpu, gpuBroken);

#ifdef IMAGE3KIT_HAS_HALIDE
    while (c.backend != Backend::Voxlib) {
        try {
            if constexpr (hasGpu) if (c.backend == Backend::Gpu) { run_kernel<Op>(K::gpu, c.backend, img, nIter, s...); return; }
            if constexpr (hasCpu) if (c.backend == Backend::HalideCpu) { run_kernel<Op>(K::cpu, c.backend, img, nIter, s...); return; }
            throw std::logic_error("no kernel for resolved backend");
        } catch (const std::exception& e) {
            if (c.strict) throw std::runtime_error(std::string("vxhl::") + Op::traits.name + " on " +
                                                   backend_name(c.backend) + ": " + e.what());
            std::cerr << "voxlide: " << Op::traits.name << " " << backend_name(c.backend)
                      << " failed, falling back: " << e.what() << std::endl;
            if (c.backend == Backend::Gpu) {
                gpuBroken = true;
                c.backend = hasCpu ? Backend::HalideCpu : Backend::Voxlib;
            } else {
                c.backend = Backend::Voxlib;
            }
        }
    }
#endif
    ref(img);
}

}  // namespace vxhl::detail
