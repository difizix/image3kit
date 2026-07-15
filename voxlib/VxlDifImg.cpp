#include "VxlDifImg.h"
#include "voxelImage.h"
#include "voxelImageI.h"
#include "InputFile.h"
#include "svplot.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace VoxLib {

template<typename VxT>
void VxlDifShort(voxelImageT<VxT>& img1, const voxelImageT<VxT>& img2,
                 std::string scaletype, double shift3, double scale3,
                 double shift1, double scale1, double shift2, double scale2) {
    if(scaletype=="log")  {
        std::cout<<"log:  ";
        forAlliii_(img1) {
            img1(iii) = std::min(std::max(0., shift3 + scale3 * (exp(std::max(1e-6, (double(img1(iii)) + shift1)) * scale1) - exp(std::max(1e-6, (double(img2(iii)) + shift2)) * scale2))), dmaxT(VxT));
        }
    }
    else
    {
        std::cout<<"lin:  ";
        forAlliii_(img1)  {
            img1(iii) = std::min(dmaxT(VxT), std::max(0., shift3 + scale3 * ((double(img1(iii)) + shift1) * scale1 - (double(img2(iii)) + shift2) * scale2)));
        }
    }
    img1.printInfo();
}

template<typename VxT>
void VxlMinizVar(voxelImageT<VxT>& img1, const voxelImageT<VxT>& img2,
                 int bgn, int end, double shift, double span) {
    span = std::max(span - shift, 0.1);
    ensure(span > 0);
    std::cout << "VxlMinizVar:  " << "img1" << " - " << "img2" << "  [" << bgn << " " << end << "]" << "  shift " << shift << "  span" << span << " {" << std::endl;

    double var1 = (varianceDbl(img1, bgn, end));
    double var2 = (varianceDbl(img2, bgn, end));

    (std::cout << "stddv: " << covarianceDbl(img1, img2, bgn, end) << " ;    vars: \n").flush();
    double scale = 1., varMin = 1e300;
    img1.printInfo();
    (std::cout << "\n scale1 " << "\t Variance  \t var/(var1+2): \n  ").flush();

    for_i_(0, 101)  {
        double scale1 = shift + span * i / 100.0;
        double scale2 = 1.0 - scale1;
        voxelImageT<int> img3(img1.size3());
        forAlliii_(img1) {
            int vv1 = img1(iii);
            img3(iii) = (bgn <= vv1 && vv1 < end)
                      ? vv1 * scale1 + scale2 * img2(iii)
                      : -1000000000;
        }
        double var = varianceDbl(img3, -1000000000 + 1, 1000000000);
        (std::cout << scale1 << "\t*img1+img2*" << scale2 << "  \t" << var << "  \t" << var / (var1 * scale1 + scale2 * var2) << " \n  ").flush();
        if (varMin > var) { varMin = var; scale = scale1; }
    }
    forAlliii_(img1) {
        img1(iii) = std::min(dmaxT(VxT), std::max(0.0, img1(iii) * scale + (1.0 - scale) * img2(iii)));
    }
    varianceDbl(img1, 1, imaxT(VxT) - 1, true);
    std::cout << " ^^^ Avg value is the grain avg value;  " << std::endl;
    std::cout << " } " << std::endl;
}

void mergePlots(const std::string& outnam, const std::string& key, const std::vector<std::string>& fnames) {
    Table<double,Vars> desc;
    svg::svgraphic fig;
    svg::svplot&   sub = fig.subplot<svg::svplot>(0);

    for (const auto& fnam : fnames)  {
        (std::cout<<" ~"<<fnam<<" \t ").flush();
        Table<double,Vars> tbl=InputFile(fnam).getOr(key,Table<double,Vars>());
        auto bnam=nameOf(fnam);
        sub.plot(tbl.vss_[0], tbl.vss_[1], bnam).line_on(true).shape(svg::no_point);

        sub.x_label(tbl.hdr(0)).y_label(tbl.hdr(1));
        desc.apnd( tbl.vss_[0], tbl.hdr(0) ).apnd( tbl.vss_[1],bnam+"_"+tbl.hdr(1) );
    }
    fig.description("RawData: "+_s(desc));
    fig.write(outnam);
}

template void VxlDifShort<unsigned char>(voxelImageT<unsigned char>&, const voxelImageT<unsigned char>&, std::string, double, double, double, double, double, double);
template void VxlDifShort<unsigned short>(voxelImageT<unsigned short>&, const voxelImageT<unsigned short>&, std::string, double, double, double, double, double, double);

template void VxlMinizVar<unsigned char>(voxelImageT<unsigned char>&, const voxelImageT<unsigned char>&, int, int, double, double);
template void VxlMinizVar<unsigned short>(voxelImageT<unsigned short>&, const voxelImageT<unsigned short>&, int, int, double, double);

} // namespace
