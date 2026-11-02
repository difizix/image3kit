//! Throughput of vxhl:: ops per backend vs voxlib (not run by ctest).
//! Usage: voxlide_bench [n=256] [reps=3] [block=6] [backend=all]
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>

#include "parity_harness.h"
#include "voxelImageML.h"

template <typename Fn>
double best_ms(const VoxelImageT<unsigned char>& input, int reps, Fn fn) {
    double best = std::numeric_limits<double>::infinity();
    for (int r = 0; r < reps; ++r) {
        auto img = input;
        auto t0 = std::chrono::steady_clock::now();
        fn(img);
        best = std::min(best, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());
    }
    return best;
}

int main(int argc, char** argv) {
    const int n = argc > 1 ? std::atoi(argv[1]) : 256;
    const int reps = argc > 2 ? std::atoi(argv[2]) : 3;
    const int block = argc > 3 ? std::atoi(argv[3]) : 6;
    const auto input = vxhl::testing::labelled_volume<unsigned char>(int3(n, n, n), 4, 0.25, 999, block);
    const double mvox = double(n) * n * n / 1e3;
    std::printf("mode26<u8> %d^3, best of %d\n", n, reps);
    const std::string only = argc > 4 ? argv[4] : "";
    // GPU first: on APUs (shared CPU/GPU power budget) a preceding all-core CPU run
    // throttles the GPU for a while, which skews its timing by ~1.6x.
    auto backends = vxhl::available_backends();
    std::reverse(backends.begin(), backends.end());
    for (vxhl::Backend b : backends) {
        if (!only.empty() && vxhl::parse_backend(only) != b) continue;
        vxhl::set_backend(b);
        auto fn = [](auto& m) { vxhl::mode26(m, short(2)); };
        best_ms(input, 1, fn);  // warm-up (GPU context, shader compile)
        const double ms = best_ms(input, reps, fn);
        std::printf("  %-7s %9.2f ms  %8.1f MVox/s\n", vxhl::backend_name(b), ms, mvox / ms);
    }
    return 0;
}
