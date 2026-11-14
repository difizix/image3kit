#include "utest.h"
#include "parity_harness.h"
#include "voxelImageI.h"

namespace {
// Few labels with heavy noise: voxels with 0..5 equal neighbours and frequent count ties.
auto make = [](auto tag, int3 n) { return vxhl::testing::labelled_volume<decltype(tag)>(n, 6, 0.5, 606, 3); };
auto make_sparse = [](auto tag, int3 n) { return vxhl::testing::labelled_volume<decltype(tag)>(n, 3, 0.05, 607, 5); };
}  // namespace

#define MODE6_PARITY(NAME, MAKE, N)                                                     \
    VXHL_PARITY_TEST_COUNT(NAME, MAKE, [](auto& m) { return (long long)::modeNSames(m, N); }, \
                           [](auto& m) { return (long long)vxhl::modeNSames(m, N); })

// nSameNei 5: UB in voxlib for >= 6 (all six neighbours equal), so not tested.
MODE6_PARITY(mode6_n0, make, 0)
MODE6_PARITY(mode6_n1, make, 1)
MODE6_PARITY(mode6_n2, make, 2)
MODE6_PARITY(mode6_n4, make, 4)
MODE6_PARITY(mode6_n5, make_sparse, 5)
MODE6_PARITY(mode6_negative, make, -1)
