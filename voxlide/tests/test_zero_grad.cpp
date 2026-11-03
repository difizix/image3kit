#include "utest.h"
#include "parity_harness.h"
#include "voxelImageI.h"

namespace {
auto make = [](auto tag, int3 n) { return vxhl::testing::labelled_volume<decltype(tag)>(n, 20, 0.5, 222, 3); };
// voxlib indexes out of range on images with fewer than 2n+1 voxels along an axis: skip those shapes.
bool fits(int3 s, int n) { return s.x >= 2 * n + 1 && s.y >= 2 * n + 1 && s.z >= 2 * n + 1; }
}  // namespace

#define ZEROGRAD_PARITY(NAME, N)                                                                \
    VXHL_PARITY_TEST(NAME, make, [](auto& m) { if (fits(m.size3(), N)) m.zeroGrad(N); },        \
                     [](auto& m) { if (fits(m.size3(), N)) vxhl::zeroGrad(m, N); })

ZEROGRAD_PARITY(zero_grad_1, 1)
ZEROGRAD_PARITY(zero_grad_2, 2)
ZEROGRAD_PARITY(zero_grad_3, 3)
