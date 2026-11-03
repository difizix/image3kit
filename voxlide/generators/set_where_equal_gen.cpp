// Pointwise merge used by delense032 (voxlib/voxelImageI.h `_delense032`, last loop):
//   if (other(p) == value) img(p) = value;     (voxlib: forAlliii_(vimgo) if(vimgo(iii)==lbl1) vImg(iii)=lbl1)
// `other` is a second image of the same size and type (Aux).

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class SetWhereEqual : public Generator<SetWhereEqual> {
public:
    VXHL_STENCIL_INPUTS
    Input<Buffer<void, 3>> other{"other"};  // Aux image
    Input<Expr> value{"value"};             // voxel type (VALUES)
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};
    void generate() { output(x, y, z) = select(other(x, y, z) == value, value, input(x, y, z)); }
    void schedule() { vxhl_gen::schedule_stencil(output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(SetWhereEqual, setWhereEqual)
