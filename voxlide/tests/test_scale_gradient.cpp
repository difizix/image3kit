#include "utest.h"
#include "parity_harness.h"
#include "voxelImageML.h"

namespace {
auto make = [](auto tag, int3 n) {
    using T = decltype(tag);
    auto img = vxhl::testing::labelled_volume<T>(n, 20, 0.4, 444, 3);
    if constexpr (!std::is_same_v<T, unsigned char>)
        for (auto& v : img.data_) v = T(double(v) * 300.0);  // scaled values reach maxT (cap)
    return img;
};
}  // namespace

// Integer types only (no kernel for floats: an explicit backend throws for them).
#define INT_ONLY if constexpr (std::is_floating_point_v<std::decay_t<decltype(m(0, 0, 0))>>) return;
#define SCALEGRAD_PARITY(NAME, AVG)                                                                  \
    VXHL_PARITY_TEST(NAME, make, [](auto& m) { INT_ONLY ::scaleGradient(m, AVG); },                  \
                     [](auto& m) { INT_ONLY vxhl::scaleGradient(m, AVG); })

SCALEGRAD_PARITY(scale_gradient_mid, 60.5)
SCALEGRAD_PARITY(scale_gradient_small, 0.37)  // most voxels scaled, many capped
SCALEGRAD_PARITY(scale_gradient_large, 1e9)   // everything below: 10

// average2p: exact (integer sums); floats run voxlib on every backend.
#define AVG2P_PARITY(NAME, MINV)                                                                                   \
    VXHL_SCALAR_PARITY_TEST(NAME, make,                                                                            \
        [](const auto& m) { return double(::average2p(m, std::decay_t<decltype(m(0, 0, 0))>(MINV))); },            \
        [](const auto& m) { return double(vxhl::average2p(m, std::decay_t<decltype(m(0, 0, 0))>(MINV))); }, 0.0)
AVG2P_PARITY(average2p_2, 2)
AVG2P_PARITY(average2p_100, 100)
