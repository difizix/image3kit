#include "DeviceImgXX.hpp"

template VoxelImageT<unsigned char>::VoxelImageT(const std::string&, readOpt, int);

void bind_VxlImgU8(pybind11::module &m, const char* name) {
    VxlPy::bind_VxlImg<unsigned char>(m, name);
    VxlPy::bind_funcs<unsigned char>(m);
    VXL_BIND_DEVICE_IMG(unsigned char, m, "DevImgU8");
}
