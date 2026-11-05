"""voxlib resample_max, resample_mode and rescale_values against numpy references."""
from __future__ import annotations

import numpy as np
import pytest

import image3kit as ik

CLASSES = [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32]


def _img(cls, a):
    img = cls(a.shape, 0)
    np.array(img, copy=False)[...] = a
    return img


def _blocks(a, f):
    n0, n1, n2 = (s // f for s in a.shape)
    b = a[: n0 * f, : n1 * f, : n2 * f].reshape(n0, f, n1, f, n2, f)
    return b.transpose(0, 2, 4, 1, 3, 5).reshape(n0, n1, n2, f**3)


def _data(cls, shape, seed=1):
    rng = np.random.default_rng(seed)
    dt = np.array(cls((1, 1, 1), 0), copy=False).dtype
    a = rng.integers(0, 4, shape).astype(dt)
    if dt.kind == "f":
        a = a - 5.5  # negative floats: the max must not start from numeric_limits<float>::min()
    return a


@pytest.mark.parametrize("cls", CLASSES)
@pytest.mark.parametrize("f", [2, 3])
def test_resample_max_down(cls, f):
    a = _data(cls, (12, 9, 6))
    img = _img(cls, a)
    img.resample_max(f)
    out = np.array(img, copy=False)
    assert np.array_equal(out, _blocks(a, f).max(axis=-1))


@pytest.mark.parametrize("cls", CLASSES)
@pytest.mark.parametrize("fn", ["resample_max", "resample_mode"])
def test_resample_up_and_identity(cls, fn):
    a = _data(cls, (4, 3, 5))
    img = _img(cls, a)
    getattr(img, fn)(0.5)
    out = np.array(img, copy=False)
    assert np.array_equal(out, a.repeat(2, 0).repeat(2, 1).repeat(2, 2))  # every output voxel set
    img = _img(cls, a)
    getattr(img, fn)(1.0)
    assert np.array_equal(np.array(img, copy=False), a)  # was an empty image for resample_max


def _mode(v):
    vals, counts = np.unique(v, return_counts=True)
    return vals[np.argmax(counts)]  # first maximum: the smallest value among ties


@pytest.mark.parametrize("cls", CLASSES)
@pytest.mark.parametrize("f", [2, 3])
def test_resample_mode_down(cls, f):
    a = _data(cls, (12, 9, 6), seed=2)
    a[:f, :f, :f] = 0  # an all-0 block (was max_element of an empty map)
    a[f : 2 * f, :f, :f] = a.flat[0] + 1  # a constant non-0 block
    img = _img(cls, a)
    img.resample_mode(f)
    out = np.array(img, copy=False)
    ref = np.apply_along_axis(_mode, -1, _blocks(a, f))
    assert np.array_equal(out, ref)


@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32])
def test_rescale_values_int(cls):
    a = np.array([[[10, 20], [30, 40]], [[50, 60], [70, 90]]])
    img = _img(cls, a)
    img.rescale_values(0, 160)
    ref = (160 * (a - 10)) // (90 - 10)
    assert np.array_equal(np.array(img, copy=False), ref)


def test_rescale_values_float():
    a = np.array([[[-1.0, -0.5], [0.0, 0.25]], [[-0.75, -0.25], [0.1, 0.2]]], dtype=np.float32)
    img = _img(ik.VxlImgF32, a)
    img.rescale_values(0.0, 1.5)  # range < 1, negative values, non-integer span
    out = np.array(img, copy=False)
    assert out.min() == pytest.approx(0.0) and out.max() == pytest.approx(1.5)
    assert np.allclose(out, 1.5 * (a + 1.0) / 1.25, atol=1e-6)


def test_rescale_values_constant():
    img = _img(ik.VxlImgU8, np.full((3, 3, 3), 7))
    img.rescale_values(5, 9)
    assert np.all(np.array(img, copy=False) == 5)
