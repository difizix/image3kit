cmake_minimum_required(VERSION 3.15...4.0)

project(voxlib LANGUAGES CXX)

add_library(voxlib STATIC voxelImage.cpp VxlStrips.cpp VxlDifImg.cpp)

target_include_directories(
  voxlib
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
         ${CMAKE_CURRENT_SOURCE_DIR}/../common
         ${CMAKE_CURRENT_SOURCE_DIR}/../src
         ${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/stb
         ${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/svplot
         ${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/optim
         ${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/optim/primitives
         ${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/zlib)

target_compile_definitions(
  voxlib
  PRIVATE SVG
          LPNG
          TIFLIB
          ZLIB
          OpenMP
          VMMLIB
          IMAGE3KIT_PYTHON_BINDINGS)

target_link_libraries(voxlib PRIVATE ZLIB::ZLIB TIFF::tiff OpenMP::OpenMP_CXX)

if(MSVC)
  target_compile_options(voxlib PRIVATE /openmp:llvm)
endif()

set_target_properties(voxlib PROPERTIES CXX_STANDARD 23)
