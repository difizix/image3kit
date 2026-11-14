// Box remaps of the image, output of another size (run_remap): voxlib cropD (with or without
// emptylyrs), growBox and shrinkBox. Output voxel o reads the input at p = o + off:
//   fill == 0: input(clamp(p, 0, n-1))                 (growBox: voxlib replicates the boundary layers)
//   fill != 0: p inside [bgn, end) ? input(p) : fillValue   (cropD's emptylyrs layers)

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class RemapBox : public Generator<RemapBox> {
public:
    VXHL_STENCIL_INPUTS
    Input<int32_t> offx{"offx"}, offy{"offy"}, offz{"offz"};
    Input<int32_t> bgnx{"bgnx"}, bgny{"bgny"}, bgnz{"bgnz"}, endx{"endx"}, endy{"endy"}, endz{"endz"};
    Input<int32_t> fill{"fill"};
    Input<double> fillValue{"fillValue"};
    Output<Buffer<void, 3>> output{"output"};

    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        const Expr px = x + offx, py = y + offy, pz = z + offz;
        const Expr v = input(clamp(px, 0, gnx - 1), clamp(py, 0, gny - 1), clamp(pz, 0, gnz - 1));
        const Expr inside = bgnx <= px && px < endx && bgny <= py && py < endy && bgnz <= pz && pz < endz;
        output(x, y, z) = select(fill != 0 && !inside, cast(output.type(), fillValue), v);
    }

    void schedule() {
        if (get_target().has_gpu_feature()) {
            Var xi{"xi"}, yi{"yi"};
            output.gpu_tile(x, y, xi, yi, 32, 8, TailStrategy::GuardWithIf);
        } else {
            vxhl_gen::schedule_stencil(gnx, output, get_target(), output.type());
        }
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(RemapBox, remapBox)
