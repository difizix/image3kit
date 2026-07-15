from __future__ import annotations

from pathlib import Path

import image3kit as ik

cwd = Path(__file__).parent


def test_readPng():
    img = ik.VxlImgU8()
    assert img.shape == (0, 0, 0)
    img = ik.VxlImgU8(cwd / "piskelapp.png")
    assert img.shape == img.data.shape
    assert img.shape[2] == 1
    assert img.nz == 1
    img.plot_all(z_profile=False)  # FIXME: zProfile hangs svplot when nz==1
    img.extrude_dist_map(offset=0.5, scale=2.0)
    assert img.nz > 1
    slice = 8
    img.plot_all(filename="extruded", normal_axis="z", slice_index=slice)
    assert Path(f"fig/extruded_z{slice}_grey.png").exists()


if __name__ == "__main__":
    # test_version()
    # test_voxlibI()
    test_readPng()
