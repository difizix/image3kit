from __future__ import annotations

try:
    import tomllib
except ImportError:
    import tomli as tomllib
from pathlib import Path

import numpy as np

import image3kit as ik


def test_version():
    with (Path(__file__).parents[1] / "pyproject.toml").open("rb") as f:
        version = tomllib.load(f)["project"]["version"]
    assert ik.__version__ == version.split("-")[0]
    assert version.startswith(ik.__version__)
    assert len(ik.__version__.split(".")) == 3


def test_voxlibI():
    img = ik.VxlImgU8((20, 20, 1), 22)

    assert np.array(img, copy=False)[0, 0, 0] == 22
    assert ik.VxlImgU8((20, 20, 1), 22)[0, 0, 0] == 22


def test_writePng():
    lnt, rad = 128, 16
    img = ik.VxlImgU8((lnt, rad * 2, rad * 2), 1)
    img.paint(ik.cylinder((0, rad, rad), (lnt, rad, rad), (rad * 3) // 4, 0))
    img.printInfo()
    img.plotSlice(filename="cross_x.png", normal_axis="x", slice_index=lnt // 2, min_val=0, max_val=2, color_map="RGB")
    img.plotSlice(filename="cross_y.png", normal_axis="y", slice_index=rad, min_val=0, max_val=2, color_map="RGB")
    img.plotSlice(filename="cross_z.png", normal_axis="z", slice_index=rad, min_val=0, max_val=2, color_map="RGB")
    indices = np.where(img.data == 0)
    print(indices)
    print(lnt * rad * rad * 4)


def test_implicit_conversion():
    from image3kit._core.sirun import int3, dbl3
    
    # Test initialization from list and tuple
    i1 = int3((1, 2, 3))
    i2 = int3([4, 5, 6])
    assert i1.x == 1 and i1.y == 2 and i1.z == 3
    assert i2.x == 4 and i2.y == 5 and i2.z == 6

    d1 = dbl3((1.5, 2.5, 3.5))
    d2 = dbl3([4.5, 5.5, 6.5])
    assert d1.x == 1.5 and d1.y == 2.5 and d1.z == 3.5
    assert d2.x == 4.5 and d2.y == 5.5 and d2.z == 6.5

    # Test implicit conversion in functions like cropD
    img = ik.VxlImgU8((20, 20, 1), 22)
    img.cropD((2, 2, 0), [18, 18, 1])
    assert img.shape == (16, 16, 1)


if __name__ == "__main__":
    test_version()
    test_voxlibI()
    test_writePng()
    test_implicit_conversion()
