# image3kit

**EXPERIMENTAL: not ready for public use**


The `image3kit` is Python packaging of a collection of C++ libraries for processing
and analysing 3D images such as those obtained using X-ray micro-tomography.

## Installation

To install the version released on PyPI, run:

```bash
pip install image3kit
```

The `main` branch (experimental) can be installed with the command:
```bash
pip install git+https://github.com/DigiPorFlow/image3kit.git
```

## CPU or GPU (optional)

The `voxlide/main` branch adds faster versions of many filters, running on the GPU or on fast CPU code
([Halide](https://halide-lang.org)), with the same results. To install it:

```bash
pip install -C cmake.define.IMAGE3KIT_HALIDE=ON git+https://github.com/DigiPorFlow/image3kit.git@voxlide/main
```

This downloads Halide during the build (Linux, macOS, Windows x64). To see what your installation has, and to choose:

```python
import image3kit as ik
print(ik.available_backends())   # e.g. ['voxlib', 'halide', 'gpu']; ['voxlib'] only on main
ik.set_backend("halide")         # or "gpu", "voxlib", "auto" (default: GPU if usable, else halide)
```

A single call on the GPU copies the image there and back, so for scripts made of separate calls `halide` is often
faster. To keep the image on the GPU for a chain of filters, use `img.to_device()`. Details:
[image3kit-docs](https://github.com/difizix/image3kit-docs) (`user/backends.md`, `user/gpu_chains.md`).

## Licenses

See [LICENSE](LICENSE) for the license (BSD 3-clause, subject to change).

This package uses several third-party codes, see [pkgs/](pkgs/) for details.

Initial commit is scaffolded from [pybind/scikit_build_example](https://github.com/pybind/scikit_build_example).
See [pkgs/pybind11/LICENSE](pkgs/pybind11/LICENSE) for the license covering the initial version of .github/workflows.
