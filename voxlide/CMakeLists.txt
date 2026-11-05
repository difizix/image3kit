cmake_minimum_required(VERSION 3.28...4.0)

project(voxlide LANGUAGES CXX)

# voxlide is always built; without Halide every vxhl:: op runs its voxlib reference.
add_library(voxlide STATIC runtime.cpp ops.cpp)
set(VOXLIDE_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_include_directories(voxlide PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${VOXLIDE_GEN_DIR})
# Bit-exact GPU parity with voxlib (generators/vxhl_gen.h VXHL_EXACT): Halide's Vulkan backend ignores
# strict_float and its division/sqrt are not correctly rounded, so the kernels add correction and anti-fusion
# work. OFF: faster GPU kernels whose float results may differ from voxlib in the last bits (parity tests
# then report GPU mismatches without failing). CPU kernels are exact either way.
option(VOXLIDE_EXACT "voxlide GPU kernels reproduce voxlib bit for bit" ON)
if(VOXLIDE_EXACT)
  set(_vxhl_exact 1)
else()
  set(_vxhl_exact 0)
endif()
target_compile_definitions(voxlide PUBLIC VXHL_EXACT=${_vxhl_exact})
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
    # Halide >= 22 publishes binaries only on PyPI: halide-bin is a Python-independent wheel (a zip) with
    # libHalide, its headers and CMake package. URLs and sha256: https://pypi.org/pypi/halide-bin/<version>/json
    set(_v "22.1.0")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
      set(_url "https://files.pythonhosted.org/packages/78/dd/3c0d28d15bbeec7953a6a0ba32578364b044e37f26aebc18e29a9506adc5/halide_bin-22.1.0-py3-none-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl")
      set(_sha fa445243347cf5e8fc4704718f5295f63184f4abd9d920d8a4ef80e2ce94d8d7)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
      set(_url "https://files.pythonhosted.org/packages/b3/fd/8483d5766647d26aa93481bdfe5da4c55b4ad9ba5d10f1e540c0975e95fb/halide_bin-22.1.0-py3-none-manylinux_2_27_aarch64.manylinux_2_28_aarch64.whl")
      set(_sha 3e74032d939ec7467e986d8104a304546e977f148f649f91b8c004953b24f667)
    elseif(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
      set(_url "https://files.pythonhosted.org/packages/68/be/adcf33f456d9f9597a89ec032cdcb189d3dba5905eedfc69057780b991e6/halide_bin-22.1.0-py3-none-macosx_11_0_arm64.whl")
      set(_sha abc21aa916ff97464652a07a93d20e1cfc1d27a28c70b3039e9827b7df55ad06)
    elseif(APPLE)
      set(_url "https://files.pythonhosted.org/packages/82/b7/6aa4f03159771ac7b2c9ba7ef7285860fb4dcdff3c9fb2b51d14fa26d333/halide_bin-22.1.0-py3-none-macosx_11_0_x86_64.whl")
      set(_sha 4d2a2f6be115d26e128eb66aa89d50072fcaad1915916eb06510d8b91c5c512f)
    elseif(WIN32 AND CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64")
      set(_url "https://files.pythonhosted.org/packages/05/68/d09950014faa027b48d49aa4155ef2fb093696e46ccc1351a7a134cabc2a/halide_bin-22.1.0-py3-none-win_amd64.whl")
      set(_sha 246e645193293b1b33ffc8659b2049a97299e5c36e1a2db9384791158105f31d)
    endif()
    if(_url)
      message(STATUS "voxlide: fetching Halide ${_v} (halide-bin wheel)")
      include(FetchContent)
      FetchContent_Declare(HalideBin URL "${_url}" URL_HASH SHA256=${_sha} DOWNLOAD_NAME halide_bin.zip)
      FetchContent_MakeAvailable(HalideBin)
      list(PREPEND CMAKE_PREFIX_PATH "${halidebin_SOURCE_DIR}/halide")
      file(GLOB _hl_cfg "${halidebin_SOURCE_DIR}/halide/lib*/cmake/Halide")  # lib64 isn't searched on every distro
      if(_hl_cfg)
        list(GET _hl_cfg 0 Halide_DIR)
      endif()
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

# ---- voxlide_add_op(<op> GENERATOR <file> [TYPES u8 u16 i32 f32] [IMAGES <input>...] [VALUES <input>...] [STRICT_FLOAT] [FP64] [F32T [FLOATS <input>...]] [REDUCE] [CPU_ONLY])
# Builds <op>_{cpu,gpu}_<type> kernels and writes <op>_kernels.h with the table
# vxhl::kernels::<op><T>{cpu, gpu}. Types not listed (or no Halide) get nullptr entries,
# so the op falls back to voxlib for them.
#   IMAGES: extra image Inputs of the generator with the op's voxel type (Snapshot / Aux args).
#   CPU_ONLY: no GPU kernel (e.g. it needs f64 cos/sin, which Halide's Vulkan backend lacks): the GPU backend throws, Auto uses Halide CPU.
#   OUTPUT_TYPE: the output voxel type when it differs from the input's (run_remap ops, e.g. u8 masks).
#   REDUCE: a reduction kernel (run_reduce): fixed-type per-z outputs, no voxel-typed `output`.
#   VALUES: scalar Input<Expr>s with the op's voxel type (voxlib arguments of type T).
#   STRICT_FLOAT: float32 in voxlib's exact operation order (no FMA fusion, no reassociation).
#   FP64:   the kernel computes in double to match voxlib bit for bit: strict IEEE float
#           (no FMA fusion) and vk_float64 on Vulkan; Metal has no doubles, so no GPU kernel.
#   F32T:   the kernel computes in voxlib's f32_t (IMAGE3KIT_F32_T, common/typses.h): generator param
#           ftype=float32|float64, and FP64 (double) or STRICT_FLOAT (float). FLOATS: scalar Input<Expr>s of type f32_t.
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
  cmake_parse_arguments(A "FP64;STRICT_FLOAT;REDUCE;CPU_ONLY;F32T" "GENERATOR;OUTPUT_TYPE" "TYPES;IMAGES;VALUES;FLOATS" ${ARGN})
  set(_fparams "")
  if(A_F32T)
    if(IMAGE3KIT_F32_T STREQUAL "float")
      set(_ft float32)
      set(A_STRICT_FLOAT TRUE)
    else()
      set(_ft float64)
      set(A_FP64 TRUE)
    endif()
    list(APPEND _fparams ftype=${_ft})
    foreach(f IN LISTS A_FLOATS)
      list(APPEND _fparams ${f}.type=${_ft})
    endforeach()
  endif()
  if(NOT A_TYPES)
    set(A_TYPES ${VOXLIDE_TYPES})
  endif()
  set(_incl "")
  set(_spec "")
  if(_VOXLIDE_HALIDE)
    add_halide_generator(${OP}.gen SOURCES ${A_GENERATOR})
    target_include_directories(${OP}.gen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/generators)
    target_compile_definitions(${OP}.gen PRIVATE VXHL_EXACT=${_vxhl_exact})
    set(_devs cpu gpu)
    set(_cpu_tgt host)
    set(_gpu_tgt ${VOXLIDE_GPU_TARGET})
    if(A_STRICT_FLOAT AND NOT A_FP64)
      set(_cpu_tgt host-strict_float)
      set(_gpu_tgt ${VOXLIDE_GPU_TARGET}-strict_float)
    endif()
    if(A_CPU_ONLY)
      set(_devs cpu)
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
      if(A_OUTPUT_TYPE)
        list(APPEND _params output.type=${_vxhl_hl_${A_OUTPUT_TYPE}})
      elseif(NOT A_REDUCE)
        list(APPEND _params output.type=${_vxhl_hl_${t}})
      endif()
      foreach(img IN LISTS A_IMAGES A_VALUES)
        list(APPEND _params ${img}.type=${_vxhl_hl_${t}})
      endforeach()
      list(APPEND _params ${_fparams})
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
voxlide_add_op(smoothBilateral GENERATOR generators/smooth_bilateral_gen.cpp IMAGES orig F32T FLOATS sigma sharp)
voxlide_add_op(mode6 GENERATOR generators/mode6_gen.cpp)
voxlide_add_op(shrink GENERATOR generators/shrink_gen.cpp IMAGES src VALUES lo hi)
voxlide_add_op(replaceRange GENERATOR generators/replace_range_gen.cpp VALUES lo hi mid)
voxlide_add_op(zeroGrad GENERATOR generators/zero_grad_gen.cpp)
voxlide_add_op(magGradient GENERATOR generators/mag_gradient_gen.cpp TYPES u8 u16 i32 F32T FLOATS diffuse)
voxlide_add_op(scaleGradient GENERATOR generators/scale_gradient_gen.cpp TYPES u8 u16 i32 F32T FLOATS avg)
voxlide_add_op(segmentVote GENERATOR generators/segment_vote_gen.cpp TYPES u8 u16 IMAGES orig grad
               F32T FLOATS localF flatnes gradFactor resolSqr_1 avgGradInv noisv krnlCub)
voxlide_add_op(median032 GENERATOR generators/median032_gen.cpp VALUES lbl0 lbl1)
voxlide_add_op(median06Grow GENERATOR generators/median06_grow_gen.cpp VALUES lblTo lblFrm)
voxlide_add_op(median06 GENERATOR generators/median06_gen.cpp STRICT_FLOAT)
voxlide_add_op(growLabel GENERATOR generators/grow_label_gen.cpp VALUES vl)
voxlide_add_op(shrinkPore GENERATOR generators/shrink_pore_gen.cpp STRICT_FLOAT)
voxlide_add_op(growPore GENERATOR generators/grow_pore_gen.cpp)
voxlide_add_op(medianAxis GENERATOR generators/median_axis_gen.cpp)
voxlide_add_op(pairOp GENERATOR generators/pair_op_gen.cpp IMAGES other)
voxlide_add_op(operateUnary GENERATOR generators/operate_gen.cpp TYPES u8 u16 FP64)
voxlide_add_op(operateImg GENERATOR generators/operate_gen.cpp TYPES u8 u16 IMAGES img2 FP64)
voxlide_add_op(operateScalar GENERATOR generators/operate_gen.cpp TYPES u8 u16 FP64)
voxlide_add_op(threshold101 GENERATOR generators/threshold101_gen.cpp VALUES bgn end)
voxlide_add_op(circleOut GENERATOR generators/circle_out_gen.cpp VALUES outVal)
voxlide_add_op(mapFrom GENERATOR generators/map_from_gen.cpp IMAGES src VALUES vmin vmax FP64)
voxlide_add_op(paintShape GENERATOR generators/paint_shape_gen.cpp FP64)
voxlide_add_op(paintTriangular GENERATOR generators/paint_triangular_gen.cpp FP64 CPU_ONLY)
voxlide_add_op(setWhereEqual GENERATOR generators/set_where_equal_gen.cpp IMAGES other VALUES value)
voxlide_add_op(moments GENERATOR generators/moments_gen.cpp FP64 REDUCE)
voxlide_add_op(histogram GENERATOR generators/histogram_gen.cpp FP64 REDUCE)
voxlide_add_op(remapBox GENERATOR generators/remap_box_gen.cpp)
voxlide_add_op(growRange GENERATOR generators/grow_range_gen.cpp VALUES midv minv maxv)
voxlide_add_op(growOutside GENERATOR generators/grow_outside_gen.cpp VALUES vo vnew)
voxlide_add_op(padEdges GENERATOR generators/pad_edges_gen.cpp)
voxlide_add_op(meanFilter GENERATOR generators/mean_gen.cpp TYPES u8 u16 i32 FP64)
voxlide_add_op(meanWideStep GENERATOR generators/mean_wide_step_gen.cpp TYPES u8 u16 i32 IMAGES grad VALUES lowerB upperB gradthresh)
voxlide_add_op(setWhereAbove GENERATOR generators/set_where_above_gen.cpp TYPES u8 u16 i32 IMAGES other VALUES thresh value)
voxlide_add_op(swapXZ GENERATOR generators/redirect_gen.cpp)
voxlide_add_op(swapXY GENERATOR generators/redirect_gen.cpp)
voxlide_add_op(flipX GENERATOR generators/redirect_gen.cpp)
voxlide_add_op(resampleUp GENERATOR generators/resample_gen.cpp)
voxlide_add_op(resampleMeanDn GENERATOR generators/resample_gen.cpp FP64)
voxlide_add_op(resampleMaxDn GENERATOR generators/resample_gen.cpp)
voxlide_add_op(resampleModeDn GENERATOR generators/resample_gen.cpp)
voxlide_add_op(resliceUp GENERATOR generators/resample_gen.cpp FP64)
voxlide_add_op(resliceDn GENERATOR generators/resample_gen.cpp TYPES u8 u16 i32)
voxlide_add_op(threshold01 GENERATOR generators/threshold01_gen.cpp OUTPUT_TYPE u8 FP64)
# keepLargest: connected components, gather-only (generators/ccl_gen.cpp)
voxlide_add_op(cclInit GENERATOR generators/ccl_gen.cpp OUTPUT_TYPE i32 VALUES lo hi)
voxlide_add_op(cclWeight GENERATOR generators/ccl_gen.cpp OUTPUT_TYPE i32 VALUES lo hi)
voxlide_add_op(cclLabel GENERATOR generators/ccl_gen.cpp TYPES i32)
voxlide_add_op(cclSum GENERATOR generators/ccl_gen.cpp TYPES i32)
voxlide_add_op(cclRoots GENERATOR generators/ccl_gen.cpp TYPES i32)
voxlide_add_op(cclBest GENERATOR generators/ccl_gen.cpp TYPES i32 REDUCE FP64)
voxlide_add_op(cclApply GENERATOR generators/ccl_gen.cpp VALUES lo hi value)
voxlide_add_op(bilateralX GENERATOR generators/bilateral_x_gen.cpp STRICT_FLOAT)
voxlide_add_op(bilateralGauss GENERATOR generators/bilateral_gauss_gen.cpp
               F32T FLOATS sigmaSqr p1rSigma p1rGSigma sharp capmin capmax)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
