// voxlib threshold01_otsu's apply step (voxelImageML.h): a u8 mask, 0 where double(v) < threshold, else 1
// (NaN voxels get 1, as voxlib's `<`). Output type u8 for every input type (OUTPUT_TYPE, run_remap).

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class Threshold01 : public Generator<Threshold01> {
public:
    VXHL_STENCIL_INPUTS
    Input<double> threshold{"threshold"};
    Output<Buffer<void, 3>> output{"output"};

    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        output(x, y, z) = select(cast<double>(input(x, y, z)) < threshold, cast(output.type(), 0), cast(output.type(), 1));
    }

    void schedule() {
        if (get_target().has_gpu_feature()) {
            Var xi{"xi"}, yi{"yi"};
            output.gpu_tile(x, y, xi, yi, 32, 8, TailStrategy::GuardWithIf);
        } else {
            vxhl_gen::schedule_stencil(output, get_target(), input.type());
        }
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(Threshold01, threshold01)
