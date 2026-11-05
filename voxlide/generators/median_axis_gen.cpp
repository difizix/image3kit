// Halide port of ::medianx / ::mediany / ::medianz(const VoxelImageT<T>&), voxlib/voxelImageI.h:1156
// (Python median_x/_y/_z): the median of a voxel and its two neighbours along one axis (axis 0, 1, 2).
// Only voxels >= 1 from EVERY image face are changed (forAllkji_1_), the rest are copied.

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class MedianAxis : public Generator<MedianAxis> {
public:
    VXHL_STENCIL_INPUTS
    Input<int32_t> axis{"axis"};
    Output<Buffer<void, 3>> output{"output"};

    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        Func in = BoundaryConditions::repeat_edge(input);
        const Expr dx = select(axis == 0, 1, 0), dy = select(axis == 1, 1, 0), dz = select(axis == 2, 1, 0);
        const Expr a = in(x, y, z), b = in(x - dx, y - dy, z - dz), c = in(x + dx, y + dy, z + dz);
        const Expr med = max(min(a, b), min(max(a, b), c));
        output(x, y, z) = select(vxhl_gen::interior(x, y, z, gnx, gny, gnz, 1), med, a);
    }

    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(MedianAxis, medianAxis)
