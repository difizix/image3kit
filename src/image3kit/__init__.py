from __future__ import annotations

from contextlib import contextmanager

from . import _core
from ._core import __doc__, __version__
from ._core.sirun import Input, dbl3, int3
from ._core.voxlib import (
    VoxelImagesBase,
    VxlImgF32,
    VxlImgI32,
    VxlImgU8,
    VxlImgU16,
    available_backends,
    connected_components,
    cube,
    cylinder,
    get_backend,
    read_image,
    set_backend,
    set_max_slab_bytes,
    shape,
    sphere,
    threshold01_otsu,
    triangular,
)

try:  # voxlide (Halide/GPU) builds only
    from ._core.voxlib import (
        DevImgF32,
        DevImgI32,
        DevImgU8,
        DevImgU16,
        run_slabs,
        slab_of,
    )

    HAS_VOXLIDE = True
except ImportError:
    HAS_VOXLIDE = False


@contextmanager
def backend(name: str):
    """Temporarily set the backend (see ``set_backend``) for the ops run inside the block.

    The setting is process-wide, not per thread.
    """
    prev = get_backend()
    set_backend(name)
    try:
        yield
    finally:
        set_backend(prev)


__all__ = [
    "HAS_VOXLIDE",
    "Input",
    "VoxelImagesBase",
    "VxlImgF32",
    "VxlImgI32",
    "VxlImgU8",
    "VxlImgU16",
    "__doc__",
    "__version__",
    "_core",
    "available_backends",
    "backend",
    "connected_components",
    "cube",
    "cylinder",
    "dbl3",
    "get_backend",
    "int3",
    "read_image",
    "set_backend",
    "set_max_slab_bytes",
    "shape",
    "sphere",
    "threshold01_otsu",
    "triangular",
]
if HAS_VOXLIDE:
    __all__ += ["DevImgF32", "DevImgI32", "DevImgU8", "DevImgU16", "run_slabs", "slab_of"]
