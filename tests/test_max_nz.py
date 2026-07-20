from __future__ import annotations

from pathlib import Path

import numpy as np

import image3kit as ik

cwd = Path(__file__).parent
fig_dir = cwd / "../fig"
fig_dir.mkdir(exist_ok=True)


def test_max_nz_restricted_loading():
    # 1. Create a dummy 3D image of size (10, 10, 10)
    arr = np.zeros((10, 10, 10), dtype=np.uint8)
    arr[:, :, :] = 42

    # 2. Write it as TIFF and Amira (.am) and MHD (.mhd)
    # We can create VxlImgU8 from array or by initializing shape
    img = ik.VxlImgU8((10, 10, 10), 42)
    assert img.shape == (10, 10, 10)

    tif_path = fig_dir / "test_nz.tif"
    am_path = fig_dir / "test_nz.am"
    mhd_path = fig_dir / "test_nz.mhd"
    raw_gz_path = fig_dir / "test_nz_reset_Nd0_10_10_10_voxel_1.0_size.raw.gz"

    img.write(str(tif_path))
    img.write(str(am_path))
    img.write(str(mhd_path))
    img.write(str(raw_gz_path))

    # 3. Load with default max_nz (no restriction)
    img_tif = ik.VxlImgU8(tif_path)
    assert img_tif.shape == (10, 10, 10)

    img_am = ik.VxlImgU8(am_path)
    assert img_am.shape == (10, 10, 10)

    img_mhd = ik.VxlImgU8(mhd_path)
    assert img_mhd.shape == (10, 10, 10)

    # Note: loading raw directly (via filename parse)
    img_raw = ik.VxlImgU8(raw_gz_path)
    assert img_raw.shape == (10, 10, 10)

    # 4. Load with max_nz restriction (e.g., max_nz=5)
    img_tif_cap = ik.VxlImgU8(tif_path, max_nz=5)
    assert img_cap_z_nz(img_tif_cap) == 5

    img_am_cap = ik.VxlImgU8(am_path, max_nz=5)
    assert img_cap_z_nz(img_am_cap) == 5

    img_mhd_cap = ik.VxlImgU8(mhd_path, max_nz=5)
    assert img_cap_z_nz(img_mhd_cap) == 5

    img_raw_cap = ik.VxlImgU8(raw_gz_path, max_nz=5)
    assert img_cap_z_nz(img_raw_cap) == 5

    # Verify that different instances can load with different restrictions in the same session
    img_am_cap_3 = ik.VxlImgU8(am_path, max_nz=3)
    assert img_cap_z_nz(img_am_cap_3) == 3
    assert img_cap_z_nz(img_am_cap) == 5  # verify previous is unchanged/not global


def img_cap_z_nz(img) -> int:
    # returns nz
    return img.nz


if __name__ == "__main__":
    test_max_nz_restricted_loading()
