# Install C++ headers alongside the Python binding Add more headers to this list
# as needed
install(FILES src/VxlImgXX.hpp src/bind_common.hpp DESTINATION image3kit)

install(
  DIRECTORY pkgs/svplot/
  DESTINATION image3kit/pkgs/svplot
  FILES_MATCHING
  PATTERN "*.hpp")

install(FILES voxlib/voxelImage.h voxlib/voxelImageI.h
        DESTINATION image3kit/voxlib)

install(
  DIRECTORY common/
  DESTINATION image3kit/common
  FILES_MATCHING
  PATTERN "*.h")

# --- CMake Package Configuration ---
include(CMakePackageConfigHelpers)

# Header-only target for external use
add_library(image3kit_headers INTERFACE)
target_include_directories(
  image3kit_headers
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/common>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/voxlib>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
            $<INSTALL_INTERFACE:image3kit/common>
            $<INSTALL_INTERFACE:image3kit/voxlib>
            $<INSTALL_INTERFACE:image3kit>)

# Export the target
target_link_libraries(
  image3kit_headers
  INTERFACE $<BUILD_INTERFACE:ZLIB::ZLIB> $<BUILD_INTERFACE:TIFF::tiff>
            $<BUILD_INTERFACE:OpenMP::OpenMP_CXX>)
install(TARGETS image3kit_headers EXPORT image3kitTargets)
install(
  EXPORT image3kitTargets
  FILE image3kitTargets.cmake
  NAMESPACE image3kit::
  DESTINATION image3kit/cmake)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/image3kitConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/image3kitConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/image3kitConfig.cmake"
  INSTALL_DESTINATION "image3kit/cmake")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/image3kitConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/image3kitConfigVersion.cmake"
        DESTINATION image3kit/cmake)
# ------------------------------------
