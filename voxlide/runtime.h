#pragma once
//! voxlide internals shared by every op: backend resolution, Halide buffer views,
//! the slab/iteration driver and the Auto fallback. Ops call detail::run_stencil and nothing else.
//! This is the only header that switches on IMAGE3KIT_HAS_HALIDE.

#include <array>
#include <atomic>
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
//! gradient), a VoxelImageT or a DeviceImage. Each kernel call gets the z-layers its `input`
//! covers (slab + halo). Within one run_stencil call the buffer is reused, so on the GPU it is
//! uploaded once for all passes; a DeviceImage on the GPU in one piece is read in place.
//! It must outlive the run_stencil call it is passed to.
template <typename T>
struct Aux {
    Aux(const VoxelImageT<T>& i) : img(&i) {}
    Aux(const DeviceImage<T>& d) : dev(&d) {}
    const VoxelImageT<T>& host() const { return dev ? dev->host() : *img; }  //!< synced
    int3 size3() const { return dev ? dev->size3() : img->size3(); }
    const VoxelImageT<T>* img = nullptr;
    const DeviceImage<T>* dev = nullptr;
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

template <typename T> HBuf<T>& resident(const DeviceImage<T>& d);

//! The view of `a` covering the z-layers of `in` (cached: the same layers reuse the buffer).
template <typename T>
halide_buffer_t* aux_view(const Aux<T>& a, const HBuf<T>& in, int3 g, bool gpu) {
    if (a.size3() != g)
        throw std::invalid_argument("vxhl: auxiliary image size differs from the image");
    const int kBgn = in.dim(2).min(), kEnd = kBgn + in.dim(2).extent();
    if (gpu && a.dev && kBgn == 0 && kEnd == g.z) return resident(*a.dev).raw_buffer();  // device copy
    if (!a.buf.data() || a.buf.dim(2).min() != kBgn || a.buf.dim(2).extent() != kEnd - kBgn) {
        a.buf = zview(const_cast<VoxelImageT<T>&>(a.host()), kBgn, kEnd);  // read only
        a.buf.set_host_dirty();  // uploaded on the first GPU use; ignored on the CPU
    }
    return a.buf.raw_buffer();
}

template <typename T, typename S>
auto kernel_arg(const HBuf<T>& in, int3 g, bool gpu, HBuf<T>* snap, const S& s) {
    if constexpr (std::is_same_v<S, Snapshot>) return snap->raw_buffer();
    else if constexpr (std::is_same_v<S, Aux<T>>) return aux_view(s, in, g, gpu);
    else return s;
}

//! Call kernel `fn` (ABI: generators/vxhl_gen.h) reading `in` (+ `snap`), writing `out`.
//! Count: the kernel also fills `cnt` (vxhl_gen::ChangeCounts, one int32 per z of `out`).
template <bool Count, typename Fn, typename T, typename... S>
void call_kernel(Fn fn, bool gpu, HBuf<T>& in, HBuf<T>* snap, HBuf<T>& out, CBuf& cnt, int3 g, const S&... s) {
    int rc;
    if constexpr (Count) rc = fn(in.raw_buffer(), g.x, g.y, g.z, kernel_arg(in, g, gpu, snap, s)..., out.raw_buffer(), cnt.raw_buffer());
    else                 rc = fn(in.raw_buffer(), g.x, g.y, g.z, kernel_arg(in, g, gpu, snap, s)..., out.raw_buffer());
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
    call_kernel<Count>(fn, gpu, ib, snap ? &sb : nullptr, ob, cb, b.nGlobal, s...);
    if (gpu && ob.copy_to_host() != 0) throw std::runtime_error(take_halide_error());
    if constexpr (Count) add_counts(cb, gpu, *total);
}

//! Number of z-slabs an image of size n is processed in on backend b.
template <typename T> int n_slabs(int3 n, Backend b) {
    const long long bytes = (long long)n.x * n.y * n.z * (long long)sizeof(T);
    const long long lim = (long long)max_slab_bytes(b);
    return std::max(1, int((bytes + lim - 1) / lim));
}

//! Run kernel `fn` nIter times over the whole image. Images rotate between `img` (read
//! only, so a failure leaves it intact for the Auto fallback) and up to 3 work buffers,
//! so Snapshot arguments never copy.
//!  - one piece: on the GPU all buffers stay on the device: one upload, one download;
//!  - image larger than max_slab_bytes(): every pass is a pass over z-slabs with a halo
//!    of the op's radius, through host memory.
//!  - `resident` (DeviceImage on the GPU, one piece): the input's device buffer (a view of
//!    `img`); the result is left on the device in it, not downloaded.
template <typename Op, bool Count, typename Fn, typename T, typename... S>
void run_kernel(Fn fn, Backend b, VoxelImageT<T>& img, int nIter, int radius, long long* total, HBuf<T>* resident,
                const S&... s) {
    *total = 0;
    const int3 n = img.size3();
    const bool gpu = b == Backend::Gpu;
    int every = 0;
    ((every = std::max(every, snapshot_every(s))), ...);
    const int nSlabs = n_slabs<T>(n, b);

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
            if (i == 0 && resident) return *resident;
            if (!v[i].data()) v[i] = zview(*f[i], 0, n.z);
            return v[i];
        };
        if (gpu && !resident) view(0).set_host_dirty();
        for (int p = 0; p < nIter; ++p) {
            if (every && p % every == 0) snap = cur;
            const int out = pick(cur, snap);
            CBuf cb;
            if constexpr (Count) cb = CBuf(n.z);
            call_kernel<Count>(fn, gpu, view(cur), every ? &view(snap) : nullptr, view(out), cb, n, s...);
            if constexpr (Count) add_counts(cb, gpu, *total);
            cur = out;
            if (gpu && snap != 0 && !resident) v[0].device_free();  // the input is not read again
        }
        if (resident) *resident = std::move(v[cur]);  // its host memory moves into img below
        else if (gpu && view(cur).copy_to_host() != 0) throw std::runtime_error(take_halide_error());
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
using DBuf2 = Halide::Runtime::Buffer<double, 2>;

//! One reduction kernel call on the z-layers of `in`; ABI (generators/vxhl_gen.h):
//!     int <op>_<dev>_<type>(input, gnx, gny, gnz, <scalars...>, stats)
//! stats(i, z), i in [0, N): the kernel's N partial results per z-layer (one download).
template <std::size_t N, typename Fn, typename T, typename... S>
void reduce_box(Fn fn, bool gpu, HBuf<T>& in, int3 g, PerZ<N>& out, const S&... s) {
    const int k0 = in.dim(2).min(), nk = in.dim(2).extent();
    DBuf2 ob(static_cast<int>(N), nk);
    ob.set_min(0, k0);
    const int rc = fn(in.raw_buffer(), g.x, g.y, g.z, s..., ob.raw_buffer());
    if (rc != 0) throw std::runtime_error(take_halide_error() + " (halide error " + std::to_string(rc) + ")");
    if (gpu && ob.copy_to_host() != 0) throw std::runtime_error(take_halide_error());
    for (std::size_t i = 0; i < N; ++i)
        for (int k = k0; k < k0 + nk; ++k) out[i][k] = ob(int(i), k);
}
#endif

}  // namespace vxhl::detail

namespace vxhl {
//! DeviceImage state: a Halide view of the host storage with an optional device mirror.
template <typename T>
struct DeviceImage<T>::State {
#ifdef IMAGE3KIT_HAS_HALIDE
    detail::HBuf<T> buf;  //!< view of img_ (host pointer) + device allocation
#endif
    bool deviceNewer = false;  //!< the device copy holds results the host lacks
    bool hostNewer = true;     //!< the host copy holds data the device lacks
    Transfers n;
};
template <typename T> DeviceImage<T>::DeviceImage(VoxelImageT<T> img) : img_(std::move(img)), st_(new State) {}
template <typename T> DeviceImage<T>::DeviceImage(DeviceImage&&) noexcept = default;
template <typename T> DeviceImage<T>& DeviceImage<T>::operator=(DeviceImage&&) noexcept = default;
template <typename T> DeviceImage<T>::~DeviceImage() = default;
template <typename T> const VoxelImageT<T>& DeviceImage<T>::host() const {
#ifdef IMAGE3KIT_HAS_HALIDE
    if (st_->deviceNewer) {
        if (st_->buf.copy_to_host() != 0) throw std::runtime_error("vxhl::DeviceImage: " + detail::take_halide_error());
        ++st_->n.down;
        st_->deviceNewer = false;
    }
#endif
    return img_;
}
template <typename T> VoxelImageT<T>& DeviceImage<T>::host_mut() {
    host();
    st_->hostNewer = true;
    return img_;
}
template <typename T> typename DeviceImage<T>::Transfers DeviceImage<T>::transfers() const { return st_->n; }
}  // namespace vxhl

namespace vxhl::detail {

#ifdef IMAGE3KIT_HAS_HALIDE
//! The device buffer of `d` holding the current image (uploads it if the host copy is newer).
template <typename T>
HBuf<T>& resident(const DeviceImage<T>& d) {
    auto& st = d.state();
    VoxelImageT<T>& img = d.unsynced();
    const int3 n = img.size3();
    if (st.buf.data() != img.data() || st.buf.dim(0).extent() != n.x || st.buf.dim(1).extent() != n.y ||
        st.buf.dim(2).extent() != n.z) {  // host storage replaced (host_mut + growBox, CPU backend)
        if (st.deviceNewer) throw std::logic_error("vxhl::DeviceImage: device data lost");
        st.buf = zview(img, 0, n.z);
        st.hostNewer = true;
    }
    if (st.hostNewer) {
        st.buf.set_host_dirty();
        ++st.n.up;
        st.hostNewer = false;
    }
    return st.buf;
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
        run_kernel<Op, Count>(fn, b, img, ps.n, ps.radius >= 0 ? ps.radius : Op::traits.radius, count, static_cast<HBuf<T>*>(nullptr), s...);
#else
        (void)count, ((void)s, ...);
#endif
    });
}
//! DeviceImage: on the GPU with one piece the image stays on the device; otherwise as above on host().
template <typename Op, typename K, bool Count, typename T, typename Ref, typename... S>
void run_stencil_impl(DeviceImage<T>& d, Passes ps, long long* count, Ref&& ref, const S&... s) {
    if (ps.n <= 0) return;
    dispatch<Op, K>([&] { ref(d.host_mut()); }, [&]([[maybe_unused]] auto fn, [[maybe_unused]] Backend b) {
#ifdef IMAGE3KIT_HAS_HALIDE
        const int radius = ps.radius >= 0 ? ps.radius : Op::traits.radius;
        if (b == Backend::Gpu && n_slabs<T>(d.size3(), b) == 1) {
            run_kernel<Op, Count>(fn, b, d.unsynced(), ps.n, radius, count, &resident(d), s...);
            d.state().deviceNewer = true;
        } else {
            run_kernel<Op, Count>(fn, b, d.host_mut(), ps.n, radius, count, static_cast<HBuf<T>*>(nullptr), s...);
        }
#else
        (void)count, ((void)s, ...);
#endif
    });
}

//! Reductions (class D): `body(pass)` computes the op's result, calling `pass(scalars...)`
//! (one kernel run over the whole image, slab by slab) as often as it needs; it returns the
//! kernel's N outputs per z (PerZ<N>), which `body` combines. On the GPU a one-piece image is
//! uploaded once for all passes (a resident DeviceImage not at all). ref(host image): the voxlib
//! function. The image is never modified.
template <typename Op, typename K, std::size_t N, typename I, typename Ref, typename Body>
auto run_reduce(const I& img, Ref&& ref, [[maybe_unused]] Body&& body) {
    using T = voxel_t<I>;
    constexpr bool device = std::is_same_v<std::remove_const_t<I>, DeviceImage<T>>;
    auto host = [&]() -> const VoxelImageT<T>& {
        if constexpr (device) return img.host(); else return img;
    };
    using R = decltype(ref(host()));
    const int3 n = img.size3();
    if ((long long)n.x * n.y * n.z == 0) return ref(host());
    return dispatch<Op, K>([&] { return ref(host()); }, [&]([[maybe_unused]] auto fn, [[maybe_unused]] Backend b) -> R {
#ifdef IMAGE3KIT_HAS_HALIDE
        const bool gpu = b == Backend::Gpu;
        HBuf<T>* res = nullptr;  // DeviceImage on the GPU in one piece: read the device copy
        if constexpr (device) if (gpu && n_slabs<T>(n, b) == 1) res = &resident(img);
        const auto boxes = splitZ(n, n_slabs<T>(n, b), 0);
        HBuf<T> whole;  // kept (and on the GPU, resident) across passes when there is one box
        auto pass = [&](const auto&... s) {
            PerZ<N> out;
            for (auto& v : out) v.assign(n.z, 0.0);
            for (const Box& box : boxes) {
                HBuf<T> local;
                HBuf<T>& in = res ? *res : boxes.size() == 1 ? whole : local;
                if (!in.data()) {
                    in = zview(const_cast<VoxelImageT<T>&>(host()), box.kBgn(), box.kEnd());  // read only
                    if (gpu) in.set_host_dirty();
                }
                reduce_box<N>(fn, gpu, in, n, out, s...);
            }
            return out;
        };
        return body(pass);
#else
        return ref(host());
#endif
    });
}

#ifdef IMAGE3KIT_HAS_HALIDE
template <typename Fn, typename T, typename TO, typename... S>
void call_remap(Fn fn, HBuf<T>& in, HBuf<TO>& out, int3 g, const S&... s) {
    const int rc = fn(in.raw_buffer(), g.x, g.y, g.z, s..., out.raw_buffer());
    if (rc != 0) throw std::runtime_error(take_halide_error() + " (halide error " + std::to_string(rc) + ")");
}
#endif

//! Ops whose output has another size and/or voxel type (F6; class F: crop, growBox, u8 masks…).
//! Kernel ABI: fn(input, gnx, gny, gnz, scalars..., output), the output in its own global
//! coordinates. The output is processed in z-slabs; zIn(kBgn, kEnd) -> std::pair{first, end} gives
//! the (non-empty) input layers that output layers [kBgn, kEnd) read. The result has size `no` and
//! header dx, X0. ref(host input) -> VoxelImageT<TO>: the voxlib function. Returns a
//! VoxelImageT<TO>, or a DeviceImage<TO> for a DeviceImage input: on the GPU in one piece the input
//! is read in place and the result stays on the device.
template <typename Op, typename K, typename TO, typename I, typename Ref, typename ZIn, typename... S>
auto run_remap(const I& img, int3 no, dbl3 dx, dbl3 X0, Ref&& ref, [[maybe_unused]] ZIn&& zIn, const S&... s) {
    using T = voxel_t<I>;
    constexpr bool device = std::is_same_v<std::remove_const_t<I>, DeviceImage<T>>;
    using R = std::conditional_t<device, DeviceImage<TO>, VoxelImageT<TO>>;
    auto host = [&]() -> const VoxelImageT<T>& {
        if constexpr (device) return img.host(); else return img;
    };
    auto wrap = [&](VoxelImageT<TO>&& v) -> R {  // a DeviceImage result keeps the input's transfer counts
        if constexpr (device) { R r(std::move(v)); r.state().n = img.state().n; return r; }
        else return std::move(v);
    };
    const int3 n = img.size3();
    if ((long long)n.x * n.y * n.z == 0 || (long long)no.x * no.y * no.z == 0) return wrap(ref(host()));
    return dispatch<Op, K>([&] { return wrap(ref(host())); }, [&]([[maybe_unused]] auto fn, [[maybe_unused]] Backend b) -> R {
#ifdef IMAGE3KIT_HAS_HALIDE
        const bool gpu = b == Backend::Gpu;
        const int nSlabs = std::max(n_slabs<T>(n, b), n_slabs<TO>(no, b));
        VoxelImageT<TO> out(no, TO(0), dx, X0);
        if constexpr (device) {
            if (gpu && nSlabs == 1) {
                R r(std::move(out));
                HBuf<TO> ob = zview(r.unsynced(), 0, no.z);
                call_remap(fn, resident(img), ob, n, s...);
                auto& st = r.state();
                st.buf = std::move(ob);
                st.hostNewer = false, st.deviceNewer = true;
                st.n = img.state().n;
                return r;
            }
        }
        for (const Box& box : splitZ(no, nSlabs, 0)) {
            const auto [k0, k1] = zIn(box.kBgn(), box.kEnd());
            HBuf<T> ib = zview(const_cast<VoxelImageT<T>&>(host()), k0, k1);  // read only
            HBuf<TO> ob = zview(out, box.kBgn(), box.kEnd());
            if (gpu) ib.set_host_dirty();
            call_remap(fn, ib, ob, n, s...);
            if (gpu && ob.copy_to_host() != 0) throw std::runtime_error(take_halide_error());
        }
        return wrap(std::move(out));
#else
        return wrap(ref(host()));
#endif
    });
}

template <typename Op, typename K, typename I, typename Ref, typename... S>
void run_stencil(I& img, Passes ps, Ref&& ref, const S&... s) {
    long long unused = 0;
    run_stencil_impl<Op, K, false>(img, ps, &unused, ref, s...);
}
//! Ops that also return how many voxels changed (voxlib: `long long`): the kernel has a second
//! output from vxhl_gen::ChangeCounts. `ref` must set `count` (it runs only on the voxlib path).
template <typename Op, typename K, typename I, typename Ref, typename... S>
long long run_stencil_count(I& img, Passes ps, Ref&& ref, const S&... s) {
    long long count = 0;
    run_stencil_impl<Op, K, true>(img, ps, &count, [&](auto& m) { count = ref(m); }, s...);
    return count;
}

}  // namespace vxhl::detail
