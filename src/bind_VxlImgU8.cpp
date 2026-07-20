#include "VxlImgXX.hpp"

template voxelImageT<unsigned char>::voxelImageT(const std::string&, readOpt, int);

void bind_VxlImgU8(pybind11::module &m, const char* name) {
    VxlPy::bind_VxlImg<unsigned char>(m, name);
}
