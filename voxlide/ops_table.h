#pragma once
//! One row per ported op. Read by resolve() (thresholds), the slab driver (radius → halo)
//! and later by voxlib_mpi's runStencil. Class letters follow
//! docs/plans/parallelism_distributed.md §3 (A pointwise, B stencil, C iterated stencil, D reduction).

namespace vxhl {

struct OpTraits {
    const char* name;
    char cls;                  //!< communication class A–F
    int radius;                //!< stencil radius in voxels (halo per iteration)
    bool iterated;             //!< runs nIters times (halo = radius·nIters)
    bool global_stats;         //!< needs global statistics passed in (class D apply-only port)
    long long min_voxels;      //!< Auto: below this, use voxlib
    long long gpu_min_voxels;  //!< Auto: below this, prefer Halide CPU over GPU
};

//! Auto never picks the GPU for now: Halide's Vulkan device->host copy reads uncached memory
//! (~0.25 GB/s on RADV), so one op on the GPU is slower than Halide CPU at every size
//! (parallelism_shared.md §9). Lower this once downloads are fast or ops are chained on device.
#define VXHL_GPU_NOT_IN_AUTO (1LL << 62)

// clang-format off
#define VXHL_OPS(X) \
/*  op      cls  radius iterated stats  min_voxels  gpu_min_voxels */ \
  X(mode26, 'B', 1,     false,   false, 16*16*16,   VXHL_GPU_NOT_IN_AUTO) \
  X(median, 'C', 1,     true,    false, 16*16*16,   VXHL_GPU_NOT_IN_AUTO)
// clang-format on

namespace op {
#define VXHL_DEF_OP_(nm, c, r, it, st, mn, gmn) \
    struct nm { static constexpr OpTraits traits{#nm, c, r, it, st, mn, gmn}; };
VXHL_OPS(VXHL_DEF_OP_)
#undef VXHL_DEF_OP_
}  // namespace op

//! Instantiate a per-type template for every voxel type image3kit binds.
#define VXHL_FOR_TYPES(X) X(unsigned char) X(unsigned short) X(int) X(float)

}  // namespace vxhl
