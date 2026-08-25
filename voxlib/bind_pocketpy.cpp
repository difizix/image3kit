/*-------------------------------------------------------------------------*\

Embeds pocketpy and exposes image3kit's real VxlImg bindings (VxlImgXX.hpp) to it, so
`.mhd` headers can embed a `VxlPy { <script> }` block that runs against the image being
loaded. Replaces the old voxelplugins.cpp/vxlPro*.cpp keyword-command DSL.

\*-------------------------------------------------------------------------*/

#include "VxlImgXX.hpp"
#include "shapeToVoxel.h"
#include <sstream>
#include <vector>
#include <algorithm>

using namespace VxlPy;
using namespace VoxLib;
namespace py = pybind11;

PYBIND11_EMBEDDED_MODULE(VxlPy, mod) {
    VxlPy::bind_var3<int>(mod, "int3");
    VxlPy::bind_var3<double>(mod, "dbl3");
    VxlPy::bind_shapes(mod);

    VxlPy::bind_VxlImg<unsigned char>(mod, "VxlImgU8");    VxlPy::bind_funcs<unsigned char>(mod);
    VxlPy::bind_VxlImg<int>(mod, "VxlImgI32");             VxlPy::bind_funcs<int>(mod);
    VxlPy::bind_VxlImg<float>(mod, "VxlImgF32");           VxlPy::bind_funcs<float>(mod);
    VxlPy::bind_VxlImg<unsigned short>(mod, "VxlImgU16");  VxlPy::bind_funcs<unsigned short>(mod);
}

namespace VxlPy {

//! lazily initialize a persistent pocketpy interpreter (avoid repeated init/finalize
//! cost across many image loads within one process). Not thread-safe: header-script
//! processing happens per-process in observed usage (MPI-parallel, not OpenMP-parallel).
static void ensureInterpreter() {
    static py::scoped_interpreter guard{};
}

//! InputFile's brace-capture keeps a "VxlPy { ... }" block's text verbatim, including
//! whatever leading whitespace the header author used for readability - Python (and
//! pocketpy) reject a top-level script whose first line is indented with no enclosing
//! block, so strip the common leading-whitespace prefix shared by all non-blank lines
//! (same idea as Python's textwrap.dedent) before executing.
std::string dedent(const std::string& script) {
    std::istringstream in(script);
    std::vector<std::string> lines;
    std::string line;
    size_t commonIndent = std::string::npos;
    while (std::getline(in, line)) {
        lines.push_back(line);
        size_t firstNonSpace = line.find_first_not_of(" \t");
        if (firstNonSpace != std::string::npos)
            commonIndent = std::min(commonIndent, firstNonSpace);
    }
    if (commonIndent == std::string::npos || commonIndent == 0) return script;
    std::string result;
    for (const auto& l : lines) {
        result += (l.size() > commonIndent ? l.substr(commonIndent) : "");
        result += "\n";
    }
    return result;
}

template<typename T>
static bool execHeaderScriptT(const std::string& script, VoxelImagesBase* imgPtr) {
    if (auto img = dynamic_cast<VoxelImageT<T>*>(imgPtr)) {
        ensureInterpreter();
        py::dict locals;
        locals["img"] = py::cast(img, py::return_value_policy::reference);
        // bring VxlImgU8/sphere/cylinder/cube/int3/dbl3/etc. into scope unqualified,
        // so header scripts can write `img.paint(sphere(...))` without an import line.
        py::exec("from VxlPy import *", py::globals(), locals);
        py::exec(script, py::globals(), locals);
        return true;
    }
    return false;
}

//! Replaces the old vxlProcess<InpT, SupportedVoxTyps>(...) dispatch: runs `script`
//! (already-extracted text, e.g. from a "VxlPy { ... }" block) as a pocketpy script
//! against the given image, bound as `img`.
void execScript(const std::string& rawScript, VoxelImagesBase* imgPtr, const std::string&) {
    if (rawScript.empty()) return;
    std::string script = dedent(rawScript);
    bool ran = execHeaderScriptT<unsigned char>(script, imgPtr)
            || execHeaderScriptT<unsigned short>(script, imgPtr)
            || execHeaderScriptT<int>(script, imgPtr)
            || execHeaderScriptT<float>(script, imgPtr);
    if (!ran) std::cout << "Unknown image type." << std::endl;
}

} // namespace VxlPy
