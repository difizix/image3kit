#pragma once
//! Bindings call vxop::<op>(…) for ops that have a voxlide (Halide) port, and ::<op>(…)
//! otherwise. vxop:: is vxhl:: (backend from image3kit.set_backend / $IMAGE3KIT_BACKEND)
//! in the _core module, and plain voxlib where voxlide isn't linked (pocketpy, downstream).
//! Same arguments and results either way; this is the only voxlide #ifdef in the bindings.

// One entry per ported op (keep in sync with voxlide/ops_table.h).
#define VXOP_LIST(X) X(mode26) X(modeNSames) X(median) X(bilateralX) X(_bilateralX)
#define VXOP_LIST_VOXLIB(X) X(smoothBilateral)  // voxlib functions in namespace VoxLib

#ifdef IMAGE3KIT_HAS_VOXLIDE
#include "voxlide.h"
#define VXOP_USE_(op) using vxhl::op;
#define VXOP_USE_VOXLIB_(op) using vxhl::op;
#else
#include "voxelImageML.h"
#include "voxelImageProcess.h"
#define VXOP_USE_(op) using ::op;
#define VXOP_USE_VOXLIB_(op) using VoxLib::op;
#endif

namespace vxop {
VXOP_LIST(VXOP_USE_)
VXOP_LIST_VOXLIB(VXOP_USE_VOXLIB_)
}
#undef VXOP_USE_
#undef VXOP_USE_VOXLIB_
