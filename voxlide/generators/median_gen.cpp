// Halide port of ::median(const VoxelImageT<T>&, int nIter), voxlib/voxelImageI.h (one iteration;
// the runtime iterates). Semantics reproduced exactly:
//  - only voxels >= 1 from every image face are changed (forAllkji_1_); the rest are copied;
//  - the result is the 4th smallest of the centre and its 6 face neighbours (nth_element at 3).
//    The median of values is unique, so tie order does not matter.

#include "vxhl_gen.h"

namespace {

using namespace Halide;

class Median : public Generator<Median> {
public:
    VXHL_STENCIL_INPUTS
    Output<Buffer<void, 3>> output{"output"};

    Var x{"x"}, y{"y"}, z{"z"};

    void generate() {
        Func in = BoundaryConditions::repeat_edge(input);
        const Expr p = in(x, y, z);
        Expr v[7] = {p, in(x - 1, y, z), in(x + 1, y, z), in(x, y - 1, z),
                     in(x, y + 1, z), in(x, y, z - 1), in(x, y, z + 1)};
        // Optimal 16-comparator sorting network for 7 inputs; unused outputs are dead code.
        static constexpr int net[][2] = {{0, 6}, {2, 3}, {4, 5}, {0, 2}, {1, 4}, {3, 6}, {0, 1}, {2, 5},
                                         {3, 4}, {1, 2}, {4, 6}, {2, 3}, {4, 5}, {1, 2}, {3, 4}, {5, 6}};
        for (const auto& c : net) {
            const Expr lo = min(v[c[0]], v[c[1]]), hi = max(v[c[0]], v[c[1]]);
            v[c[0]] = lo;
            v[c[1]] = hi;
        }
        output(x, y, z) = select(vxhl_gen::interior(x, y, z, gnx, gny, gnz, 1), v[3], p);
    }

    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(Median, median)
