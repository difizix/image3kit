#include "DeviceImgXX.hpp"

template VoxelImageT<int>::VoxelImageT(const std::string&, readOpt, int);

void bind_VxlImgI32(pybind11::module &m, const char* name) {
    VxlPy::bind_VxlImg<int>(m, name);
    VxlPy::bind_funcs<int>(m);
    VXL_BIND_DEVICE_IMG(int, m, "DevImgI32");
}
