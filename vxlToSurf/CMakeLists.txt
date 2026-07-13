cmake_minimum_required(VERSION 3.15...4.0)

project(vxlToSurf LANGUAGES CXX)

add_library(vxlToSurf STATIC
    voxelToSurfMesh.cpp
    voxelSurface.cpp
    voxelSurface_write.cpp
    smoothGauss.cpp
)

target_include_directories(vxlToSurf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(vxlToSurf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../voxlib
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/svplot
    ${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/optim
    ${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/optim/primitives
    ${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/zlib
)

target_compile_definitions(vxlToSurf PRIVATE
    SVG
    LPNG
    TIFLIB
    ZLIB
    OpenMP
    VMMLIB
    IMAGE3KIT_PYTHON_BINDINGS
)

target_link_libraries(vxlToSurf PRIVATE
    voxlib
    ZLIB::ZLIB
    TIFF::tiff
    OpenMP::OpenMP_CXX
)

set_target_properties(vxlToSurf PROPERTIES CXX_STANDARD 23)
