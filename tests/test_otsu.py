from __future__ import annotations

import numpy as np
import pytest

import image3kit as ik


def _two_phase(cls, scale=1, shape=(12, 10, 8), noise=0.0, seed=3):
    img = cls(shape, 0)
    a = np.array(img, copy=False)
    x = np.indices(shape)[0]
    a[...] = np.where(x < shape[0] // 2, 50, 150) * scale
    if noise:
        a[...] += np.random.default_rng(seed).integers(-int(noise), int(noise) + 1, a.shape).astype(a.dtype) * scale
    return img


@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16])
def test_threshold01_otsu_separates_phases(cls):
    img = _two_phase(cls)
    thr = img.otsu_threshold(1, 254)
    assert 50 < thr[2] <= 150  # voxels below the threshold are class 0
    assert thr[1] == pytest.approx(50) and thr[3] == pytest.approx(150)  # class means in voxel values
    mask = np.array(ik.threshold01_otsu(img, 1, 254), copy=False)
    assert np.array_equal(mask, (np.array(img, copy=False) >= thr[2]).astype(mask.dtype))
    assert mask.min() == 0 and mask.max() == 1


@pytest.mark.parametrize("scale", [2, 5, 100, 255])
def test_otsu_of_scaled_image_is_scaled_otsu(scale):
    base = _two_phase(ik.VxlImgU16, 1, noise=12)
    scaled = _two_phase(ik.VxlImgU16, scale, noise=12)
    t1 = base.otsu_threshold(0, 256)
    ts = scaled.otsu_threshold(0, 256 * scale)  # 256 bins: one value each, resp. `scale` values each
    assert ts == pytest.approx([scale * v for v in t1], rel=1e-9)
