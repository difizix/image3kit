#include "utest.h"
#include "parity_harness.h"
#include "voxelImageML.h"

namespace {
// 4 labels, 35% noise: plenty of 2- and 3-way frequency ties in the 26-neighbourhood.
auto make = [](auto tag, int3 n) { return vxhl::testing::labelled_volume<decltype(tag)>(n, 4, 0.35, 101); };
}  // namespace

// minDif values cover 0, odd (minDif/2 truncation), negative (truncation toward zero), large.
#define MODE26_PARITY(NAME, D) \
    VXHL_PARITY_TEST(NAME, make, [](auto& m) { ::mode26(m, short(D)); }, [](auto& m) { vxhl::mode26(m, short(D)); })

MODE26_PARITY(mode26_d0, 0)
MODE26_PARITY(mode26_d1, 1)
MODE26_PARITY(mode26_d2, 2)
MODE26_PARITY(mode26_dm3, -3)
MODE26_PARITY(mode26_d6, 6)
