from __future__ import annotations

import image3kit as ik


def test_bitwise_not():
    # Test for U8 image (0-255)
    img = ik.VxlImgU8(shape=(10, 10, 10), value=1)

    # Set some values
    img2 = img.copy()
    img2[0, 0, 0] = 0
    img2[1, 1, 1] = 255
    img2[2, 2, 2] = 100

    assert img2[0, 0, 0] == 0
    assert img2[1, 1, 1] == 255
    assert img2[2, 2, 2] == 100
    assert img2[3, 3, 3] == 1

    img.not_(img2)

    # Check logical inversion
    assert img[0, 0, 0] == 1
    assert img[1, 1, 1] == 0
    assert img[2, 2, 2] == 0
    assert img[3, 3, 3] == 0


def test_operate():
    # Test U8 unary
    img = ik.VxlImgU8(shape=(2, 2, 2), value=15)
    img.operate("~")
    assert img[0, 0, 0] == 240  # ~15 = 240 in 8-bit

    img.operate("=", 0)
    assert img[0, 0, 0] == 0
    img[0, 0, 0] = 5
    img.operate("!")
    assert img[0, 0, 0] == 0
    assert img[1, 1, 1] == 1

    # Test U8 scalar arithmetic with clamping/overflow protection
    img = ik.VxlImgU8(shape=(2, 2, 2), value=200)
    img.operate("+", 100)
    assert img[0, 0, 0] == 255  # Clamped to maxT(u8) = 255

    img.operate("-", 300)
    assert img[0, 0, 0] == 0  # Clamped to minT(u8) = 0

    img.operate("=", 50)
    img.operate("*", 3)
    assert img[0, 0, 0] == 150

    img.operate("b", 180)
    assert img[0, 0, 0] == 180  # bound min

    img.operate("e", 100)
    assert img[0, 0, 0] == 100  # bound max

    # Test U8 binary image operate
    img1 = ik.VxlImgU8(shape=(2, 2, 2), value=150)
    img2 = ik.VxlImgU8(shape=(2, 2, 2), value=120)
    img1.operate("+", img2)
    assert img1[0, 0, 0] == 255  # Clamped 150 + 120 = 270 -> 255

    # Test U16 image operate
    img_u16 = ik.VxlImgU16(shape=(2, 2, 2), value=60000)
    img_u16.operate("+", 10000)
    assert img_u16[0, 0, 0] == 65535  # Clamped to maxT(u16) = 65535
