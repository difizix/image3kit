#include "VxlImgXX.hpp"

template voxelImageT<unsigned short>::voxelImageT(const std::string&, readOpt, int);

void bind_VxlImgU16(pybind11::module &m, const char* name) {
    VxlPy::bind_VxlImg<unsigned short>(m, name);
}
