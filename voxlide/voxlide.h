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
    Auto,       //!< GPU for large images if usable, else Halide CPU, else voxlib; never throws on GPU problems
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
template <typename T> VoxelImageT<T> median(const VoxelImageT<T>& vImg, int nIter = 1);

}  // namespace vxhl
