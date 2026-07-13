cmake_minimum_required(VERSION 3.15...4.0)

project(vxlToFoam LANGUAGES CXX)

add_library(vxlToFoam STATIC
    voxelToFoam.cpp
    voxelToFoamPar.cpp
    voxelToFoamPar_seq.cpp
)

target_include_directories(vxlToFoam PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(vxlToFoam PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../voxlib
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/svplot
    ${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/optim
    ${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/optim/primitives
    ${CMAKE_CURRENT_SOURCE_DIR}/../pkgs/zlib
)

target_compile_definitions(vxlToFoam PRIVATE
    SVG
    LPNG
    TIFLIB
    ZLIB
    OpenMP
    VMMLIB
    IMAGE3KIT_PYTHON_BINDINGS
)

target_link_libraries(vxlToFoam PRIVATE
    voxlib
    ZLIB::ZLIB
    TIFF::tiff
    OpenMP::OpenMP_CXX
)

set_target_properties(vxlToFoam PROPERTIES CXX_STANDARD 23)

add_executable(Ufraw2Uc Ufraw2Uc.cpp)
target_link_libraries(Ufraw2Uc PRIVATE
    vxlToFoam
    voxlib
    ZLIB::ZLIB
    TIFF::tiff
    OpenMP::OpenMP_CXX
)
set_target_properties(Ufraw2Uc PROPERTIES CXX_STANDARD 23)
