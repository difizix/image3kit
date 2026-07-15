from __future__ import annotations

import image3kit as ik


def test_vxlimg_copy_constructor():
    # Create an image
    img1 = ik.VxlImgI32(shape=(3, 4, 5), value=42)
    img1.spacing = (1e-6, 2e-6, 3e-6)
    img1.origin = (0.1, 0.2, 0.3)

    # Initialize from the image
    img2 = ik.VxlImgI32(img1)

    # Verify shape
    assert img2.shape == (3, 4, 5)
    # Verify data buffer copy
    assert img2.data.tolist() == img1.data.tolist()
    # Verify metadata fields
    assert img2.spacing == (1e-6, 2e-6, 3e-6)
    assert img2.origin == (0.1, 0.2, 0.3)


def test_vxlimg_subclass_copy():
    class MySubclass(ik.VxlImgI32):
        def __init__(self, other):
            super().__init__(other)

    img1 = ik.VxlImgI32(shape=(2, 2, 2), value=7)
    img1.spacing = (2e-6, 2e-6, 2e-6)

    sub = MySubclass(img1)
    assert sub.shape == (2, 2, 2)
    assert sub.data.tolist() == [[[7, 7], [7, 7]], [[7, 7], [7, 7]]]
    assert sub.spacing == (2e-6, 2e-6, 2e-6)
