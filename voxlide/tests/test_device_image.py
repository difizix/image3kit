"""DevImg (img.to_device()): every method gives the VxlImg result, and a chain of ops uploads/downloads once."""

from __future__ import annotations

import numpy as np
import pytest

import image3kit as ik

CLASSES = [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32, ik.VxlImgF32]


def _labelled(cls, shape=(17, 23, 31), seed=1):
    img = cls(shape, 0)
    a = np.array(img, copy=False)
    a[...] = np.random.default_rng(seed).integers(1, 5, a.shape) * 10 + 5
    return img


def _arr(img):
    return np.array(img.to_host() if hasattr(img, "to_host") else img, copy=False)


# The same lambda runs on a VxlImg and on its DevImg: the methods have the same names and arguments.
CALLS = {
    "mode26": lambda m, o: m.mode26(2),
    "mode6": lambda m, o: m.mode6(2),
    "median_filter": lambda m, o: m.median_filter(2),
    "median_x": lambda m, o: m.median_x(),
    "median_y": lambda m, o: m.median_y(),
    "median_z": lambda m, o: m.median_z(),
    "median06": lambda m, o: m.median06(2, 4),
    "median032": lambda m, o: m.median032(14, 14, 15, 25),
    "grow_label": lambda m, o: m.grow_label(15),
    "grow0": lambda m, o: m.grow0(),
    "shrink0": lambda m, o: m.shrink0(),
    "fill_holes": lambda m, o: m.fill_holes(2),
    "replace_range": lambda m, o: m.replace_range(15, 25, 9),
    "threshold101": lambda m, o: m.threshold101(15, 25),
    "circle_out": lambda m, o: m.circle_out(8, 11, 7, "z", 9),
    "and_": lambda m, o: m.and_(o),
    "or_": lambda m, o: m.or_(o),
    "xor_": lambda m, o: m.xor_(o),
    "not_": lambda m, o: m.not_(o),
    "iadd": lambda m, o: m.__iadd__(o),
    "isub": lambda m, o: m.__isub__(o),
    "operate_scalar": lambda m, o: m.operate("+", 3.0),
    "operate_img": lambda m, o: m.operate("-", o, 2.0),
    "operate_unary": lambda m, o: m.operate("!"),
    "map_from": lambda m, o: m.map_from(o, 15, 25, 1.0, 0.5),
    "smooth_bilateral": lambda m, o: m.smooth_bilateral(iterations=2, kernel_radius=2, sigma_val=16.0, sharpness=0.1),
    "bilateral_gauss": lambda m, o: m.bilateral_gauss(iterations=2, kernel_radius=1),
    "grow_outside_value": lambda m, o: m.grow_outside_value(0, 2, 2),
    "grow_box": lambda m, o: m.grow_box(3),
    "shrink_box": lambda m, o: m.shrink_box(2),
    "crop": lambda m, o: m.crop((1, 2, 3), (15, 20, 30), 2, 7),
    "redirect_z": lambda m, o: m.redirect("z"),
    "resample": lambda m, o: m.resample(0.5),
    "resample_mean": lambda m, o: m.resample_mean(2.0),
    "reslice_z": lambda m, o: m.reslice_z(0.4),
    "resample_max": lambda m, o: m.resample_max(2.0),
    "resample_mode": lambda m, o: m.resample_mode(3.0),
    "keep_largest": lambda m, o: m.keep_largest(10, 30),
}


@pytest.mark.parametrize("backend", ik.available_backends())
def test_copy_stays_on_device(backend):
    ref = _labelled(ik.VxlImgU8)
    with ik.backend(backend):
        dev = ik.VxlImgU8(ref).to_device()
        dev.median_filter()
        cpy = dev.copy()
        dev.median_filter()  # the copy is independent
        ref.median_filter()
        assert np.array_equal(np.array(ref, copy=False), np.array(cpy.to_host(), copy=False))
        if backend == "gpu":
            assert cpy.transfers()[1] == 1  # only the to_host() above


def _pair(cls, backend):
    ref = _labelled(cls)
    other = _labelled(cls, seed=2)
    return ref, other


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", CLASSES)
@pytest.mark.parametrize("name", sorted(CALLS))
def test_method_matches_vxlimg(backend, cls, name):
    ref, other = _pair(cls, backend)
    got, got_other = cls(ref), cls(other)
    call = CALLS[name]
    if name.startswith("operate") and cls in (ik.VxlImgI32, ik.VxlImgF32):
        pytest.skip("operate is bound for u8/u16 only")
    with ik.backend(backend):
        r_ref = call(ref, other)
        dev, dev_other = got.to_device(), got_other.to_device()
        r_dev = call(dev, dev_other)
        res = dev.to_host()
    assert np.array_equal(np.array(ref, copy=False), np.array(res, copy=False))
    if isinstance(r_ref, (int, float)):
        assert r_ref == r_dev


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", CLASSES)
def test_statistics_and_otsu(backend, cls):
    ref = _labelled(cls)
    with ik.backend(backend):
        dev = cls(ref).to_device()
        for f in ("mean_value", "min_value", "max_value"):
            assert getattr(ref, f)() == getattr(dev, f)()
        assert ref.variance(0, 255) == pytest.approx(dev.variance(0, 255), rel=1e-10)
        assert ref.porosity(20) == dev.porosity(20)
        assert ref.otsu_threshold(0, 255) == dev.otsu_threshold(0, 255)
        if cls in (ik.VxlImgU8, ik.VxlImgU16):
            mask = ik.threshold01_otsu(ref, 0, 255)
            dmask = dev.threshold01_otsu(0, 255).to_host()
            assert np.array_equal(np.array(mask, copy=False), np.array(dmask, copy=False))


@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16])
def test_segment2_on_device(cls):
    rng = np.random.default_rng(3)
    img = cls((24, 24, 24), 0)
    a = np.array(img, copy=False)
    a[...] = np.where(rng.random(a.shape) < 0.5, 40, 180) + rng.integers(-5, 5, a.shape)
    ref, dev = cls(img), cls(img).to_device()
    args = dict(thresholds=[1, 100, 254], min_sizes=[1, 2])
    with ik.backend("voxlib"):
        ref.segment2(**args)
    dev.segment2(**args)
    assert np.array_equal(np.array(ref, copy=False), np.array(dev.to_host(), copy=False))


@pytest.mark.parametrize("backend", ik.available_backends())
@pytest.mark.parametrize("cls", [ik.VxlImgU8, ik.VxlImgU16, ik.VxlImgI32])
def test_mean_wide_on_device(backend, cls, tmp_path, monkeypatch):
    monkeypatch.chdir(tmp_path)  # mean_wide writes dump*.raw files to the working directory
    img = cls((22, 23, 24), 0)
    a = np.array(img, copy=False)
    ii, jj, kk = np.indices(a.shape)
    a[...] = np.clip(
        np.where(((ii // 5 + jj // 6 + kk // 4) % 3) == 0, 150, 60) + np.random.default_rng(4).normal(0, 3, a.shape),
        3,
        240,
    ).astype(a.dtype)
    ref = cls(img)
    with ik.backend("voxlib"):
        ref.mean_wide(width=2, average=100, iterations=3)
    with ik.backend(backend):
        dev = cls(img).to_device()
        dev.mean_wide(width=2, average=100, iterations=3)
        assert np.array_equal(np.array(ref, copy=False), np.array(dev.to_host(), copy=False))


def test_shape_copy_and_chain_transfers():
    img = _labelled(ik.VxlImgU8, (20, 21, 22))
    assert img.to_device().shape == (20, 21, 22)
    with ik.backend("gpu" if "gpu" in ik.available_backends() else "voxlib"):
        ref = ik.VxlImgU8(img)
        ref.grow_box(2)
        ref.median_filter(1)
        ref.mode26(2)
        ref.shrink_box(2)
        dev = img.to_device()
        dev.grow_box(2)
        dev.median_filter(1)
        dev.mode26(2)
        dev.shrink_box(2)
        cp = dev.copy()
        assert np.array_equal(np.array(ref, copy=False), np.array(dev.to_host(), copy=False))
        assert cp.shape == dev.shape
        if "gpu" in ik.available_backends():
            assert dev.transfers() == (1, 1)  # one upload for the whole chain, one download


def test_device_image_isolated_from_source():
    img = _labelled(ik.VxlImgU8)
    before = np.array(img, copy=True)
    dev = img.to_device()
    dev.operate("+", 5.0)
    assert np.array_equal(before, np.array(img, copy=False))  # to_device copies


# ---- run_slabs: a chain of DevImg steps per z-slab with a deep halo equals the steps on the whole image
@pytest.mark.parametrize("cls", CLASSES)
def test_run_slabs_equals_whole_image(cls):
    img = _labelled(cls, (19, 17, 41), seed=4)
    ref = img.copy()
    with ik.backend("voxlib"):
        ref.median_filter(2)
        ref.mode26(1)
    steps = [(2, lambda d: d.median_filter(2)), (1, lambda d: d.mode26(1))]
    layer = 19 * 17 * np.dtype(np.array(img, copy=False).dtype).itemsize
    for be in ik.available_backends():
        with ik.backend(be):
            for interior in (0, 1, 5):  # 0: one piece
                ik.set_max_slab_bytes((2 * 3 + interior) * layer if interior else 0)
                try:
                    got = img.copy()
                    ik.run_slabs(got, steps)
                finally:
                    ik.set_max_slab_bytes(0)
                assert np.array_equal(_arr(got), _arr(ref)), (be, interior)


def test_run_slabs_slab_range_and_second_image():
    img = _labelled(ik.VxlImgU8, (8, 9, 30), seed=5)
    src = _labelled(ik.VxlImgU8, (8, 9, 30), seed=6)
    seen = []

    def step(d, z):  # the second image's slab lines up with the piece
        seen.append(z)
        part = ik.slab_of(src, z)
        assert np.array(part, copy=False).shape == d.shape
        d.and_(part.to_device())

    ref = img.copy()
    with ik.backend("voxlib"):
        ref.and_(src)
    ik.set_max_slab_bytes(8 * 9 * 7)  # at most 7 layers per piece, halo 0
    try:
        got = img.copy()
        ik.run_slabs(got, [(0, step)])
    finally:
        ik.set_max_slab_bytes(0)
    assert np.array_equal(_arr(got), _arr(ref))
    assert seen == [(6 * i, 6 * i + 6) for i in range(5)]  # ceil(30 / 7) = 5 slabs, split evenly
