
#include "bind_common.hpp"
#include "shapeToVoxel.h"
#include "voxelRegions.h"
#include <pybind11/iostream.h>
#include "voxlide.h"

namespace py = pybind11;

// Forward declarations
void bind_VxlImgU8(pybind11::module &m, const char* name);
void bind_VxlImgU16(pybind11::module &m, const char* name);
void bind_VxlImgI32(pybind11::module &m, const char* name);
void bind_VxlImgF32(pybind11::module &m, const char* name);

PYBIND11_MODULE(_core, mod, py::mod_gil_not_used()) {
    using namespace VxlPy;
    using namespace VoxLib;

    py::add_ostream_redirect(mod, "ostream_redirect");

    // **************** sirun submodule ***************

    auto sirun = mod.def_submodule("sirun", "The sirun submodule, not to be used directly atm");

    VxlPy::bind_var3<int>(sirun, "int3");
    VxlPy::bind_var3<double>(sirun, "dbl3");


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

    auto voxlib = mod.def_submodule("voxlib", "Auto-generated wrapper for VoxelImageT template C++ classes.");

    VxlPy::bind_shapes(voxlib);

    // TODO switch to int32_t...
    // order matters for stubgen
    bind_VxlImgU8(voxlib, "VxlImgU8");
    bind_VxlImgI32(voxlib, "VxlImgI32");
    bind_VxlImgF32(voxlib, "VxlImgF32");
    bind_VxlImgU16(voxlib, "VxlImgU16");

    // Backend for ops with a voxlide (Halide) port; see docs/plans/parallelism_shared.md §1.
    voxlib.def("set_backend", [](const std::string& name) { vxhl::set_backend(vxhl::parse_backend(name)); },
        py::arg("name"),
        "Set the backend for ops with a Halide port: 'auto', 'voxlib' (alias 'cpu'), 'halide' or 'gpu'.\n"
        "Default: $IMAGE3KIT_BACKEND, else 'auto' (GPU for large images, else Halide CPU, else voxlib).\n"
        "Results are identical on every backend.");
    voxlib.def("get_backend", [] { return std::string(vxhl::backend_name(vxhl::get_backend())); },
        "Name of the current backend.");
    voxlib.def("available_backends", [] {
            std::vector<std::string> v;
            for (auto b : vxhl::available_backends()) v.push_back(vxhl::backend_name(b));
            return v;
        }, "Backends usable on this machine, e.g. ['voxlib', 'halide', 'gpu'].");
    voxlib.def("set_max_slab_bytes", [](std::size_t bytes) { vxhl::set_max_slab_bytes(bytes); }, py::arg("bytes"),
        "Max bytes of image (plus halo) processed in one piece; larger images are processed in z-slabs\n"
        "(run_slabs: a whole chain per slab). 0 restores the default: 512 MiB on the GPU, 1 GiB on CPU.");

    // TODO: What does it return ? casted image or a base-class, we shall create an adaptor that does the cast automatically
    voxlib.def("read_image",
        [](py::object filename, int max_nz) {
            return readImage(py::str(filename).cast<std::string>(), 0, max_nz);
        }, py::arg("filename"), py::arg("max_nz") = -1,
        "Global helper to read an image from a file, use VxlImg..() constructors if you know image type.");


    // Bind docstrings or versions to the main module or submodules as needed
    mod.doc() = "Auto-generated _core (PyBind11) of image3kit package containing sirun and voxlib submodules.";

#ifdef VERSION_INFO
    mod.attr("__version__") = TOSTRING(VERSION_INFO);
#else
    mod.attr("__version__") = "dev";
#endif
}
