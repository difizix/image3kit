#pragma once
//! voxlide: Halide-accelerated versions of voxlib ops, with voxlib as the fallback.
//! Each vxhl::<op> takes the same arguments as voxlib's ::<op> and gives the same result;
//! the backend comes from set_backend(), else $IMAGE3KIT_BACKEND, else Auto.
//! See docs/plans/parallelism_shared.md.

#include <cstddef>
#include <memory>
#include <type_traits>
#include <string>
#include <vector>
#include "voxelImage.h"

namespace vxhl {

enum class Backend {
    Auto,       //!< GPU if usable, else Halide CPU, else voxlib; a failing GPU kernel falls back (with a warning)
    Voxlib,     //!< reference OpenMP C++ implementation (voxlib)
    HalideCpu,  //!< Halide AOT vectorised + parallel CPU schedule
    Gpu         //!< Halide AOT GPU schedule (Vulkan; Metal on macOS)
};

//! Parse "auto", "voxlib"/"cpu"/"omp", "halide"/"halide_cpu", "gpu"/"vulkan"/"metal"; throws on others.
Backend parse_backend(const std::string& name);
//! Canonical lower-case name: "auto", "voxlib", "halide", "gpu".
const char* backend_name(Backend b);

//! Process-wide backend for all vxhl:: ops (not thread-local).
void    set_backend(Backend b);
Backend get_backend();

//! Backends usable on this machine and build: voxlib always, then halide, gpu.
std::vector<Backend> available_backends();

//! Max bytes per piece when an image is processed in z-slabs; 0 restores the default
//! (1 GiB on CPU, 512 MiB on GPU). Images larger than this are processed slab by slab.
void set_max_slab_bytes(std::size_t bytes);

//! An image that stays on the GPU between vxhl:: ops (docs/plans/parallelism_shared.md §2.7):
//!     vxhl::DeviceImage<uint8_t> d(img);           // no copy to the GPU yet
//!     vxhl::mode26(d, 2); vxhl::median(d, 3);      // on the GPU: one upload, no downloads
//!     double m = vxhl::meanDbl(d);                 // reductions download only their results
//!     img = d.host();                              // one download
//! Every ported op has a DeviceImage overload. On the GPU backend, with an image that fits in one
//! piece (max_slab_bytes), ops pass the device buffer on without host transfers; otherwise
//! (CPU backends, slabs, voxlib fallback, growBox) they use the host copy, synced as needed.
//! The host copy is only up to date through host()/host_mut(): never read it any other way.
template <typename T>
class DeviceImage {
public:
    explicit DeviceImage(VoxelImageT<T> img);
    DeviceImage(DeviceImage&&) noexcept;
    DeviceImage& operator=(DeviceImage&&) noexcept;
    ~DeviceImage();

    const VoxelImageT<T>& host() const;  //!< the current image; downloads if the device copy is newer
    VoxelImageT<T>& host_mut();          //!< same, to modify on the host: re-uploaded by the next GPU op
    int3 size3() const { return img_.size3(); }
    void growBox(int n) { host_mut().growBox(n); }  // on the host (device copy dropped)
    void shrinkBox(int n) { host_mut().shrinkBox(n); }

    struct Transfers {
        long long up = 0, down = 0;  //!< whole-image host→device / device→host copies so far
    };
    Transfers transfers() const;

    struct State;  //!< voxlide runtime state (device mirror); runtime.h
    State& state() const { return *st_; }
    VoxelImageT<T>& unsynced() const { return img_; }  //!< voxlide internal: host storage, not synced

private:
    mutable VoxelImageT<T> img_;
    std::unique_ptr<State> st_;
};

template <typename I> struct voxel_of;
template <typename T> struct voxel_of<VoxelImageT<T>> { using type = T; };
template <typename T> struct voxel_of<DeviceImage<T>> { using type = T; };
//! The voxel type of an image argument (VoxelImageT<T> or DeviceImage<T>).
template <typename I> using voxel_t = typename voxel_of<std::remove_const_t<I>>::type;
template <typename I> concept Image = requires { typename voxel_of<std::remove_const_t<I>>::type; };

// ---- ops: one declaration per ported op: voxlib's arguments, with the image as I& (Image I: a
// VoxelImageT<T> or a DeviceImage<T>) and voxlib's T as voxel_t<I>.
template <Image I> void mode26(I& vImg, short minDif);
//! Returns the number of changed voxels (voxlib: Python mode6).
template <Image I> long long modeNSames(I& vImage, short nSameNei, bool verbose = false);
//! segment2's shrink: voxels of seged in [min, maks] next to a different value take vImg's value.
template <Image I> void shrink(I& seged, voxel_t<I> min, voxel_t<I> maks, const VoxelImageT<voxel_t<I>>& vImg);
// statistics (reductions; voxlib/voxelImageI.h)
template <Image I> double varianceDbl(const I& img, int bgn, int end, bool verbose = false);
template <Image I> double stdevDbl(const I& img, int bgn, int end);
template <Image I> double meanDbl(const I& img);
template <Image I> double minDbl(const I& img);
template <Image I> double maxDbl(const I& img);
template <Image I> double porosityDbl(const I& img, voxel_t<I> threshold = 0);
template <typename T> VoxelImageT<T> median(const VoxelImageT<T>& vImg, int nIter = 1);
template <typename T> void median(DeviceImage<T>& vImg, int nIter = 1);  //!< in place
template <Image I> bool smoothBilateral(I& vImg, int nItrs, int kernRad, double sigmavv, double sharpFact);
template <Image I> void bilateralX(I& vImg, int kernRad, int Xstp, float sigmaSqr = 16, float sharpFact = 0.05,
                                   float sigmaGaussSqr = 2., float capmin = 0, float capmax = fmaxT(voxel_t<I>));
template <Image I> void bilateralGauss(I& vImg, int kernRad, float sigmaSqr = 16, float sharpFact = 0.05,
                                       float sigmaGaussSqr = 2., float capmin = 0, float capmax = fmaxT(voxel_t<I>));
template <Image I> bool _bilateralGauss(I& vImg, int nItrs, int kernRad, double sigmavv, double sharpFact, double sigmadd);
template <Image I> bool _bilateralX(I& vImg, int nItrs, int kernRad, int Xstp, double sigmavv, double sharpFact,
                                    double sigmadd);

}  // namespace vxhl
