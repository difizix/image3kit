// Pointwise operations between the image and a second image of the same size and type (Aux):
//   code 0  a && b            voxlib VoxelImageT<T>::AND  (Python and_)   -> 1 or 0
//   code 1  a || b            ::OR   (or_)
//   code 2  a != b            ::XOR  (xor_; the VALUES differ, not a logical xor: 2 xor 3 = 1)
//   code 3  a && !b           ::NOT  (not_)
//   code 4  a + b             addEq  (Python +=, +)   in the voxel type, wrapping for 8/16-bit
//   code 5  a - b             subEq  (-=, -)
// voxlib/voxelImageI.h. Truthiness is "!= 0".

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class PairOp : public Generator<PairOp> {
public:
    VXHL_STENCIL_INPUTS
    Input<Buffer<void, 3>> other{"other"};  // Aux image (voxlide_add_op IMAGES)
    Input<int32_t> code{"code"};
    Output<Buffer<void, 3>> output{"output"};

    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        const Type t = input.type();
        const Expr a = input(x, y, z), b = other(x, y, z);
        const Expr zero = cast(t, 0), one = cast(t, 1);
        const Expr na = a != zero, nb = b != zero;
        output(x, y, z) = select(code == 0, select(na && nb, one, zero),
                          select(code == 1, select(na || nb, one, zero),
                          select(code == 2, select(a != b, one, zero),
                          select(code == 3, select(na && !nb, one, zero),
                          select(code == 4, a + b, a - b)))));
    }
    void schedule() { vxhl_gen::schedule_stencil(output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(PairOp, pairOp)
