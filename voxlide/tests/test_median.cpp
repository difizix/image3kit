#include "utest.h"
#include "parity_harness.h"
#include "voxelImageML.h"

namespace {
// 20 labels (fits u8), 50% noise: many distinct values and repeated values in each 7-point set.
auto make = [](auto tag, int3 n) { return vxhl::testing::labelled_volume<decltype(tag)>(n, 20, 0.5, 202); };
}  // namespace

// nIter > 1 exercises the device-resident ping-pong (one slab) and per-iteration slab passes.
#define MEDIAN_PARITY(NAME, N) \
    VXHL_PARITY_TEST(NAME, make, [](auto& m) { m = ::median(m, N); }, [](auto& m) { m = vxhl::median(m, N); })

MEDIAN_PARITY(median_n1, 1)
MEDIAN_PARITY(median_n2, 2)
MEDIAN_PARITY(median_n5, 5)
