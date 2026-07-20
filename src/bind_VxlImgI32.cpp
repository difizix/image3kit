#include "VxlImgXX.hpp"

template voxelImageT<int>::voxelImageT(const std::string&, readOpt, int);

void bind_VxlImgI32(pybind11::module &m, const char* name) {
    VxlPy::bind_VxlImg<int>(m, name);
}
