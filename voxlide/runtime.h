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

#ifdef IMAGE3KIT_HAS_HALIDE
//! z-layers [kBgn,kEnd) of `f` as a Halide buffer in global coordinates (zero-copy).
template <typename T>
Halide::Runtime::Buffer<T, 3> zview(VoxelField<T>& f, int kBgn, int kEnd) {
    const int3 n = f.size3();
    Halide::Runtime::Buffer<T, 3> b(f.data() + (long long)kBgn * n.x * n.y, n.x, n.y, kEnd - kBgn);
    b.set_min(0, 0, kBgn);
    return b;
}

//! Call kernel `fn` (ABI: generators/vxhl_gen.h) reading `in`, writing `out`; throws on failure.
template <typename Fn, typename T, typename... S>
void call_kernel(Fn fn, Halide::Runtime::Buffer<T, 3>& in, Halide::Runtime::Buffer<T, 3>& out, int3 g, S... s) {
    const int rc = fn(in.raw_buffer(), g.x, g.y, g.z, s..., out.raw_buffer());
    if (rc != 0) throw std::runtime_error(take_halide_error() + " (halide error " + std::to_string(rc) + ")");
}

//! Run kernel `fn` on one box: read box+halo of `in`, write the box interior of `out`.
template <typename Fn, typename T, typename... S>
void invoke_box(Fn fn, bool gpu, VoxelField<T>& in, VoxelField<T>& out, const Box& b, S... s) {
    auto ib = zview(in, b.kBgnRead(), b.kEndRead());
    auto ob = zview(out, b.kBgn(), b.kEnd());
    if (gpu) ib.set_host_dirty();
    call_kernel(fn, ib, ob, b.nGlobal, s...);
    if (gpu && ob.copy_to_host() != 0) throw std::runtime_error(take_halide_error());
}

//! Run kernel `fn` nIter times over the whole image. `img` is only replaced at the end, so
//! a failure leaves it untouched for the Auto fallback.
//!  - one piece: ping-pong two buffers; on the GPU they stay on the device, so there is one
//!    upload and one download in total, whatever nIter is;
//!  - image larger than max_slab_bytes(): every iteration is a pass over z-slabs with a
//!    halo of the op's radius, through host memory.
template <typename Op, typename Fn, typename T, typename... S>
void run_kernel(Fn fn, Backend b, VoxelImageT<T>& img, int nIter, S... s) {
    const int3 n = img.size3();
    const bool gpu = b == Backend::Gpu;
    const long long bytes = (long long)n.x * n.y * n.z * (long long)sizeof(T);
    const long long lim = (long long)max_slab_bytes(b);
    const int nSlabs = int((bytes + lim - 1) / lim);
    VoxelField<T> pp[2] = {VoxelField<T>(n), VoxelField<T>(nIter > 1 ? n : int3(0, 0, 0))};
    if (nSlabs <= 1) {
        auto src = zview(img, 0, n.z);
        Halide::Runtime::Buffer<T, 3> buf[2] = {zview(pp[0], 0, n.z), nIter > 1 ? zview(pp[1], 0, n.z) : Halide::Runtime::Buffer<T, 3>()};
        if (gpu) src.set_host_dirty();
        call_kernel(fn, src, buf[0], n, s...);
        if (gpu) src.device_free();
        for (int it = 1; it < nIter; ++it) call_kernel(fn, buf[(it - 1) & 1], buf[it & 1], n, s...);
        if (gpu && buf[(nIter - 1) & 1].copy_to_host() != 0) throw std::runtime_error(take_halide_error());
    } else {
        const auto boxes = splitZ(n, nSlabs, Op::traits.radius);
        VoxelField<T>* src = &img;
        for (int it = 0; it < nIter; ++it) {
            for (const Box& box : boxes) invoke_box(fn, gpu, *src, pp[it & 1], box, s...);
            src = &pp[it & 1];
        }
    }
    img.data_ = std::move(pp[(nIter - 1) & 1].data_);
}
#endif

//! The one implementation every ported op goes through.
//! Op: vxhl::op::<name> (traits); K: kernels::<name><T> (generated, cpu/gpu may be nullptr);
//! nIter: kernel applications (iterated ops; 1 otherwise); ref: the voxlib reference,
//! applied in place, doing all nIter iterations itself; s: the kernel's op scalars.
template <typename Op, typename K, typename T, typename Ref, typename... S>
void run_stencil(VoxelImageT<T>& img, int nIter, Ref&& ref, S... s) {
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
