#pragma once
//! Bindings call vxop::<op>(…) for ops that have a voxlide (Halide) port, and ::<op>(…)
//! otherwise. vxop:: is vxhl:: (backend from image3kit.set_backend / $IMAGE3KIT_BACKEND)
//! in the _core module, and plain voxlib where voxlide isn't linked (pocketpy, downstream).
//! Same arguments and results either way; this is the only voxlide #ifdef in the bindings.

// One entry per ported op (keep in sync with voxlide/ops_table.h).
#define VXOP_LIST(X) X(mode26) X(modeNSames) X(and_) X(or_) X(xor_) X(not_) X(addEq) X(subEq) X(operateUnary) X(operateImg) X(operateScalar) X(threshold101) X(circleOut) X(mapToFrom) X(fillHoles) X(_delense032) X(growPore) X(shrinkPore) X(medianx) X(mediany) X(medianz) X(median032) X(FaceMedGrowToFrom) X(median06) X(growLabel) X(replaceRange) X(segment2) X(median) X(varianceDbl) X(stdevDbl) X(meanDbl) X(minDbl) X(maxDbl) X(porosityDbl) X(bilateralX) X(_bilateralX) X(bilateralGauss) X(_bilateralGauss) X(cropD) X(growBox) X(shrinkBox) X(redirect) X(resampleMean) X(resliceZ) X(otsu_minAvgThresholdAvgMax) X(threshold01_otsu)
#define VXOP_LIST_VOXLIB(X) X(smoothBilateral) X(paint) X(paintAdd) X(growOutSideValue)  // voxlib functions in namespace VoxLib

#ifdef IMAGE3KIT_HAS_VOXLIDE
#include "voxlide.h"
#define VXOP_USE_(op) using vxhl::op;
#define VXOP_USE_VOXLIB_(op) using vxhl::op;
#else
#include "voxelImageML.h"
#include "voxelImageProcess.h"
#include "shapeToVoxel.h"
#define VXOP_USE_(op) using ::op;
#define VXOP_USE_VOXLIB_(op) using VoxLib::op;
#endif

namespace vxop {
VXOP_LIST(VXOP_USE_)
VXOP_LIST_VOXLIB(VXOP_USE_VOXLIB_)
}
#undef VXOP_USE_
#undef VXOP_USE_VOXLIB_
