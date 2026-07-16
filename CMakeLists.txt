# Require CMake 3.15+ (matching scikit-build-core) Use new versions of all
# policies up to CMake 4.0
cmake_minimum_required(VERSION 3.15...4.0)

# Disable C++20 module dependency scanning (requires clang-scan-deps)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Scikit-build-core sets these values, provide fallbacks for standalone builds
if(NOT DEFINED SKBUILD_PROJECT_NAME)
  set(SKBUILD_PROJECT_NAME "image3kit")
endif()
if(NOT DEFINED SKBUILD_PROJECT_VERSION)
  set(SKBUILD_PROJECT_VERSION "0.0.0")
endif()

project(
  ${SKBUILD_PROJECT_NAME}
  VERSION ${SKBUILD_PROJECT_VERSION}
  LANGUAGES CXX)

# Find Python first (required for pybind11 fallback, for IDEs)
find_package(
  Python
  COMPONENTS Interpreter Development.Module
  REQUIRED)

if(MSVC)
  add_compile_definitions(NOMINMAX)
endif()

include(FetchContent)

# Find pybind11 and Python
find_package(pybind11 CONFIG QUIET)
if(NOT pybind11_FOUND)
  FetchContent_Declare(
    pybind11
    URL https://github.com/pybind/pybind11/archive/refs/tags/v3.0.1.tar.gz)
  FetchContent_MakeAvailable(pybind11)
endif()

find_package(ZLIB)
if(NOT ZLIB_FOUND)
  message(STATUS "ZLIB not found, fetching from source...")
  FetchContent_Declare(
    ZLIB
    URL https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz
        EXCLUDE_FROM_ALL)
  FetchContent_MakeAvailable(ZLIB)
  if(NOT TARGET ZLIB::ZLIB)
    if(TARGET zlibstatic)
      add_library(ZLIB::ZLIB ALIAS zlibstatic)
    elseif(TARGET zlib)
      add_library(ZLIB::ZLIB ALIAS zlib)
    endif()
  endif()
endif()

if(APPLE)
  if(NOT DEFINED OpenMP_ROOT AND DEFINED ENV{OpenMP_ROOT})
    set(OpenMP_ROOT $ENV{OpenMP_ROOT})
  endif()

  find_package(OpenMP)

  if(NOT OpenMP_FOUND)
    # Hint for Homebrew libomp
    set(OpenMP_CXX_FLAGS -Xpreprocessor -fopenmp)
    set(OpenMP_CXX_LIB_NAMES "omp")
    if(EXISTS "${OpenMP_ROOT}/lib/libomp.dylib")
      set(OpenMP_omp_LIBRARY "${OpenMP_ROOT}/lib/libomp.dylib")
    elseif(EXISTS "/opt/homebrew/opt/libomp/lib/libomp.dylib")
      set(OpenMP_omp_LIBRARY "/opt/homebrew/opt/libomp/lib/libomp.dylib")
    elseif(EXISTS "/usr/local/opt/libomp/lib/libomp.dylib")
      set(OpenMP_omp_LIBRARY "/usr/local/opt/libomp/lib/libomp.dylib")
    endif()

    if(DEFINED OpenMP_omp_LIBRARY)
      message(STATUS "Manual config for OpenMP: ${OpenMP_omp_LIBRARY}")
      set(OpenMP_FOUND TRUE)
      add_library(OpenMP::OpenMP_CXX UNKNOWN IMPORTED)
      set_target_properties(
        OpenMP::OpenMP_CXX
        PROPERTIES IMPORTED_LOCATION "${OpenMP_omp_LIBRARY}"
                   INTERFACE_COMPILE_OPTIONS "${OpenMP_CXX_FLAGS}"
                   INTERFACE_LINK_LIBRARIES "${OpenMP_omp_LIBRARY}")
    endif()
  endif()
endif()

if(NOT OpenMP_FOUND)
  find_package(OpenMP REQUIRED)
endif()
FetchContent_Declare(
  TIFF
  URL https://gitlab.com/libtiff/libtiff/-/archive/v4.6.0/libtiff-v4.6.0.tar.gz
      EXCLUDE_FROM_ALL)
set(tiff-tools
    OFF
    CACHE BOOL "Build libtiff tools")
set(tiff-tests
    OFF
    CACHE BOOL "Build libtiff tests")
set(tiff-contrib
    OFF
    CACHE BOOL "Build libtiff contrib")
set(tiff-docs
    OFF
    CACHE BOOL "Build libtiff docs")
set(jpeg
    OFF
    CACHE BOOL "use libjpeg" FORCE)
set(zstd
    OFF
    CACHE BOOL "use libzstd" FORCE)
set(webp
    OFF
    CACHE BOOL "use libwebp" FORCE)
set(jbig
    OFF
    CACHE BOOL "use jbig" FORCE)
set(lzma
    OFF
    CACHE BOOL "use lzma" FORCE)
set(libdeflate
    OFF
    CACHE BOOL "use libdeflate" FORCE)
set(BUILD_SHARED_LIBS
    OFF
    CACHE BOOL "Build shared libraries" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WARN_DEPRECATED
    OFF
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(TIFF)
set(CMAKE_WARN_DEPRECATED
    ON
    CACHE BOOL "" FORCE)

# Add targets for local includes to ensure they appear in compile_commands.json
if(MSVC)
  add_compile_options(/openmp:llvm /diagnostics:caret /bigobj)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_subdirectory(common)
add_subdirectory(voxlib)
add_subdirectory(vxlToFoam)
add_subdirectory(vxlToSurf)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add a library using FindPython's tooling (pybind11 also provides a helper like
# this)
file(GLOB PyBind_SRCS CONFIGURE_DEPENDS "src/*.cpp")

python_add_library(_core MODULE ${PyBind_SRCS} WITH_SOABI)

target_compile_definitions(
  _core
  PRIVATE SVG
          LPNG
          TIFLIB
          ZLIB
          OpenMP
          VMMLIB
          IMAGE3KIT_PYTHON_BINDINGS
          DUMMY=1)

target_link_libraries(_core PRIVATE pybind11::headers)

target_include_directories(_core PRIVATE common)
target_include_directories(_core PRIVATE voxlib)
target_include_directories(_core PRIVATE vxlToFoam)
target_include_directories(_core PRIVATE vxlToSurf)
target_include_directories(_core PRIVATE src)
target_include_directories(_core PRIVATE pkgs/optim)
target_include_directories(_core PRIVATE pkgs/optim/primitives)
target_include_directories(_core PRIVATE pkgs/stb)
target_include_directories(_core PRIVATE pkgs/zlib)
target_include_directories(_core PRIVATE pkgs/svplot)

target_compile_definitions(_core PRIVATE VERSION_INFO=${PROJECT_VERSION})

set_property(TARGET _core PROPERTY CXX_STANDARD 23)

target_link_libraries(
  _core
  PRIVATE voxlib
          vxlToFoam
          vxlToSurf
          Python::Module
          ZLIB::ZLIB
          TIFF::tiff
          OpenMP::OpenMP_CXX)

# The install directory is the output (wheel) directory
install(TARGETS _core DESTINATION image3kit)
install(
  DIRECTORY src/image3kit/
  DESTINATION image3kit
  FILES_MATCHING
  PATTERN "*.py"
  PATTERN "*.pyi")

# Handle installation of headers and CMake package configuration
include(cmake/PackageConfig.cmake)

# Add targets for local includes to ensure they appear in compile_commands.json
