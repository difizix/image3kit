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
