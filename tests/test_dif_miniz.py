import image3kit as ik
import pytest

def test_vxl_dif_short_u8():
    img1 = ik.VxlImgU8(shape=(2, 2, 2), value=100)
    img2 = ik.VxlImgU8(shape=(2, 2, 2), value=50)
    img1.VxlDifShort(img2, scaletype="lin", shift3=0.0, scale3=1.0, shift1=0.0, scale1=1.0, shift2=0.0, scale2=1.0)
    # 100 - 50 = 50
    assert img1.data.tolist() == [[[50, 50], [50, 50]], [[50, 50], [50, 50]]]

def test_vxl_dif_short_u16():
    img1 = ik.VxlImgU16(shape=(2, 2, 2), value=1000)
    img2 = ik.VxlImgU16(shape=(2, 2, 2), value=400)
    img1.VxlDifShort(img2, scaletype="lin", shift3=0.0, scale3=1.0, shift1=0.0, scale1=1.0, shift2=0.0, scale2=1.0)
    # 1000 - 400 = 600
    assert img1.data.tolist() == [[[600, 600], [600, 600]], [[600, 600], [600, 600]]]

def test_vxl_miniz_var_u8():
    img1 = ik.VxlImgU8(shape=(2, 2, 2), value=100)
    img2 = ik.VxlImgU8(shape=(2, 2, 2), value=100)
    # Should work and not crash
    img1.VxlMinizVar(img2, bgn=0, end=255, shift=-1.0, span=1.0)
