#pragma once
#include <string>
#include <vector>

template <typename T> class voxelImageT;

namespace VoxLib {

template<typename VxT>
void VxlDifShort(voxelImageT<VxT>& img1, const voxelImageT<VxT>& img2,
                 std::string scaletype, double shift3, double scale3,
                 double shift1, double scale1, double shift2, double scale2);

template<typename VxT>
void VxlMinizVar(voxelImageT<VxT>& img1, const voxelImageT<VxT>& img2,
                 int bgn, int end, double shift, double span);

void mergePlots(const std::string& outnam, const std::string& key, const std::vector<std::string>& fnames);

} // namespace VoxLib
