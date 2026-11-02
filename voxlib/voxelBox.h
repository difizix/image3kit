#pragma once
//! Geometry of a region (a z-slab) of a larger image, used to run stencils piecewise:
//! out of GPU memory, past Halide's 2^31-byte buffer limit, and later per MPI rank
//! (see docs/plans/parallelism_shared.md §2.4, parallelism_distributed.md §4).
//! Pure C++: no Halide, no MPI.

#include <algorithm>
#include <vector>
#include "typses.h"

struct Box {
  int3 n;        //!< interior size: the voxels this piece writes
  int3 global0;  //!< global index of the interior's first voxel
  int3 nGlobal;  //!< size of the whole image
  int  halo = 0; //!< z-layers read on each side of the interior (clipped at the image ends)

  static Box whole(int3 nn) { return Box{nn, int3(0, 0, 0), nn, 0}; }

  int kBgn()     const { return global0.z; }
  int kEnd()     const { return global0.z + n.z; }
  int kBgnRead() const { return std::max(0, global0.z - halo); }
  int kEndRead() const { return std::min(nGlobal.z, global0.z + n.z + halo); }
};

//! Split an image of size `nGlobal` into `nSlabs` z-slabs of near-equal thickness, each
//! reading `halo` extra layers on both sides.
inline std::vector<Box> splitZ(int3 nGlobal, int nSlabs, int halo) {
  nSlabs = std::clamp(nSlabs, 1, std::max(1, nGlobal.z));
  std::vector<Box> boxes;
  boxes.reserve(nSlabs);
  for (int s = 0; s < nSlabs; ++s) {
    const int k0 = int((long long)nGlobal.z * s / nSlabs);
    const int k1 = int((long long)nGlobal.z * (s + 1) / nSlabs);
    boxes.push_back(Box{int3(nGlobal.x, nGlobal.y, k1 - k0), int3(0, 0, k0), nGlobal, halo});
  }
  return boxes;
}
