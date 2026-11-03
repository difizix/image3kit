cmake_minimum_required(VERSION 3.28...4.0)

project(voxlide LANGUAGES CXX)

# voxlide is always built; without Halide every vxhl:: op runs its voxlib reference.
add_library(voxlide STATIC runtime.cpp ops.cpp)
set(VOXLIDE_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_include_directories(voxlide PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${VOXLIDE_GEN_DIR})
target_link_libraries(voxlide PUBLIC voxlib PRIVATE OpenMP::OpenMP_CXX)
set_target_properties(voxlide PROPERTIES CXX_STANDARD 23 POSITION_INDEPENDENT_CODE ON)

# ---- Halide: IMAGE3KIT_HALIDE = AUTO (use an installed Halide if found) | ON (also fetch) | OFF
set(_VOXLIDE_HALIDE FALSE)
if(NOT IMAGE3KIT_HALIDE STREQUAL "OFF")
  if(DEFINED Python_EXECUTABLE AND NOT Halide_DIR)  # the `halide` PyPI wheel ships a CMake package
    execute_process(
      COMMAND "${Python_EXECUTABLE}" -c
              "import pathlib, sysconfig; r = pathlib.Path(sysconfig.get_paths()['platlib']) / 'halide'; print(next(iter(sorted(r.glob('lib*/cmake/Halide'))), ''))"
      OUTPUT_VARIABLE _py_halide_dir OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if(_py_halide_dir)
      set(Halide_DIR "${_py_halide_dir}")
    endif()
  endif()
  find_package(Halide QUIET)

  if(NOT Halide_FOUND AND IMAGE3KIT_HALIDE STREQUAL "ON")
    set(_v "21.0.0")
    set(_c "b629c80de18f1534ec71fddd8b567aa7027a0876")
    set(_u "https://github.com/halide/Halide/releases/download/v${_v}/Halide-${_v}")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
      set(_url "${_u}-x86-64-linux-${_c}.tar.gz")  # sha256 from the GitHub release asset digests
      set(_sha b56139ddc5d863486b9b339e1c9b7cc3f6aadd4dd8a2eff2202e79ca68706091)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
      set(_url "${_u}-arm-64-linux-${_c}.tar.gz")
      set(_sha 6fa8be9a556ddf3e899a0db59af84d69d0bebbf91ead90c03f90f731ab314d95)
    elseif(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
      set(_url "${_u}-arm-64-osx-${_c}.tar.gz")
      set(_sha 040a6fbde5ba264870df4975138417ce2ff2c8e9de550302c8b17f36c36e5afa)
    elseif(APPLE)
      set(_url "${_u}-x86-64-osx-${_c}.tar.gz")
      set(_sha d7d26c91adcfe62528e20e248ba673aa635de519669b19c47e5a367f856a8ab0)
    elseif(WIN32 AND CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64")
      set(_url "${_u}-x86-64-windows-${_c}.zip")
      set(_sha 4efec94b7c8958b1ae0125a73245a148e4c98dbb54f2678bc34c6abe36ee899a)
    endif()
    if(_url)
      message(STATUS "voxlide: fetching Halide ${_v}")
      include(FetchContent)
      FetchContent_Declare(HalideBin URL "${_url}" URL_HASH SHA256=${_sha})
      FetchContent_MakeAvailable(HalideBin)
      list(PREPEND CMAKE_PREFIX_PATH "${halidebin_SOURCE_DIR}")
      find_package(Halide QUIET)
    endif()
  endif()
  if(Halide_FOUND)
    set(_VOXLIDE_HALIDE TRUE)
  else()
    message(STATUS "voxlide: Halide not found (IMAGE3KIT_HALIDE=${IMAGE3KIT_HALIDE}); vxhl:: ops use voxlib")
  endif()
endif()

if(_VOXLIDE_HALIDE)
  if(APPLE)
    set(VOXLIDE_GPU_TARGET "host-metal" CACHE STRING "Halide GPU target for voxlide kernels")
  else()
    set(VOXLIDE_GPU_TARGET "host-vulkan-vk_int8-vk_int16-vk_v13" CACHE STRING "Halide GPU target for voxlide kernels")
  endif()
  message(STATUS "voxlide: Halide ${Halide_VERSION}, kernels for host + ${VOXLIDE_GPU_TARGET}")
  add_halide_runtime(voxlide_runtime TARGETS host ${VOXLIDE_GPU_TARGET})
  target_link_libraries(voxlide PRIVATE voxlide_runtime Halide::Runtime)
  target_compile_definitions(voxlide PRIVATE IMAGE3KIT_HAS_HALIDE=1)
endif()

# ---- voxlide_add_op(<op> GENERATOR <file> [TYPES u8 u16 i32 f32] [IMAGES <input>...] [VALUES <input>...] [STRICT_FLOAT] [FP64] [REDUCE])
# Builds <op>_{cpu,gpu}_<type> kernels and writes <op>_kernels.h with the table
# vxhl::kernels::<op><T>{cpu, gpu}. Types not listed (or no Halide) get nullptr entries,
# so the op falls back to voxlib for them.
#   IMAGES: extra image Inputs of the generator with the op's voxel type (Snapshot / Aux args).
#   REDUCE: a reduction kernel (run_reduce): fixed-type per-z outputs, no voxel-typed `output`.
#   VALUES: scalar Input<Expr>s with the op's voxel type (voxlib arguments of type T).
#   STRICT_FLOAT: float32 in voxlib's exact operation order (no FMA fusion, no reassociation).
#   FP64:   the kernel computes in double to match voxlib bit for bit: strict IEEE float
#           (no FMA fusion) and vk_float64 on Vulkan; Metal has no doubles, so no GPU kernel.
set(VOXLIDE_TYPES u8 u16 i32 f32)
set(_vxhl_hl_u8 uint8)   # Halide type name
set(_vxhl_hl_u16 uint16)
set(_vxhl_hl_i32 int32)
set(_vxhl_hl_f32 float32)
set(_vxhl_c_u8 uint8_t)  # C++ type name
set(_vxhl_c_u16 uint16_t)
set(_vxhl_c_i32 int32_t)
set(_vxhl_c_f32 float)

function(voxlide_add_op OP)
  cmake_parse_arguments(A "FP64;STRICT_FLOAT;REDUCE" "GENERATOR" "TYPES;IMAGES;VALUES" ${ARGN})
  if(NOT A_TYPES)
    set(A_TYPES ${VOXLIDE_TYPES})
  endif()
  set(_incl "")
  set(_spec "")
  if(_VOXLIDE_HALIDE)
    add_halide_generator(${OP}.gen SOURCES ${A_GENERATOR})
    target_include_directories(${OP}.gen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/generators)
    set(_devs cpu gpu)
    set(_cpu_tgt host)
    set(_gpu_tgt ${VOXLIDE_GPU_TARGET})
    if(A_STRICT_FLOAT AND NOT A_FP64)
      set(_cpu_tgt host-strict_float)
      set(_gpu_tgt ${VOXLIDE_GPU_TARGET}-strict_float)
    endif()
    if(A_FP64)
      set(_cpu_tgt host-strict_float)
      if(VOXLIDE_GPU_TARGET MATCHES "vulkan")
        set(_gpu_tgt ${VOXLIDE_GPU_TARGET}-vk_float64-strict_float)
      else()
        set(_devs cpu)
      endif()
    endif()
    foreach(t IN LISTS A_TYPES)
      set(_params input.type=${_vxhl_hl_${t}})
      if(NOT A_REDUCE)
        list(APPEND _params output.type=${_vxhl_hl_${t}})
      endif()
      foreach(img IN LISTS A_IMAGES A_VALUES)
        list(APPEND _params ${img}.type=${_vxhl_hl_${t}})
      endforeach()
      foreach(dev IN LISTS _devs)
        add_halide_library(${OP}_${dev}_${t} FROM ${OP}.gen GENERATOR ${OP}
                           USE_RUNTIME voxlide_runtime TARGETS ${_${dev}_tgt} PARAMS ${_params})
        target_link_libraries(voxlide PRIVATE ${OP}_${dev}_${t})
        string(APPEND _incl "#include \"${OP}_${dev}_${t}.h\"\n")
      endforeach()
      if("gpu" IN_LIST _devs)
        set(_gpu "&${OP}_gpu_${t}")
      else()
        set(_gpu "nullptr")
      endif()
      string(APPEND _spec "template <> struct ${OP}<${_vxhl_c_${t}}> {\n"
                          "    static constexpr auto cpu = &${OP}_cpu_${t};\n"
                          "    static constexpr auto gpu = ${_gpu};\n};\n")
    endforeach()
  endif()
  file(CONFIGURE OUTPUT "${VOXLIDE_GEN_DIR}/${OP}_kernels.h" @ONLY CONTENT
"// Generated by voxlide_add_op(${OP}) in voxlide/CMakeLists.txt; do not edit.
#pragma once
#include <cstddef>
#include <cstdint>
${_incl}
namespace vxhl::kernels {
template <class T> struct ${OP} {
    static constexpr std::nullptr_t cpu = nullptr, gpu = nullptr;
};
${_spec}}  // namespace vxhl::kernels
")
endfunction()

# ---- ported ops: one line each (also add a row to ops_table.h)
voxlide_add_op(mode26 GENERATOR generators/mode26_gen.cpp)
voxlide_add_op(median GENERATOR generators/median_gen.cpp)
voxlide_add_op(smoothBilateral GENERATOR generators/smooth_bilateral_gen.cpp IMAGES orig FP64)
voxlide_add_op(mode6 GENERATOR generators/mode6_gen.cpp)
voxlide_add_op(shrink GENERATOR generators/shrink_gen.cpp IMAGES src VALUES lo hi)
voxlide_add_op(replaceRange GENERATOR generators/replace_range_gen.cpp VALUES lo hi mid)
voxlide_add_op(zeroGrad GENERATOR generators/zero_grad_gen.cpp)
voxlide_add_op(magGradient GENERATOR generators/mag_gradient_gen.cpp TYPES u8 u16 i32 FP64)
voxlide_add_op(scaleGradient GENERATOR generators/scale_gradient_gen.cpp TYPES u8 u16 i32 FP64)
voxlide_add_op(segmentVote GENERATOR generators/segment_vote_gen.cpp TYPES u8 u16 IMAGES orig grad FP64)
voxlide_add_op(median032 GENERATOR generators/median032_gen.cpp VALUES lbl0 lbl1)
voxlide_add_op(median06Grow GENERATOR generators/median06_grow_gen.cpp VALUES lblTo lblFrm)
voxlide_add_op(median06 GENERATOR generators/median06_gen.cpp STRICT_FLOAT)
voxlide_add_op(growLabel GENERATOR generators/grow_label_gen.cpp VALUES vl)
voxlide_add_op(moments GENERATOR generators/moments_gen.cpp FP64 REDUCE)
voxlide_add_op(bilateralX GENERATOR generators/bilateral_x_gen.cpp STRICT_FLOAT)
voxlide_add_op(bilateralGauss GENERATOR generators/bilateral_gauss_gen.cpp FP64)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
