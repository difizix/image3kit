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


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
def test_bilateral_wide_matches_voxlib(backend, cls):
    ref = _labelled(cls, (17, 23, 31))
    got = cls(ref)
    with ik.backend("voxlib"):
        ref.bilateral_wide(iterations=2, kernel_radius=1, x_step=2)
    with ik.backend(backend):
        got.bilateral_wide(iterations=2, kernel_radius=1, x_step=2)
    a, b = np.array(ref, copy=False).astype(np.float64), np.array(got, copy=False).astype(np.float64)
    if backend != "gpu":  # Halide CPU is exact; GPU float32 is not (no strict_float on Vulkan)
        assert np.array_equal(a, b)
    elif cls is ik.VxlImgF32:
        assert np.abs(a - b).max() <= 2.0
    else:
        assert np.abs(a - b).max() <= 1 and np.count_nonzero(a != b) <= 1e-4 * a.size + 1


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
def test_mode6_matches_voxlib(backend, cls):
    ref = _labelled(cls, (17, 23, 31))
    got = cls(ref)
    with ik.backend("voxlib"):
        n_ref = ref.mode6(2)
    with ik.backend(backend):
        n_got = got.mode6(2)
    assert n_ref > 0 and n_got == n_ref
    assert np.array_equal(np.array(ref, copy=False), np.array(got, copy=False))


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
def test_bilateral_gauss_matches_voxlib(backend, cls):
    ref = _labelled(cls, (17, 23, 31))
    got = cls(ref)
    with ik.backend("voxlib"):
        ref.bilateral_gauss(iterations=2, kernel_radius=2, sigma_val=16.0, sharpness=0.1)
    with ik.backend(backend):
        got.bilateral_gauss(iterations=2, kernel_radius=2, sigma_val=16.0, sharpness=0.1)
    a, b = np.array(ref, copy=False), np.array(got, copy=False)
    if backend == "gpu":  # float32 op: Halide's Vulkan backend ignores strict_float
        assert np.abs(a.astype(float) - b.astype(float)).max() <= 2
    else:
        assert np.array_equal(a, b)


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
def test_statistics_match_voxlib(backend, cls):
    img = _labelled(cls, (17, 23, 31))
    calls = [
        lambda m: m.variance(20, 40), lambda m: m.stdev(0, 255), lambda m: m.mean_value(),
        lambda m: m.min_value(), lambda m: m.max_value(), lambda m: m.porosity(25),
    ]
    with ik.backend("voxlib"):
        ref = [f(img) for f in calls]
    with ik.backend(backend):
        got = [f(img) for f in calls]
    assert got == pytest.approx(ref, rel=1e-10)


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
def test_replace_range_matches_voxlib(backend, cls):
    ref = _labelled(cls, (17, 23, 31))
    got = cls(ref)
    with ik.backend("voxlib"):
        ref.replace_range(20, 30, 7)
    with ik.backend(backend):
        got.replace_range(20, 30, 7)
    assert np.array_equal(np.array(ref, copy=False), np.array(got, copy=False))


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls,scale", [(ik.VxlImgU8, 1), (ik.VxlImgU16, 200)])
def test_segment2_matches_voxlib(backend, cls, scale):
    rng = np.random.default_rng(9)
    x, y, z = np.indices((20, 22, 24))  # the numpy view is indexed [x, y, z]
    levels = 50 + 70 * ((x // 7 + (y // 5) * 2 + z // 6) % 3) + rng.normal(0, 14, x.shape)
    ref = cls((20, 22, 24), 0)
    np.array(ref, copy=False)[...] = (np.clip(levels, 0, 255) * scale).astype(np.array(ref, copy=False).dtype)
    got = cls(ref)
    kw = dict(thresholds=[1, 85 * scale, 155 * scale, 254 * scale], min_sizes=[1, 2, 2], noise_val=16.0 * scale,
              n_iterations=6)
    with ik.backend("voxlib"):
        ref.segment2(**kw)
    with ik.backend(backend):
        got.segment2(**kw)
    assert np.array_equal(np.array(ref, copy=False), np.array(got, copy=False))
