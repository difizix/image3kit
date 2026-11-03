#pragma once
//! voxlide: Halide-accelerated versions of voxlib ops, with voxlib as the fallback.
//! Each vxhl::<op> takes the same arguments as voxlib's ::<op> and gives the same result;
//! the backend comes from set_backend(), else $IMAGE3KIT_BACKEND, else Auto.
//! See docs/plans/parallelism_shared.md.

#include <cstddef>
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

// ---- ops: one declaration per ported op (same arguments as voxlib) ----
template <typename T> void mode26(VoxelImageT<T>& vImg, short minDif);
//! Returns the number of changed voxels (voxlib: Python mode6).
template <typename T> long long modeNSames(VoxelImageT<T>& vImage, short nSameNei, bool verbose = false);
template <typename T> VoxelImageT<T> median(const VoxelImageT<T>& vImg, int nIter = 1);
template <typename T> bool smoothBilateral(VoxelImageT<T>& vImg, int nItrs, int kernRad, double sigmavv, double sharpFact);
template <typename T> void bilateralX(VoxelImageT<T>& vImg, int kernRad, int Xstp, float sigmaSqr = 16, float sharpFact = 0.05,
                                      float sigmaGaussSqr = 2., float capmin = 0, float capmax = fmaxT(T));
template <typename T> void bilateralGauss(VoxelImageT<T>& vImg, int kernRad, float sigmaSqr = 16, float sharpFact = 0.05,
                                          float sigmaGaussSqr = 2., float capmin = 0, float capmax = fmaxT(T));
template <typename T> bool _bilateralGauss(VoxelImageT<T>& vImg, int nItrs, int kernRad, double sigmavv, double sharpFact,
                                           double sigmadd);
template <typename T> bool _bilateralX(VoxelImageT<T>& vImg, int nItrs, int kernRad, int Xstp, double sigmavv,
                                       double sharpFact, double sigmadd);

}  // namespace vxhl
