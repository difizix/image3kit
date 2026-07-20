#include "VxlImgXX.hpp"

template voxelImageT<float>::voxelImageT(const std::string&, readOpt, int);

void bind_VxlImgF32(pybind11::module &m, const char* name) {
    VxlPy::bind_VxlImg<float>(m, name);
}
