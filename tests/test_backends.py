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


def _three_labels(cls, shape=(17, 23, 31), seed=4):
    img = cls(shape, 0)
    np.array(img, copy=False)[...] = np.random.default_rng(seed).integers(0, 3, shape) * 10 + 5
    return img


def _binary(cls, shape=(17, 23, 31), seed=5):
    img = cls(shape, 0)
    np.array(img, copy=False)[...] = np.random.default_rng(seed).integers(0, 2, shape)
    return img


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
@pytest.mark.parametrize(
    "call",
    [
        lambda m: m.median032(14, 14, 15, 25),
        lambda m: m.median06_grow(15, 5, 0),  # (the binding's argument names are swapped, see its FIXME)
        lambda m: m.grow_label(15),
    ],
    ids=["median032", "median06_grow", "grow_label"],
)
def test_label_ops_match_voxlib(backend, cls, call):
    ref = _three_labels(cls)
    got = cls(ref)
    with ik.backend("voxlib"):
        call(ref)
    with ik.backend(backend):
        call(got)
    assert np.array_equal(np.array(ref, copy=False), np.array(got, copy=False))


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
def test_median06_matches_voxlib(backend, cls):
    ref = _binary(cls)
    got = cls(ref)
    with ik.backend("voxlib"):
        n_ref = ref.median06(2, 4)
    with ik.backend(backend):
        n_got = got.median06(2, 4)
    assert n_ref > 0 and n_got == n_ref
    assert np.array_equal(np.array(ref, copy=False), np.array(got, copy=False))


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
@pytest.mark.parametrize("op", ["grow0", "shrink0"])
def test_pore_ops_match_voxlib(backend, cls, op):
    ref = _binary(cls)
    got = cls(ref)
    with ik.backend("voxlib"):
        getattr(ref, op)()
    with ik.backend(backend):
        getattr(got, op)()
    assert not np.array_equal(np.array(cls(_binary(cls)), copy=False), np.array(ref, copy=False))  # it did something
    assert np.array_equal(np.array(ref, copy=False), np.array(got, copy=False))


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
@pytest.mark.parametrize("op", ["median_x", "median_y", "median_z"])
def test_median_axis_matches_voxlib(backend, cls, op):
    ref = _three_labels(cls)
    got = cls(ref)
    with ik.backend("voxlib"):
        getattr(ref, op)()
    with ik.backend(backend):
        getattr(got, op)()
    assert np.array_equal(np.array(ref, copy=False), np.array(got, copy=False))


def _small_values(cls, shape=(17, 23, 31), seed=6, top=5):
    img = cls(shape, 0)
    np.array(img, copy=False)[...] = np.random.default_rng(seed).integers(0, top, shape)
    return img


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
@pytest.mark.parametrize("call", ["and_", "or_", "xor_", "not_", "__iadd__", "__isub__"])
def test_two_image_ops_match_voxlib(backend, cls, call):
    other = _small_values(cls, seed=7, top=4)
    ref = _small_values(cls)
    got = cls(ref)
    with ik.backend("voxlib"):
        getattr(ref, call)(other)
    with ik.backend(backend):
        getattr(got, call)(other)
    assert np.array_equal(np.array(ref, copy=False), np.array(got, copy=False))


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
def test_add_sub_operators_match_voxlib(backend, cls):
    a, b = _small_values(cls), _small_values(cls, seed=8)
    with ik.backend("voxlib"):
        ref = (a + b) - b
    with ik.backend(backend):
        got = (a + b) - b
    assert np.array_equal(np.array(ref, copy=False), np.array(got, copy=False))


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
def test_threshold101_circle_out_map_from_match_voxlib(backend, cls):
    src = _small_values(cls, seed=9, top=4)
    results = {}
    for be in ("voxlib", backend):
        img = _small_values(cls)
        with ik.backend(be):
            img.threshold101(1, 3)
            img.circle_out(8, 11, 7, "z", 2)
            img.map_from(src, 0, 1, 2.0, 0.5)
        results[be] = np.array(img, copy=False).copy()
    assert np.array_equal(results["voxlib"], results[backend])


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16])
@pytest.mark.parametrize(
    "call",
    [
        lambda m, o: m.operate("~"),
        lambda m, o: m.operate("!"),
        lambda m, o: m.operate("+", o),
        lambda m, o: m.operate("-", o, 1.5),
        lambda m, o: m.operate("*", o, 0.25),
        lambda m, o: m.operate("/", o),
        lambda m, o: m.operate("&", o),
        lambda m, o: m.operate("%", o),
        lambda m, o: m.operate("+", 7.0),
        lambda m, o: m.operate("*", 2.5),
        lambda m, o: m.operate("b", 2.0),
        lambda m, o: m.operate("%", 3.0),
    ],
    ids=["~", "!", "img+", "img-", "img*", "img/", "img&", "img%", "+7", "*2.5", "b2", "%3"],
)
def test_operate_matches_voxlib(backend, cls, call):
    other = _small_values(cls, seed=10, top=200)
    ref = _small_values(cls, top=250)
    got = cls(ref)
    with ik.backend("voxlib"):
        call(ref, other)
    with ik.backend(backend):
        call(got, other)
    assert np.array_equal(np.array(ref, copy=False), np.array(got, copy=False))


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
@pytest.mark.parametrize("which", ["sphere", "cylinder", "cube", "triangular"])
def test_paint_matches_voxlib(backend, cls, which):
    shapes = {
        "sphere": lambda v: ik.sphere((8, 11, 15), 7.5, v),
        "cylinder": lambda v: ik.cylinder((0, 10, 10), (17, 12, 20), 5, v),
        "cube": lambda v: ik.cube((3, 4, 5), (9, 12, 14), v),
        "triangular": lambda v: ik.triangular((0, 20, 14), L1=8, L2=8, h=14, Lt=12, c_rp_rt=3.2, c_side_mid=1.6, val=v),
    }
    results = {}
    for be in ("voxlib", backend):
        img = _small_values(cls)
        with ik.backend(be):
            if be == "gpu" and which == "triangular":
                pytest.skip("triangular is Halide-CPU only (no f64 cos/sin in Halide's Vulkan backend)")
            img.paint(shapes[which](7))
            img.paint_add(shapes[which](2))
        results[be] = np.array(img, copy=False).copy()
    assert not np.array_equal(results["voxlib"], np.array(_small_values(cls), copy=False))  # it painted something
    assert np.array_equal(results["voxlib"], results[backend])


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
@pytest.mark.parametrize("radius", [1, 2])
def test_fill_holes_matches_voxlib(backend, cls, radius):
    img = cls((17, 23, 31), 0)
    a = np.array(img, copy=False)
    x, y, z = np.indices(a.shape)
    a[...] = ((x // 6 + y // 6 + z // 6) % 2) * 1
    holes = np.random.default_rng(11).random(a.shape) < 0.06
    a[...] = np.where(holes, 1 - a, a)  # flip some voxels: holes and specks
    ref, got = img, cls(img)
    with ik.backend("voxlib"):
        ref.fill_holes(radius)
    with ik.backend(backend):
        got.fill_holes(radius)
    assert np.array_equal(np.array(ref, copy=False), np.array(got, copy=False))


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
def test_delense032_matches_voxlib(backend, cls):
    ref = _three_labels(cls)
    got = cls(ref)
    with ik.backend("voxlib"):
        ref.delense032(2, 14, 14, 5, 15)
    with ik.backend(backend):
        got.delense032(2, 14, 14, 5, 15)
    assert np.array_equal(np.array(ref, copy=False), np.array(got, copy=False))
