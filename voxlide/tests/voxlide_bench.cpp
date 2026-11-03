//! Throughput of vxhl:: ops per backend vs voxlib (not run by ctest).
//! Usage: voxlide_bench [n=256] [reps=3] [block=6] [backend=all] [op=all]
//! ops: mode26, mode6 (returns the change count), shrink (Aux source image), mean / variance (reductions: 1 / 2 passes),
//!      maggrad (magGradient, f64; GPU: exact sqrt emulation),
//!      segment2 (Python defaults, 2 segments; whole pipeline on DeviceImages),
//!      replace / zerograd / scalegrad / avg2p (replaceRange, zeroGrad, scaleGradient, average2p),
//!      median032 / median06grow / median06 / growlabel (the median family on label images),
//!      growpore / shrinkpore / medianx / mediany (grow0, shrink0, median_x, median_y),
//!      and / add / op_img / op_scalar / thresh101 / circleout / mapfrom (pointwise ops),
//!      dev20 (20 × shrink on one DeviceImage),
//!      chain / chain_dev (mode26, median×3, shrink, mode6, mode26 on VoxelImageT / DeviceImage), median, median5 (5 iterations: device-resident ping-pong),
//!      bilat1x1 / bilat2x3 (smooth_bilateral nItrs x kernRad, f64),
//!      gauss1r1 / gauss3r2 (bilateral_gauss nItrs, kernel_radius; f32),
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
        {"mode6", [](Img& m) { vxhl::modeNSames(m, short(2)); }},
        {"shrink", [&](Img& m) { vxhl::shrink(m, (unsigned char)5, (unsigned char)15, input); }},
        {"mean", [](Img& m) { volatile double r = vxhl::meanDbl(m); (void)r; }},
        {"variance", [](Img& m) { volatile double r = vxhl::varianceDbl(m, 0, 255); (void)r; }},
        {"chain", [&](Img& m) {  // 5 ops, each through host memory
             vxhl::mode26(m, 2); m = vxhl::median(m, 3); vxhl::shrink(m, (unsigned char)5, (unsigned char)15, input);
             vxhl::modeNSames(m, 2); vxhl::mode26(m, 1); }},
        {"chain_dev", [&](Img& m) {  // the same on a DeviceImage: one upload, one download
             vxhl::DeviceImage<unsigned char> d(std::move(m));
             vxhl::mode26(d, 2); vxhl::median(d, 3); vxhl::shrink(d, (unsigned char)5, (unsigned char)15, input);
             vxhl::modeNSames(d, 2); vxhl::mode26(d, 1); m = d.host(); }},
        {"dev20", [&](Img& m) {  // 20 ops on one DeviceImage (work-buffer reuse)
             vxhl::DeviceImage<unsigned char> d(std::move(m));
             for (int i = 0; i < 20; ++i) vxhl::shrink(d, (unsigned char)5, (unsigned char)15, input);
             m = d.host(); }},
        {"maggrad", [](Img& m) { m = vxhl::magGradient(m, 2.0); }},
        {"segment2", [](Img& m) {  // 2 segments, krnl 2, 13 iterations (Python defaults)
             vars<int> th(std::vector<int>{1, 20, 254}), mins(std::vector<int>{1, 2});
             vxhl::segment2(m, 2, th, mins, 2.0, 0.05, 0.1, 2.0, 0.0, 2, 13, 0); }},
        {"replace", [](Img& m) { vxhl::replaceRange(m, (unsigned char)5, (unsigned char)15, (unsigned char)9); }},
        {"zerograd", [](Img& m) { vxhl::zeroGrad(m, 2); }},
        {"scalegrad", [](Img& m) { vxhl::scaleGradient(m, 60.5); }},
        {"avg2p", [](Img& m) { volatile int r = vxhl::average2p(m, (unsigned char)2); (void)r; }},
        {"median032", [](Img& m) { vxhl::median032(m, 14, 14, (unsigned char)15, (unsigned char)25); }},
        {"median06grow", [](Img& m) { vxhl::FaceMedGrowToFrom(m, (unsigned char)15, (unsigned char)5, 0); }},
        {"median06", [](Img& m) { vxhl::median06(m, 2, 4); }},
        {"growlabel", [](Img& m) { vxhl::growLabel(m, (unsigned char)15); }},
        {"growpore", [](Img& m) { vxhl::growPore(m); }},
        {"shrinkpore", [](Img& m) { vxhl::shrinkPore(m); }},
        {"medianx", [](Img& m) { m = vxhl::medianx(m); }},
        {"mediany", [](Img& m) { m = vxhl::mediany(m); }},
        {"and", [&](Img& m) { vxhl::and_(m, input); }},
        {"add", [&](Img& m) { vxhl::addEq(m, input); }},
        {"op_img", [&](Img& m) { vxhl::operateImg(m, "+", input, 0.0); }},
        {"op_scalar", [](Img& m) { vxhl::operateScalar(m, "+", 20.0); }},
        {"thresh101", [](Img& m) { vxhl::threshold101(m, (unsigned char)5, (unsigned char)15); }},
        {"circleout", [](Img& m) { vxhl::circleOut(m, 128, 128, 100, 'z', (unsigned char)9); }},
        {"mapfrom", [&](Img& m) { vxhl::mapToFrom(m, input, (unsigned char)5, (unsigned char)5, 1.0, 0.5); }},
        {"median", [](Img& m) { m = vxhl::median(m, 1); }},
        {"median5", [](Img& m) { m = vxhl::median(m, 5); }},
        {"bilat1x1", [](Img& m) { vxhl::smoothBilateral(m, 1, 1, 16.0, 0.1); }},
        {"bilat2x3", [](Img& m) { vxhl::smoothBilateral(m, 2, 3, 16.0, 0.1); }},
        {"gauss1r1", [](Img& m) { vxhl::_bilateralGauss(m, 1, 1, 256.0, 0.1, 4.0); }},
        {"gauss3r2", [](Img& m) { vxhl::_bilateralGauss(m, 3, 2, 256.0, 0.1, 4.0); }},
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
