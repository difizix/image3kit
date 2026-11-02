from __future__ import annotations

import numpy as np
import pytest

import image3kit as ik


def _labelled(cls, shape, seed=1):
    img = cls(shape, 0)
    a = np.array(img, copy=False)
    a[...] = np.random.default_rng(seed).integers(1, 5, a.shape) * 10 + 5
    return img


def test_backend_setting():
    assert "voxlib" in ik.available_backends()
    with ik.backend("voxlib"):
        assert ik.get_backend() == "voxlib"
    with ik.backend("cpu"):  # alias
        assert ik.get_backend() == "voxlib"
    with pytest.raises(ValueError):
        ik.set_backend("no-such-backend")


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
def test_mode26_matches_voxlib(backend, cls):
    shape = (17, 23, 31)  # not a cube, not a multiple of the vector width
    ref = _labelled(cls, shape)
    got = cls(ref)
    with ik.backend("voxlib"):
        ref.mode26(2)
    with ik.backend(backend):
        got.mode26(2)
    assert np.array_equal(np.array(ref, copy=False), np.array(got, copy=False))


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
@pytest.mark.parametrize("n_iter", [1, 3])
def test_median_matches_voxlib(backend, cls, n_iter):
    ref = _labelled(cls, (17, 23, 31))
    got = cls(ref)
    with ik.backend("voxlib"):
        ref.median_filter(n_iter)
    with ik.backend(backend):
        got.median_filter(n_iter=n_iter)
    assert np.array_equal(np.array(ref, copy=False), np.array(got, copy=False))


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
def test_smooth_bilateral_matches_voxlib(backend, cls):
    ref = _labelled(cls, (17, 23, 31))
    got = cls(ref)
    with ik.backend("voxlib"):
        ref.smooth_bilateral(iterations=2, kernel_radius=2, sigma_val=16.0, sharpness=0.1)
    with ik.backend(backend):
        got.smooth_bilateral(iterations=2, kernel_radius=2, sigma_val=16.0, sharpness=0.1)
    assert np.array_equal(np.array(ref, copy=False), np.array(got, copy=False))
