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
  X(replaceRange, 'A', 0, false, false) \
  X(zeroGrad, 'B', 1, false, false) /* radius nLayers: passed per call */ \
  X(magGradient, 'B', 1, false, false) \
  X(scaleGradient, 'A', 0, false, false) \
  X(segmentVote, 'C', 2, false, false) /* radius krnl: passed per call; reads 2 Aux images */ \
  X(median032, 'B', 1, false, false) \
  X(median06Grow, 'B', 1, false, false) \
  X(median06, 'B', 1, false, false) /* returns the change count */ \
  X(growLabel, 'B', 1, false, false) \
  X(shrinkPore, 'B', 1, false, false) \
  X(growPore, 'B', 1, false, false) \
  X(medianAxis, 'B', 1, false, false) /* median_x/y/z: axis is a kernel argument */ \
  X(pairOp, 'A', 0, false, false) /* and_/or_/xor_/not_/+=/-= with an Aux image */ \
  X(operateUnary, 'A', 0, false, false) \
  X(operateImg, 'A', 0, false, false) \
  X(operateScalar, 'A', 0, false, false) \
  X(threshold101, 'A', 0, false, false) \
  X(circleOut, 'A', 0, false, false) \
  X(mapFrom, 'A', 0, false, false) /* same-grid source image only */ \
  X(paintShape, 'A', 0, false, false) /* paint / paint_add: sphere, cylinder, cube */ \
  X(paintTriangular, 'A', 0, false, false) /* triangular: CPU only */ \
  X(setWhereEqual, 'A', 0, false, false) /* delense032's merge */ \
  X(moments, 'D', 0,   false,   true)  /* reduction: count, Σ, Σ², min, max per z */ \
  X(threshold01, 'A', 0, false, true) /* threshold01_otsu's apply: u8 output (run_remap) */ \
  X(swapXZ, 'F', 0, false, false) /* redirect('z'): needs every input layer per output slab */ \
  X(swapXY, 'F', 0, false, false) /* redirect('y') */ \
  X(flipX, 'F', 0, false, false) /* redirect('-') */ \
  X(resampleUp, 'F', 0, false, false) /* resampleMean, factor < 1 */ \
  X(resampleMeanDn, 'F', 0, false, false) /* resampleMean, factor > 1 */ \
  X(resampleMaxDn, 'F', 0, false, false) /* resampleMax, factor > 1 */ \
  X(resampleModeDn, 'F', 0, false, false) /* resampleMode, factor > 1 */ \
  X(cclInit, 'F', 0, false, false) /* keepLargest: i32 labels from the range */ \
  X(cclWeight, 'F', 0, false, false) /* keepLargest: i32 weights */ \
  X(cclLabel, 'E', 0, false, false) /* keepLargest: one round of line scans; halo gnz passed per call */ \
  X(cclSum, 'E', 0, false, false) \
  X(cclRoots, 'A', 0, false, false) \
  X(cclBest, 'D', 0, false, true) \
  X(cclApply, 'A', 0, false, false) \
  X(resliceUp, 'F', 0, false, false) /* resliceZ, factor < 1 */ \
  X(resliceDn, 'F', 0, false, false) /* resliceZ, factor > 1 (integer voxels) */ \
  X(growRange, 'B', 1, false, false) /* ::grow(vImg, midv, minv, maxv) */ \
  X(growOutside, 'F', 0, false, false) /* growOutSideValue's line scans: halo gnz (whole z extent) */ \
  X(padEdges, 'F', 0, false, false) /* meanWide's border planes: halo gnz */ \
  X(meanFilter, 'B', 1, false, false) /* ::mean: 3x3x3 mean */ \
  X(meanWideStep, 'B', 1, false, false) /* one meanWide iteration: radius nW passed per call; reads an Aux image */ \
  X(setWhereAbove, 'A', 0, false, false) /* meanWide: v = val where grad > thresh */ \
  X(remapBox, 'F', 0, false, false) /* crop / growBox / shrinkBox: another size (run_remap) */ \
  X(histogram, 'D', 0, false, true) /* reduction: per-z bin counts, bit-exact with voxlib histogram */ \
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
