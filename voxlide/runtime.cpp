#include "runtime.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <mutex>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef IMAGE3KIT_HAS_HALIDE
#if defined(__APPLE__)
#include "HalideRuntimeMetal.h"
#define VXHL_GPU_INTERFACE halide_metal_device_interface
#else
#include "HalideRuntimeVulkan.h"
#define VXHL_GPU_INTERFACE halide_vulkan_device_interface
#endif
#endif

namespace vxhl {

namespace {

std::atomic<int> g_backend{-1};          // -1: not set, use $IMAGE3KIT_BACKEND
std::atomic<std::size_t> g_slab_bytes{0};  // 0: default

#ifdef IMAGE3KIT_HAS_HALIDE
// Halide may report errors from its worker threads, so this is shared, not thread_local.
std::mutex g_err_mutex;
std::string g_halide_error;

extern "C" void vxhl_error_handler(void*, const char* msg) {
    std::lock_guard<std::mutex> lk(g_err_mutex);
    g_halide_error += msg ? msg : "";
}

void ensure_init() {
    static std::once_flag once;
    std::call_once(once, [] {
        halide_set_error_handler(vxhl_error_handler);
#ifdef _OPENMP
        halide_set_num_threads(omp_get_max_threads());
#endif
    });
}

bool gpu_usable() {
    static const bool ok = [] {
        ensure_init();
        const halide_device_interface_t* iface = VXHL_GPU_INTERFACE();
        if (!iface) return false;
        uint8_t dummy[8] = {0};
        Halide::Runtime::Buffer<uint8_t, 1> buf(dummy, 8);
        const bool good = buf.device_malloc(iface) == 0;
        if (good) buf.device_free();
        (void)detail::take_halide_error();  // a failed probe is not an error
        return good;
    }();
    return ok;
}
#else
void ensure_init() {}
bool gpu_usable() { return false; }
#endif

constexpr bool kHalideBuilt =
#ifdef IMAGE3KIT_HAS_HALIDE
    true;
#else
    false;
#endif

}  // namespace

Backend parse_backend(const std::string& name) {
    std::string k = name;
    std::transform(k.begin(), k.end(), k.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    if (k.empty() || k == "auto") return Backend::Auto;
    if (k == "voxlib" || k == "cpu" || k == "omp") return Backend::Voxlib;
    if (k == "halide" || k == "halide_cpu") return Backend::HalideCpu;
    if (k == "gpu" || k == "vulkan" || k == "metal") return Backend::Gpu;
    throw std::invalid_argument("unknown image3kit backend '" + name +
                                "' (expected auto, voxlib, halide or gpu)");
}

const char* backend_name(Backend b) {
    switch (b) {
        case Backend::Auto:      return "auto";
        case Backend::Voxlib:    return "voxlib";
        case Backend::HalideCpu: return "halide";
        case Backend::Gpu:       return "gpu";
    }
    return "?";
}

void set_backend(Backend b) { g_backend = int(b); }

Backend get_backend() {
    const int b = g_backend;
    if (b >= 0) return Backend(b);
    static const Backend fromEnv = [] {
        const char* e = std::getenv("IMAGE3KIT_BACKEND");
        return e ? parse_backend(e) : Backend::Auto;
    }();
    return fromEnv;
}

std::vector<Backend> available_backends() {
    std::vector<Backend> v{Backend::Voxlib};
    if (kHalideBuilt) v.push_back(Backend::HalideCpu);
    if (gpu_usable()) v.push_back(Backend::Gpu);
    return v;
}

void set_max_slab_bytes(std::size_t bytes) { g_slab_bytes = bytes; }

namespace detail {

std::size_t max_slab_bytes(Backend b) {
    const std::size_t s = g_slab_bytes;
    if (s) return s;
    return b == Backend::Gpu ? (std::size_t(1) << 29) : (std::size_t(1) << 30);
}

std::string take_halide_error() {
#ifdef IMAGE3KIT_HAS_HALIDE
    std::string m;
    {
        std::lock_guard<std::mutex> lk(g_err_mutex);
        std::swap(m, g_halide_error);
    }
    while (!m.empty() && (m.back() == '\n' || m.back() == ' ')) m.pop_back();
    return m.empty() ? std::string("Halide pipeline failed") : m;
#else
    return "voxlide built without Halide";
#endif
}

Choice resolve(const OpTraits& op, bool hasCpu, bool hasGpu, bool gpuBroken) {
    ensure_init();
    switch (get_backend()) {
        case Backend::Voxlib:
            return {Backend::Voxlib, true};
        case Backend::HalideCpu:
            if (!hasCpu) throw std::runtime_error(std::string("vxhl::") + op.name + ": no Halide CPU kernel in this build");
            return {Backend::HalideCpu, true};
        case Backend::Gpu:
            if (!hasGpu) throw std::runtime_error(std::string("vxhl::") + op.name + ": no GPU kernel in this build");
            if (!gpu_usable()) throw std::runtime_error(std::string("vxhl::") + op.name + ": no usable GPU device");
            return {Backend::Gpu, true};
        case Backend::Auto:
            break;
    }
    // No size or speed thresholds: Auto never offloads to the CPU while the GPU works.
    if (hasGpu && !gpuBroken && gpu_usable()) return {Backend::Gpu, false};
    if (hasCpu) return {Backend::HalideCpu, false};
    return {Backend::Voxlib, false};
}

}  // namespace detail
}  // namespace vxhl
