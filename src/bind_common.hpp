#pragma once
#include <pybind11/pybind11.h>
#ifndef _POCKETPY
#include <pybind11/numpy.h>
#include "pybind11/pytypes.h"
#endif
#include <pybind11/stl.h>
#ifdef min
#undef min
#undef max
#endif
#include "typses.h"
#include "InputFile.h"
#include "shapeToVoxel.h"

namespace VxlPy {
namespace py = pybind11;


#ifdef _POCKETPY
// pkbind has no py::sequence / py::make_tuple; VxlImgXX.hpp's only caller already passes
// a py::tuple, which is-a py::sequence for the real-pybind11 build below.
using py_seq = py::tuple;
template<typename T> inline var3<T> tov3(py::tuple v) { return var3<T>(v[0].cast<T>(), v[1].cast<T>(), v[2].cast<T>()); }
template<typename T> inline py::tuple tot3(var3<T> v) { py::tuple t(3); t[0]=v.x; t[1]=v.y; t[2]=v.z; return t; }
inline py::tuple make_tuple3(int x, int y, int z) { py::tuple t(3); t[0]=x; t[1]=y; t[2]=z; return t; }
#else
using py_seq = py::sequence;
template<typename T> inline var3<T> tov3(py::sequence v) { return var3<T>(v[0].cast<T>(), v[1].cast<T>(), v[2].cast<T>()); }
template<typename T> inline py::tuple tot3(var3<T> v) { return py::make_tuple(v.x, v.y, v.z); }
inline py::tuple make_tuple3(int x, int y, int z) { return py::make_tuple(x, y, z); }
#endif


inline InputFile pyCastInput(py::dict dic) {
  InputFile inp;
  for (const auto &kv : dic) {
        inp.add(kv.first.cast<std::string>(), py::str(kv.second).cast<std::string>());
  }
  return inp;
}


//! Shared by image3d_core.cpp's real-pybind11 `image3kit._core` module and
//! voxlib/bind_pocketpy.cpp's pocketpy-embedded `VxlPy` module, so int3/dbl3 behave
//! identically (and get fixes like __repr__) in both.
template<typename T>
inline void bind_var3(py::module_ &mod, const char* name) {
  py::class_<var3<T>>(mod, name)
  .def(py::init<>())
  .def(py::init<T, T, T>())
  .def(py::init([](py_seq s) { return tov3<T>(s); }))
  .def_readwrite("x", &var3<T>::x)
  .def_readwrite("y", &var3<T>::y)
  .def_readwrite("z", &var3<T>::z)
  .def("__getitem__", [](const var3<T> &v, int iVal) {
      int i = iVal;
      if (i < 0) i += 3;
      if (i < 0 || i >= 3) throw py::index_error("index out of range");
      return v[i];
  })
  .def("__setitem__", [](var3<T> &v, int i, T val) { v[i] = val; })
  .def("__repr__", [name](const var3<T> &v) { return std::string(name) + "(" + _s(v.x) + ", " + _s(v.y) + ", " + _s(v.z) + ")"; })
  .def("__eq__", [](const var3<T> &v, py::object other) {
      try {
          py_seq s = other.cast<py_seq>();
          if (s.size() != 3) return false;
          auto o = tov3<T>(s);
          return v.x == o.x && v.y == o.y && v.z == o.z;
      } catch (const std::exception &) {
          return false;
      }
  })
#ifndef _POCKETPY
  .def("__len__", [](const var3<T> &) { return 3; })
#endif
  ;
#ifndef _POCKETPY
  py::implicitly_convertible<py::tuple, var3<T>>();
  py::implicitly_convertible<py::list, var3<T>>();
#endif
}

//! Shared VoxelImagesBase + paintable shape hierarchy bindings, see bind_var3() above
//! for why this is shared between the two pybind11-API consumers.
inline void bind_shapes(py::module_ &mod) {
  py::class_<VoxelImagesBase>(mod, "VoxelImagesBase")
  .def("write", &VoxelImagesBase::write, py::arg("filename"))
  .def("print_info", &VoxelImagesBase::printInfo)
  ;

  py::class_<VoxLib::shape>(mod, "shape");

  py::class_<VoxLib::sphere, VoxLib::shape>(mod, "sphere")
  .def(py::init([](py::tuple tpl, double r, int val) {
      return VoxLib::sphere(tov3<double>(tpl), r, val); }),
      py::arg("center"), py::arg("r"), py::arg("val"))
  ;

  py::class_<VoxLib::cylinder, VoxLib::shape>(mod, "cylinder")
  .def(py::init([](py::tuple p1, py::tuple p2, double r, int val) {
      return VoxLib::cylinder(tov3<double>(p1), tov3<double>(p2), r, val); }),
      py::arg("p1"), py::arg("p2"), py::arg("r"), py::arg("val"),
      "p1: first point on axis, p2: second point on axis, r: radius, val: paint value")
  ;

  py::class_<VoxLib::kube, VoxLib::shape>(mod, "cube")
  .def(py::init([](py::tuple p1, py::tuple size, int val) {
      return VoxLib::kube(tov3<double>(p1), tov3<double>(size), val); }),
      py::arg("p1"), py::arg("size"), py::arg("val"),
      "p1: first point, size: size of cuboid sides, val: paint value")
  ;

  py::class_<VoxLib::triangular, VoxLib::shape>(mod, "triangular")
  .def(py::init([](py::tuple po, double L1, double L2, double h, double Lt, double ch, int val) {
      return VoxLib::triangular(tov3<double>(po), L1, L2, h, Lt, ch, val); }),
      py::arg("po"), py::arg("L1"), py::arg("L2"), py::arg("h"), py::arg("Lt"), py::arg("ch"), py::arg("val"),
      "po: apex point, L1/L2: half-widths, h: height, Lt: throat length, ch: contraction ratio, val: paint value")
  ;
}

} // namespace VxlPy
