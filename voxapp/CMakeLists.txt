# Command line tools using image3kit, not part of the python package (pyproject).
# Standalone: cmake -S voxapp -B build -DPOCKETPY_DIR=/path/to/pocketpy -DPOCKETPY_LIB=/path/to/libpocketpy.a
cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
project(voxapp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ZLIB REQUIRED)
find_package(TIFF REQUIRED)
find_package(OpenMP REQUIRED)

set(I3K ${CMAKE_CURRENT_LIST_DIR}/..)
set(SRCS_FULL
    ${I3K}/voxlib/bind_pocketpy.cpp
    ${I3K}/voxlib/voxelImage.cpp
    ${I3K}/voxlib/voxelImgUtils.cpp
    ${I3K}/voxlib/VxlDifImg.cpp
    ${I3K}/voxlib/VxlStrips.cpp
    ${I3K}/vxlToFoam/voxelToFoam.cpp
    ${I3K}/vxlToFoam/voxelToFoamPar.cpp
    ${I3K}/vxlToFoam/voxelToFoamPar_seq.cpp
    ${I3K}/vxlToSurf/smoothGauss.cpp
    ${I3K}/vxlToSurf/voxelSurface.cpp
    ${I3K}/vxlToSurf/voxelSurface_write.cpp
    ${I3K}/vxlToSurf/voxelToSurfMesh.cpp)

add_executable(voxelImageProcPy voxelImageProcPy.cpp ${SRCS_FULL})
target_include_directories(voxelImageProcPy PRIVATE
    ${I3K}/common ${I3K}/voxlib ${I3K}/src ${I3K}/pkgs/stb ${I3K}/pkgs/zlib
    ${I3K}/pkgs/svplot ${I3K}/vxlToFoam ${I3K}/vxlToSurf ${POCKETPY_DIR}/include)
# NB! VOX_FULL, not VOX_BASIC
target_compile_definitions(voxelImageProcPy PRIVATE NO_MAIN SVG LPNG TIFLIB ZLIB OpenMP _POCKETPY)
target_link_libraries(voxelImageProcPy PRIVATE ${POCKETPY_LIB} ZLIB::ZLIB TIFF::TIFF OpenMP::OpenMP_CXX)
install(TARGETS voxelImageProcPy DESTINATION bin)
