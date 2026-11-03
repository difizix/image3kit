#pragma once
//! One row per ported op. Read by the slab driver (radius → halo)
//! and later by voxlib_mpi's runStencil. Class letters follow
//! docs/plans/parallelism_distributed.md §3 (A pointwise, B stencil, C iterated stencil, D reduction).

namespace vxhl {

struct OpTraits {
    const char* name;
    char cls;                  //!< communication class A–F
    int radius;                //!< stencil radius in voxels (halo per iteration)
    bool iterated;             //!< runs nIters times (halo = radius·nIters)
    bool global_stats;         //!< needs global statistics passed in (class D apply-only port)
};

// clang-format off
#define VXHL_OPS(X) \
/*  op      cls  radius iterated stats */ \
  X(mode26, 'B', 1,     false,   false) \
  X(mode6, 'B', 1,     false,   false) /* returns the change count */ \
  X(shrink, 'B', 1,    false,   false) /* reads an Aux image */ \
  X(median, 'C', 1,     true,    false) \
  X(smoothBilateral, 'C', 1, true, false) /* radius per pass; kernRad passes per iteration */ \
  X(bilateralX, 'C', 1, true, false) /* radius kernRad: passed per call (Passes) */ \
  X(bilateralGauss, 'C', 1, true, false) /* radius kernRad: passed per call (Passes) */
// clang-format on

namespace op {
#define VXHL_DEF_OP_(nm, c, r, it, st) \
    struct nm { static constexpr OpTraits traits{#nm, c, r, it, st}; };
VXHL_OPS(VXHL_DEF_OP_)
#undef VXHL_DEF_OP_
}  // namespace op

//! Instantiate a per-type template for every voxel type image3kit binds.
#define VXHL_FOR_TYPES(X) X(unsigned char) X(unsigned short) X(int) X(float)

}  // namespace vxhl
