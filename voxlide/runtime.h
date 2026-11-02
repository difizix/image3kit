#pragma once
//! voxlide internals shared by every op: backend resolution, Halide buffer views,
//! the slab driver and the Auto fallback. Ops call detail::run_stencil and nothing else.
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
Choice resolve(const OpTraits& op, long long nvox, bool hasCpu, bool hasGpu, bool gpuBroken);
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

//! Run kernel `fn` on one box: read box+halo of `in`, write the box interior of `out`.
//! Kernel ABI (generators/vxhl_gen.h): fn(input, gnx, gny, gnz, op scalars..., output).
template <typename Fn, typename T, typename... S>
void invoke_box(Fn fn, bool gpu, VoxelField<T>& in, VoxelField<T>& out, const Box& b, S... s) {
    auto ib = zview(in, b.kBgnRead(), b.kEndRead());
    auto ob = zview(out, b.kBgn(), b.kEnd());
    if (gpu) ib.set_host_dirty();
    int rc = fn(ib.raw_buffer(), b.nGlobal.x, b.nGlobal.y, b.nGlobal.z, s..., ob.raw_buffer());
    if (rc == 0 && gpu) rc = ob.copy_to_host();
    if (gpu) { ib.device_free(); ob.device_free(); }
    if (rc != 0) throw std::runtime_error(take_halide_error() + " (halide error " + std::to_string(rc) + ")");
}

//! Run kernel `fn` over the whole image, slab by slab when it exceeds max_slab_bytes().
template <typename Op, typename Fn, typename T, typename... S>
void run_kernel(Fn fn, Backend b, VoxelImageT<T>& img, S... s) {
    const int3 n = img.size3();
    const long long bytes = (long long)n.x * n.y * n.z * (long long)sizeof(T);
    const long long lim = (long long)max_slab_bytes(b);
    const int halo = Op::traits.radius;
    const int nSlabs = int((bytes + lim - 1) / lim);
    VoxelField<T> out(n);
    for (const Box& box : splitZ(n, nSlabs, halo))
        invoke_box(fn, b == Backend::Gpu, img, out, box, s...);
    img.data_ = std::move(out.data_);
}
#endif

//! The one implementation every ported op goes through.
//! Op: vxhl::op::<name> (traits); K: kernels::<name><T> (generated, cpu/gpu may be nullptr);
//! ref: calls the voxlib reference in place; s: the kernel's op scalars.
template <typename Op, typename K, typename T, typename Ref, typename... S>
void run_stencil(VoxelImageT<T>& img, Ref&& ref, S... s) {
    constexpr bool hasCpu = !std::is_null_pointer_v<std::remove_cv_t<decltype(K::cpu)>>;
    constexpr bool hasGpu = !std::is_null_pointer_v<std::remove_cv_t<decltype(K::gpu)>>;
    static std::atomic<bool> gpuBroken{false};  // per op × type
    const int3 n = img.size3();
    Choice c = resolve(Op::traits, (long long)n.x * n.y * n.z, hasCpu, hasGpu, gpuBroken);

#ifdef IMAGE3KIT_HAS_HALIDE
    while (c.backend != Backend::Voxlib) {
        try {
            if constexpr (hasGpu) if (c.backend == Backend::Gpu) { run_kernel<Op>(K::gpu, c.backend, img, s...); return; }
            if constexpr (hasCpu) if (c.backend == Backend::HalideCpu) { run_kernel<Op>(K::cpu, c.backend, img, s...); return; }
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
