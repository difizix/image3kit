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


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
@pytest.mark.parametrize(
    "call",
    [
        lambda m: m.grow_box(3),
        lambda m: m.shrink_box(2),
        lambda m: m.crop((1, 2, 3), (15, 20, 30)),
        lambda m: m.crop((1, 2, 3), (15, 20, 30), 2, 7),
        lambda m: m.redirect("z"),
        lambda m: m.direction("y"),
        lambda m: m.redirect("-"),
        lambda m: m.resample(0.5),
        lambda m: m.resample_mean(2.0),
        lambda m: m.reslice_z(0.4),
        lambda m: m.reslice_z(2.0),
    ],
    ids=["grow_box", "shrink_box", "crop", "crop_empty_layers", "redirect_z", "direction_y", "flip",
         "resample_up", "resample_mean", "reslice_up", "reslice_down"],
)
def test_box_ops_match_voxlib(backend, cls, call):
    ref = _labelled(cls, (17, 23, 31))
    got = cls(ref)
    with ik.backend("voxlib"):
        call(ref)
    with ik.backend(backend):
        call(got)
    assert np.array_equal(np.array(ref, copy=False), np.array(got, copy=False))


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16])
@pytest.mark.parametrize("lo,hi,skip", [(0, 255, 0.0), (0, 1000, 0.0), (0, 255, 0.2)])
def test_otsu_matches_voxlib(backend, cls, lo, hi, skip):
    img = cls((17, 23, 31), 0)
    a = np.array(img, copy=False)
    rng = np.random.default_rng(6)
    a[...] = np.where(rng.random(a.shape) < 0.4, rng.integers(20, 90, a.shape), rng.integers(110, 200, a.shape))
    with ik.backend("voxlib"):
        ref_t = img.otsu_threshold(lo, hi)
        ref_m = np.array(ik.threshold01_otsu(img, lo, hi, skip), copy=False).copy()
    with ik.backend(backend):
        got_t = img.otsu_threshold(lo, hi)
        got_m = np.array(ik.threshold01_otsu(img, lo, hi, skip), copy=False)
    assert ref_t == got_t
    assert np.array_equal(ref_m, got_m)


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
@pytest.mark.parametrize("hole_size", [0, 3])
def test_grow_outside_value_matches_voxlib(backend, cls, hole_size):
    img = cls((21, 22, 23), 0)
    a = np.array(img, copy=False)
    rng = np.random.default_rng(8)
    a[4:17, 3:18, 5:19] = rng.choice([0, 7, 9], size=(13, 15, 14), p=[0.25, 0.4, 0.35])
    a[rng.random(a.shape) < 0.05] = 7
    ref, got = cls(img), cls(img)
    with ik.backend("voxlib"):
        ref.grow_outside_value(0, 2, hole_size)
    with ik.backend(backend):
        got.grow_outside_value(0, 2, hole_size)
    assert np.array_equal(np.array(ref, copy=False), np.array(got, copy=False))


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32])
def test_mean_wide_matches_voxlib(backend, cls, tmp_path, monkeypatch):
    monkeypatch.chdir(tmp_path)  # mean_wide writes dump*.raw files to the working directory
    img = cls((24, 25, 26), 0)
    a = np.array(img, copy=False)
    rng = np.random.default_rng(12)
    ii, jj, kk = np.indices(a.shape)
    solid = ((ii // 5 + jj // 6 + kk // 4) % 3) == 0
    a[...] = np.clip(np.where(solid, 150, 60) + 20.0 * ii / a.shape[0] + rng.normal(0, 3, a.shape), 3, 240).astype(a.dtype)
    ref, got = cls(img), cls(img)
    with ik.backend("voxlib"):
        ref.mean_wide(width=2, noise_val=4, average=100, delta=20, iterations=3)
    with ik.backend(backend):
        got.mean_wide(width=2, noise_val=4, average=100, delta=20, iterations=3)
    assert not np.array_equal(np.array(img, copy=False), np.array(ref, copy=False))  # the test exercises something
    assert np.array_equal(np.array(ref, copy=False), np.array(got, copy=False))
