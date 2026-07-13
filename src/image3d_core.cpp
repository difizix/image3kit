
#include "bind_common.hpp"
#include "shapeToVoxel.h"
#include "voxelRegions.h"

namespace py = pybind11;

// Forward declarations
void bind_VxlImgU8(pybind11::module &m, const char* name);
void bind_VxlImgU16(pybind11::module &m, const char* name);
void bind_VxlImgI32(pybind11::module &m, const char* name);
void bind_VxlImgF32(pybind11::module &m, const char* name);

PYBIND11_MODULE(_core, mod, py::mod_gil_not_used()) {
    using namespace VxlPy;
    using namespace VoxLib;

    // **************** sirun submodule ***************

    auto sirun = mod.def_submodule("sirun", "The sirun submodule, not to be used directly atm");

    py::class_<var3<int>>(sirun, "int3")
    .def(py::init<>())
    .def(py::init<int, int, int>())
    .def(py::init([](py::sequence s) { return tov3<int>(s); }))
    .def_readwrite("x", &var3<int>::x)
    .def_readwrite("y", &var3<int>::y)
    .def_readwrite("z", &var3<int>::z)
    .def("__getitem__", [](const var3<int> &v, int iVal) {
        int i = iVal;
        if (i < 0) i += 3;
        if (i < 0 || i >= 3) throw py::index_error();
        return v[i];
    })
    .def("__setitem__", [](var3<int> &v, int i, int val) { v[i] = val; })
    .def("__len__", [](const var3<int> &) { return 3; })
    .def("__repr__", [](const var3<int> &v) { return "int3(" + _s(v) + ")"; })
    ;
    py::implicitly_convertible<py::tuple, var3<int>>();
    py::implicitly_convertible<py::list, var3<int>>();

    py::class_<var3<double>>(sirun, "dbl3")
    .def(py::init<>())
    .def(py::init<double, double, double>())
    .def(py::init([](py::sequence s) { return tov3<double>(s); }))
    .def_readwrite("x", &var3<double>::x)
    .def_readwrite("y", &var3<double>::y)
    .def_readwrite("z", &var3<double>::z)
    .def("__getitem__", [](const var3<double> &v, int iVal) {
        int i = iVal;
        if (i < 0) i += 3;
        if (i < 0 || i >= 3) throw py::index_error();
        return v[i];
    })
    .def("__setitem__", [](var3<double> &v, int i, double val) { v[i] = val; })
    .def("__len__", [](const var3<double> &) { return 3; })
    .def("__repr__", [](const var3<double> &v) { return "dbl3(" + _s(v) + ")"; })
    ;
    py::implicitly_convertible<py::tuple, var3<double>>();
    py::implicitly_convertible<py::list, var3<double>>();


    py::class_<InputFile>(sirun, "Input")
    .def(py::init([](py::dict dic) { return pyCastInput(dic); }))
    .def("add", [](InputFile &inp, std::string key, std::string val) { inp.add(key, val); })
    .def("set", [](InputFile &inp, std::string key, std::string val) { inp.set(key, val); })
    .def("get", [](InputFile &inp, std::string key) { return inp.kwrd(key); })
    .def("setDefault", &InputFile::setDefault)
    .def("echoKeywords", [](InputFile &inp) { inp.echoKeywords(); })
    .def("renameKeys", &InputFile::renameKeys)
    ;


    // **************** sirun submodule ***************

    auto voxlib = mod.def_submodule("voxlib", "Auto-generated wrapper for voxelImageT template C++ classes.");

    auto _voxelImageTBase = py::class_<voxelImageTBase>(voxlib, "voxelImageTBase")
    ;

    auto _shape = py::class_<shape>(voxlib, "shape");

    py::class_<sphere,shape>(voxlib, "sphere")
    .def(py::init([](py::tuple tpl, double r, int val) {
        return sphere(tov3<double>(tpl), r, val); }))
    ;

    py::class_<cylinder,shape>(voxlib, "cylinder")
    .def(py::init([](py::tuple p1, py::tuple p2, double r, int val) {
        return cylinder(tov3<double>(p1), tov3<double>(p2), r, val); }),
        py::arg("p1"), py::arg("p2"), py::arg("r"), py::arg("val"),
        "p1: first point on axis, p2: second point on axis, r: radius, val: paint value")
    ;

    py::class_<kube,shape>(voxlib, "cube")
    .def(py::init([](py::tuple p1, py::tuple size, int val) {
        std::cout << " p1: " << tov3<double>(p1)  << " size: " << tov3<double>(size)  << " val: " << val << " ";
        return kube(tov3<double>(p1), tov3<double>(size), val); }),
        py::arg("p1"), py::arg("size"), py::arg("val"),
        "p1: first point, size: size of cuboid sides, val: paint value")
    ;

    // TODO switch to int32_t...
    bind_VxlImgU8(voxlib, "VxlImgU8");
    bind_VxlImgU16(voxlib, "VxlImgU16");
    bind_VxlImgI32(voxlib, "VxlImgI32");
    bind_VxlImgF32(voxlib, "VxlImgF32");

    voxlib.def("labelImage", [](voxelImage &m, double minvv, double maxvv) {
        auto lbls = labelImage(m, (unsigned char)(minvv), (unsigned char)(maxvv));
        compressLabelImage(lbls);
        return lbls;
    });

    voxlib.def("readImageBase",
        [](py::object filename) {
            return readImage(py::str(filename).cast<std::string>(), 0);
        }, py::arg("filename"),
        "Global helper to read an image from a file, use VxlImg..() constructors instead.");


    // Bind docstrings or versions to the main module or submodules as needed
    mod.doc() = "Auto-generated _core (PyBind11) of image3kit package containing sirun and voxlib submodules.";

#ifdef VERSION_INFO
    mod.attr("__version__") = TOSTRING(VERSION_INFO);
#else
    mod.attr("__version__") = "dev";
#endif
}
