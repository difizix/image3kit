// Halide port of ::mean(vImg) (voxlib/voxelImageML.h:638): in the interior (one layer in from the faces,
// forAllkji_1_) v = T(0.5 + double(Σ 27 neighbours)/27), the sum in an int (k, j, i order; exact in any order for
// integer voxels), the rest copied. Integer voxel types only (floats truncate at every step of the int sum).

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class MeanFilter : public Generator<MeanFilter> {
public:
    VXHL_STENCIL_INPUTS
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};
    Func in{"in"}, acc{"acc"};

    void generate() {
        in = BoundaryConditions::repeat_edge(input);
        RDom r(-1, 3, -1, 3, -1, 3);
        acc(x, y, z) = 0;
        acc(x, y, z) += cast<int32_t>(in(x + r.x, y + r.y, z + r.z));
        const Expr mean = Internal::make_const(Float(64), 0.5) + vxhl_gen::div64(cast<double>(acc(x, y, z)), Internal::make_const(Float(64), 27.0), get_target());
        output(x, y, z) = select(vxhl_gen::interior(x, y, z, gnx, gny, gnz, 1), cast(output.type(), cast<int32_t>(mean)), input(x, y, z));
    }
    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), Float(64), {acc}); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(MeanFilter, meanFilter)
