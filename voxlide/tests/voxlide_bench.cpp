//! Throughput of vxhl:: ops per backend vs voxlib (not run by ctest).
//! Usage: voxlide_bench [n=256] [reps=3] [block=6] [backend=all] [op=all]
//! ops: mode26, median, median5 (5 iterations: device-resident ping-pong),
//!      bilat1x1 / bilat2x3 (smooth_bilateral nItrs x kernRad, f64),
//!      wide1r1x2 / wide3r2x2 (bilateral_wide nItrs, kernel_radius, x_step 2; f32).
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <limits>
#include <string>

#include "parity_harness.h"

using Img = VoxelImageT<unsigned char>;

double best_ms(const Img& input, int reps, const std::function<void(Img&)>& fn) {
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
    const std::string onlyBackend = argc > 4 ? argv[4] : "all";
    const std::string onlyOp = argc > 5 ? argv[5] : "all";
    const auto input = vxhl::testing::labelled_volume<unsigned char>(int3(n, n, n), 4, 0.25, 999, block);
    const double mvox = double(n) * n * n / 1e3;
    const std::pair<const char*, std::function<void(Img&)>> ops[] = {
        {"mode26", [](Img& m) { vxhl::mode26(m, short(2)); }},
        {"median", [](Img& m) { m = vxhl::median(m, 1); }},
        {"median5", [](Img& m) { m = vxhl::median(m, 5); }},
        {"bilat1x1", [](Img& m) { vxhl::smoothBilateral(m, 1, 1, 16.0, 0.1); }},
        {"bilat2x3", [](Img& m) { vxhl::smoothBilateral(m, 2, 3, 16.0, 0.1); }},
        {"wide1r1x2", [](Img& m) { vxhl::_bilateralX(m, 1, 1, 2, 256.0, 0.1, 4.0); }},
        {"wide3r2x2", [](Img& m) { vxhl::_bilateralX(m, 3, 2, 2, 256.0, 0.1, 4.0); }},
    };
    // GPU first: on APUs (shared CPU/GPU power budget) a preceding all-core CPU run
    // throttles the GPU for a while, which skews its timing by ~1.6x.
    auto backends = vxhl::available_backends();
    std::reverse(backends.begin(), backends.end());
    std::printf("u8 %d^3, best of %d\n", n, reps);
    for (const auto& [name, fn] : ops) {
        if (onlyOp != "all" && onlyOp != name) continue;
        for (vxhl::Backend b : backends) {
            if (onlyBackend != "all" && vxhl::parse_backend(onlyBackend) != b) continue;
            vxhl::set_backend(b);
            best_ms(input, 1, fn);  // warm-up (GPU context, shader compile)
            const double ms = best_ms(input, reps, fn);
            std::printf("  %-8s %-7s %9.2f ms  %8.1f MVox/s\n", name, vxhl::backend_name(b), ms, mvox / ms);
        }
    }
    return 0;
}
