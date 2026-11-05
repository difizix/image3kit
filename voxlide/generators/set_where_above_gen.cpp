// meanWide's `forAlliii_(grad) if (grad(iii) > tresh) vImg(iii) = val;`: other is a second image of the same size and type (Aux).

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class SetWhereAbove : public Generator<SetWhereAbove> {
public:
    VXHL_STENCIL_INPUTS
    Input<Buffer<void, 3>> other{"other"};  // Aux image
    Input<Expr> thresh{"thresh"}, value{"value"};  // voxel type (VALUES)
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};
    void generate() { output(x, y, z) = select(other(x, y, z) > thresh, value, input(x, y, z)); }
    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(SetWhereAbove, setWhereAbove)
