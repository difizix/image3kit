// voxlib VoxelImageT::rotate(direction) (Python redirect / direction): output of another shape (run_remap).
//   swapXZ ('z'): out(x, y, z) = in(z, y, x);  swapXY ('y'): out(x, y, z) = in(y, x, z);
//   flipX  ('-'): out(x, y, z) = in(nx-1-x, y, z).
// One kernel per mode, so bounds inference asks each slab for only the input layers it reads.

#include "vxhl_gen.h"

namespace {

using namespace Halide;

template <int Mode>
class Redirect : public Generator<Redirect<Mode>> {
public:
    template <typename T> using Input = typename Generator<Redirect<Mode>>::template Input<T>;
    template <typename T> using Output = typename Generator<Redirect<Mode>>::template Output<T>;
    VXHL_STENCIL_INPUTS
    Output<Buffer<void, 3>> output{"output"};

    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        if constexpr (Mode == 0) output(x, y, z) = input(z, y, x);
        else if constexpr (Mode == 1) output(x, y, z) = input(y, x, z);
        else output(x, y, z) = input(gnx - 1 - x, y, z);
    }

    void schedule() {
        if (this->get_target().has_gpu_feature()) {
            Var xi{"xi"}, yi{"yi"};
            output.gpu_tile(x, y, xi, yi, 16, 16, TailStrategy::GuardWithIf);
        } else {
            vxhl_gen::schedule_stencil(this->gnx, output, this->get_target(), output.type());
        }
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(Redirect<0>, swapXZ)
HALIDE_REGISTER_GENERATOR(Redirect<1>, swapXY)
HALIDE_REGISTER_GENERATOR(Redirect<2>, flipX)
