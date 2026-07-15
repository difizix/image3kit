from __future__ import annotations

from pathlib import Path

import numpy as np

import image3kit as ik

cwd = Path(__file__).parent


def test_readPng():
    img = ik.VxlImgU8()
    assert np.asarray(img).shape == (0, 0, 0)
    img = ik.VxlImgU8(cwd / "piskelapp.png")
    Path("fig").mkdir(exist_ok=True)
    img.write("fig/piskelapp.tif")
    assert Path("fig/piskelapp.tif").exists()
    img2 = ik.VxlImgU8("fig/piskelapp.tif")
    assert np.asarray(img2).shape == (72, 54, 1)
    assert img2.data.shape == (72, 54, 1)
    assert img2.shape == (72, 54, 1)
    img2.plot_slice(filename="fig/piskelapp2", normal_axis="z")


if __name__ == "__main__":
    # test_version()
    # test_voxlibI()
    test_readPng()
