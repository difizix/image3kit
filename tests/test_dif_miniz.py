from __future__ import annotations

import image3kit as ik


def test_scaled_diff_u8():
    img1 = ik.VxlImgU8(shape=(2, 2, 2), value=100)
    img2 = ik.VxlImgU8(shape=(2, 2, 2), value=50)
    img1.scaled_diff(img2, scaletype="lin", shift3=0.0, scale3=1.0, shift1=0.0, scale1=1.0, shift2=0.0, scale2=1.0)
    # 100 - 50 = 50
    assert img1.data.tolist() == [[[50, 50], [50, 50]], [[50, 50], [50, 50]]]


def test_scaled_diff_u16():
    img1 = ik.VxlImgU16(shape=(2, 2, 2), value=1000)
    img2 = ik.VxlImgU16(shape=(2, 2, 2), value=400)
    img1.scaled_diff(img2, scaletype="lin", shift3=0.0, scale3=1.0, shift1=0.0, scale1=1.0, shift2=0.0, scale2=1.0)
    # 1000 - 400 = 600
    assert img1.data.tolist() == [[[600, 600], [600, 600]], [[600, 600], [600, 600]]]


def test_blend_min_variance_u8():
    img1 = ik.VxlImgU8(shape=(2, 2, 2), value=100)
    img2 = ik.VxlImgU8(shape=(2, 2, 2), value=100)
    # Should work and not crash
    img1.blend_min_variance(img2, bgn=0, end=255, shift=-1.0, span=1.0)
