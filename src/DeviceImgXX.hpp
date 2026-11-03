#pragma once
//! Python DeviceImg<T> classes: an image that stays on the GPU between ops (vxhl::DeviceImage, see
//! docs/plans/parallelism_shared.md §2.7). `img.to_device()` copies a VxlImg in (the upload happens with the first
//! GPU op), the methods below mirror the VxlImg methods that have a voxlide port (same names, arguments and
//! defaults) and run on the device, `to_host()` downloads. Only built with voxlide, not for pocketpy.
#include "VxlImgXX.hpp"

#if defined(IMAGE3KIT_HAS_VOXLIDE) && !defined(_POCKETPY)
namespace VxlPy {

template<typename VxT>
void bind_DeviceImg(py::module &mod, const char* name) {
  using SelfT = VoxelImageT<VxT>;
  using DevT = vxhl::DeviceImage<VxT>;
  using Mask = vxhl::DeviceImage<unsigned char>;

  auto cls = py::class_<DevT>(mod, name,
      "Image kept on the GPU between operations: `img.to_device()` creates one, `to_host()` downloads it. "
      "Its methods are the VxlImg methods with a Halide port, with the same arguments; the image is uploaded "
      "once for a chain of them.")
    .def(py::init([](const SelfT &m) { return std::make_unique<DevT>(SelfT(m)); }), arg("image"))
    .def("to_host", [](DevT &d) { return SelfT(d.host()); }, "Download: a VxlImg copy of the current image.")
    .def("copy", [](DevT &d) { return std::make_unique<DevT>(SelfT(d.host())); })
    .def_property_readonly("shape", [](DevT &d) { const int3 n = d.size3(); return std::make_tuple(n.x, n.y, n.z); })
    .def("transfers", [](DevT &d) { const auto t = d.transfers(); return std::make_tuple(t.up, t.down); },
         "(uploads, downloads) of the whole image so far.")
    // ---- in place, same size
    .def("mode26", [](DevT &m, int nMinD) { vxhl::mode26(m, short(nMinD)); }, arg("n_min_diff"))
    .def("mode6", [](DevT &m, short n) { return vxhl::modeNSames(m, n); }, arg("n_same_neighbors"))
    .def("median_filter", [](DevT &m, int nIter) { vxhl::median(m, nIter); }, arg("n_iter") = 1)
    .def("median_x", [](DevT &m) { vxhl::medianx(m); })
    .def("median_y", [](DevT &m) { vxhl::mediany(m); })
    .def("median_z", [](DevT &m) { vxhl::medianz(m); })
    .def("median06", [](DevT &m, int a, int b) { return vxhl::median06(m, a, b); }, arg("nAdj0"), arg("nAdj1"))
    .def("median032", [](DevT &m, int a, int b, VxT l0, VxT l1) { vxhl::median032(m, a, b, l0, l1); },
         arg("nAdj0"), arg("nAdj1"), arg("lbl0"), arg("lbl1"))
    .def("grow_label", [](DevT &m, VxT vl) { vxhl::growLabel(m, vl); })
    .def("grow0", [](DevT &m) { vxhl::growPore(m); })
    .def("shrink0", [](DevT &m) { vxhl::shrinkPore(m); })
    .def("grow_outside_value", [](DevT &m, int vo, int vnew, int n) { vxhl::growOutSideValue(m, vo, vnew, n); },
         arg("val_old") = 0, arg("val_new") = 2, arg("hole_size") = 5)
    .def("fill_holes", [](DevT &m, int r) { vxhl::fillHoles(m, r); }, arg("maxHoleRadius"))
    .def("delense032", [](DevT &m, int n, int a0, int a1, intOr<VxT> l0, intOr<VxT> l1) { vxhl::_delense032(m, n, a0, a1, l0, l1); },
         arg("iterations"), arg("nAdj0"), arg("nAdj1"), arg("lbl0"), arg("lbl1"))
    .def("replace_range", [](DevT &m, VxT lo, VxT hi, VxT v) { vxhl::replaceRange(m, lo, hi, v); }, arg("min"), arg("max"), arg("val"))
    .def("range_to", [](DevT &m, VxT lo, VxT hi, VxT v) { vxhl::replaceRange(m, lo, hi, v); }, arg("min"), arg("max"), arg("val"))
    .def("threshold101", [](DevT &m, VxT lo, VxT hi) { vxhl::threshold101(m, lo, hi); }, arg("min"), arg("max"))
    .def("circle_out", [](DevT &m, int x, int y, int r, std::string d, VxT v) { vxhl::circleOut(m, x, y, r, d[0], v); },
         arg("x"), arg("y"), arg("r"), arg("normal_axis"), arg("val"))
    .def("paint", [](DevT &m, const shape& sh) { vxhl::paint(m, sh); }, arg("shape"))
    .def("paint_add", [](DevT &m, const shape& sh) { vxhl::paintAdd(m, sh); }, arg("shape"))
    .def("and_", [](DevT &m, const DevT &o) { vxhl::and_(m, o); }, arg("image2"))
    .def("or_", [](DevT &m, const DevT &o) { vxhl::or_(m, o); }, arg("image2"))
    .def("xor_", [](DevT &m, const DevT &o) { vxhl::xor_(m, o); }, arg("image2"))
    .def("not_", [](DevT &m, const DevT &o) { vxhl::not_(m, o); }, arg("image2"))
    .def("__iadd__", [](DevT &m, const DevT &o) -> DevT& { vxhl::addEq(m, o); return m; }, py::is_operator())
    .def("__isub__", [](DevT &m, const DevT &o) -> DevT& { vxhl::subEq(m, o); return m; }, py::is_operator())
    .def("map_from", [](DevT &m, const DevT &src, VxT vmin, VxT vmax, double scale, double shift) { vxhl::mapToFrom(m, src, vmin, vmax, scale, shift); },
         arg("source_image"), arg("vmin"), arg("vmax"), arg("scale"), arg("shift"))
    .def("smooth_bilateral", [](DevT &m, int n, int r, double sv, double sh) { vxhl::smoothBilateral(m, n, r, sv, sh); },
         arg("iterations") = 1, arg("kernel_radius") = 1, arg("sigma_val") = 16.0, arg("sharpness") = 0.1)
    .def("bilateral_wide", [](DevT &m, int n, int r, int xs, double sv, double sh, double sd) { vxhl::_bilateralX(m, n, r, xs, sv * sv, sh, sd * sd); },
         arg("iterations") = 1, arg("kernel_radius") = 1, arg("x_step") = 2, arg("sigma_val") = 16.0, arg("sharpness") = 0.1, arg("sigma_spatial") = 2.0)
    .def("bilateral_gauss", [](DevT &m, int n, int r, double sv, double sh, double sd) { vxhl::_bilateralGauss(m, n, r, sv * sv, sh, sd * sd); },
         arg("iterations") = 1, arg("kernel_radius") = 1, arg("sigma_val") = 16.0, arg("sharpness") = 0.1, arg("sigma_spatial") = 2.0)
    .def("segment2", [](DevT &m, std::vector<intOr<VxT>> th, std::vector<int> minSizs, double noisev, double localF, double flatnes,
                        double resolution, double gradFactor, int krnl, int nItrs, int writedumps) {
           if constexpr (sizeof(VxT) <= 2)  // the pipeline runs on DeviceImages inside; the result lands in `m`
             vxhl::segment2(m.host_mut(), int(th.size()) - 1, th, minSizs, noisev, localF, flatnes, resolution, gradFactor, krnl, nItrs, writedumps);
           else throw std::runtime_error("segment2 is for 8 and 16 bit images");
         },
         arg("thresholds") = std::vector<int>(), arg("min_sizes") = std::vector<int>(), arg("noise_val") = 2.,
         arg("local_factor") = 0.05, arg("flatnes") = 0.1, arg("effective_resolution") = 2., arg("gradient_factor") = 0.,
         arg("kernel_radius") = 2, arg("n_iterations") = 13, arg("write_dumps") = 0)
    // ---- another size (still on the device)
    .def("crop", [](DevT &m, int3 bgn, int3 end, int emptyLayers, VxT emptyLayersValue, bool verbose) {
           vxhl::cropD(m, bgn, end, emptyLayers, emptyLayersValue, verbose); },
         arg("begin"), arg("end"), arg("emptyLayers") = 0, arg("emptyLayersValue") = 1, arg("verbose") = false)
    .def("grow_box", [](DevT &m, int n) { vxhl::growBox(m, n); }, arg("num_layers"))
    .def("shrink_box", [](DevT &m, int n) { vxhl::shrinkBox(m, n); }, arg("num_layers"))
    .def("redirect", [](DevT &m, char d) { vxhl::redirect(m, d); })
    .def("direction", [](DevT &m, char d) { vxhl::redirect(m, d); })
    .def("resample", [](DevT &m, double f) { m = vxhl::resampleMean(m, f); }, arg("factor"))
    .def("resample_mean", [](DevT &m, double f) { m = vxhl::resampleMean(m, f); }, arg("factor"))
    .def("reslice_z", [](DevT &m, double f) { m = vxhl::resliceZ(m, f); }, arg("factor"))
    .def("threshold01_otsu", [](DevT &m, int minvi, int maxvi, double skipFrac) { return std::make_unique<Mask>(vxhl::threshold01_otsu(m, minvi, maxvi, skipFrac)); },
         arg("min_val") = 0, arg("max_val") = maxT(VxT), arg("skip_frac") = 0.0,
         "A DeviceImg of u8: 0 where the value is below the Otsu threshold, 1 elsewhere.")
    // ---- results on the host (reductions download only their results)
    .def("variance", [](DevT &m, int lo, int hi) { return vxhl::varianceDbl(m, lo, hi); }, arg("min_val") = 0, arg("max_val") = 255)
    .def("stdev", [](DevT &m, int lo, int hi) { return vxhl::stdevDbl(m, lo, hi); }, arg("min_val") = 0, arg("max_val") = 255)
    .def("mean_value", [](DevT &m) { return vxhl::meanDbl(m); })
    .def("min_value", [](DevT &m) { return vxhl::minDbl(m); })
    .def("max_value", [](DevT &m) { return vxhl::maxDbl(m); })
    .def("porosity", [](DevT &m, VxT t) { return vxhl::porosityDbl(m, t); }, arg("threshold") = VxT(0))
    .def("otsu_threshold", [](DevT &m, int minv, int maxv) {
           auto a = vxhl::otsu_minAvgThresholdAvgMax(m, minv, maxv);
           return std::vector<double>(a.begin(), a.end()); }, arg("min_val") = 0, arg("max_val") = maxT(VxT));

  if constexpr (sizeof(VxT) <= 2)  // operate is ported for 8 and 16 bit images only
    cls
    .def("operate", [](DevT &m, std::string opr) { vxhl::operateUnary(m, opr); }, arg("opr"))
    .def("operate", [](DevT &m, std::string opr, const DevT &img2, double shift) { vxhl::operateImg(m, opr, img2, shift); },
         arg("opr"), arg("image2"), arg("shift") = 0.0)
    .def("operate", [](DevT &m, std::string opr, double val) { vxhl::operateScalar(m, opr, val); }, arg("opr"), arg("val"));
}

}  // namespace VxlPy
#define VXL_BIND_DEVICE_IMG(VxT, mod, name) VxlPy::bind_DeviceImg<VxT>(mod, name)
#else
#define VXL_BIND_DEVICE_IMG(VxT, mod, name) ((void)0)
#endif
