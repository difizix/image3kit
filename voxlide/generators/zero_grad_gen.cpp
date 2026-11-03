// Halide port of VoxelImageT<T>::zeroGrad(int nLayers), voxlib/voxelImageI.h:797 (segment2):
// the outer nLayers layers on every face are replaced by the nearest interior layer (y, then x,
// then z layer copies). For images with >= 2·nLayers+1 voxels per axis that equals reading
// the input at coordinates clamped to [nLayers, n-1-nLayers]; smaller images make voxlib index
// out of range (not reproduced). Halo = nLayers (passed as the radius).

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class ZeroGrad : public Generator<ZeroGrad> {
public:
    VXHL_STENCIL_INPUTS
    Input<int32_t> n{"n"};
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        output(x, y, z) = input(clamp(x, n, gnx - 1 - n), clamp(y, n, gny - 1 - n), clamp(z, n, gnz - 1 - n));
    }
    void schedule() { vxhl_gen::schedule_stencil(output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(ZeroGrad, zeroGrad)
