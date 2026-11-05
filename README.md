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

## CPU or GPU

When [image3kit is built](https://github.com/difizix/image3kit-docs/dev/building.md) with [Halide](https://halide-lang.org), many filters also have fast versions, and you get the same results either way. By default (`auto`), each of these filters runs on the GPU if one is usable, otherwise on the fast CPU code (`halide`). Filters without a fast version always run the original C++ code (`voxlib`). To see what your installation has, and to choose:

```python
import image3kit as ik
print(ik.available_backends())   # e.g. ['voxlib', 'halide', 'gpu']
ik.set_backend("halide")         # or "gpu", "voxlib", "auto"
```

A single call on the GPU copies the image there and back, so for scripts made of separate calls `halide` is often faster. Keep the image on the GPU for a chain of filters with `img.to_device()`. Details: [image3kit-docs](https://github.com/difizix/image3kit-docs) (`user/backends.md`, `user/gpu_chains.md`).

## Licenses

See [LICENSE](LICENSE) for the license (BSD 3-clause, subject to change).

This package uses several third-party codes, see [pkgs/](pkgs/) for details.

Initial commit is scaffolded from [pybind/scikit_build_example](https://github.com/pybind/scikit_build_example).
See [pkgs/pybind11/LICENSE](pkgs/pybind11/LICENSE) for the license covering the initial version of .github/workflows.
