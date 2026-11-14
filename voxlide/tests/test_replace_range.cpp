#include "utest.h"
#include "parity_harness.h"
#include "voxelImageI.h"

namespace {
auto make = [](auto tag, int3 n) { return vxhl::testing::labelled_volume<decltype(tag)>(n, 20, 0.4, 111, 3); };
}  // namespace

#define REPLACE_PARITY(NAME, LO, HI, MID)                                                                        \
    VXHL_PARITY_TEST(NAME, make,                                                                                 \
                     [](auto& m) { using T = std::decay_t<decltype(m(0, 0, 0))>; ::replaceRange(m, T(LO), T(HI), T(MID)); }, \
                     [](auto& m) { using T = std::decay_t<decltype(m(0, 0, 0))>; vxhl::replaceRange(m, T(LO), T(HI), T(MID)); })

REPLACE_PARITY(replace_range_one, 25, 25, 24)
REPLACE_PARITY(replace_range_span, 30, 120, 77)
REPLACE_PARITY(replace_range_empty, 120, 30, 0)
