// Halide port of ::mapToFrom(vImage, vimg2, vmin, vmax, scale, shift), voxlib/voxelImageI.h:1640
// (Python map_from) for a source image on the SAME grid (equal size, Dn == 0; other cases run
// voxlib): voxels with vmin <= v <= vmax take the source image's value, then, if scale > 1e-16,
// every voxel becomes T(v·scale + shift) (double arithmetic, C++ conversion).

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class MapFrom : public Generator<MapFrom> {
public:
    VXHL_STENCIL_INPUTS
    Input<Buffer<void, 3>> src{"src"};  // Aux image
    Input<Expr> vmin{"vmin"}, vmax{"vmax"};  // voxel type (VALUES)
    Input<double> scale{"scale"}, shift{"shift"};
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};
    void generate() {
        const Type t = input.type();
        const Expr v = input(x, y, z);
        const Expr mapped = select(vmin <= v && v <= vmax, src(x, y, z), v);
        const Expr sc = cast<double>(mapped) * scale + shift;
        // T(double): float -> rounds; integers -> truncate (via int32, as elsewhere)
        const Expr conv = t.is_float() ? cast(t, sc) : cast(t, cast<int32_t>(sc));
        output(x, y, z) = select(scale > Internal::make_const(Float(64), 1e-16), conv, mapped);
    }
    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(MapFrom, mapFrom)
