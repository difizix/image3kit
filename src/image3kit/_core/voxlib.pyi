"""
Auto-generated wrapper for voxelImageT template C++ classes.
"""

from __future__ import annotations

import collections.abc
import typing

import numpy
import numpy.typing

import image3kit._core.sirun

__all__: list[str] = [
    "VxlImgF32",
    "VxlImgI32",
    "VxlImgU8",
    "VxlImgU16",
    "connected_components",
    "cube",
    "cylinder",
    "read_image",
    "shape",
    "sphere",
    "voxelImageTBase",
]

class VxlImgF32(voxelImageTBase):
    def __add__(self, arg0: VxlImgF32) -> VxlImgF32: ...
    def __array__(self) -> numpy.typing.NDArray[numpy.float32]:
        """
        Get the raw data buffer as a numpy array.
        """
    def __buffer__(self, flags):
        """
        Return a buffer object that exposes the underlying memory of the object.
        """
    def __getitem__(self, arg0: image3kit._core.sirun.int3) -> float: ...
    def __iadd__(self, arg0: VxlImgF32) -> VxlImgF32: ...
    @typing.overload
    def __init__(
        self, shape: image3kit._core.sirun.int3 = ..., value: typing.SupportsFloat | typing.SupportsIndex = 0
    ) -> None:
        """
        Initialize a new image of size (nx, ny, nz) with the fill value.
        """
    @typing.overload
    def __init__(self, image: voxelImageTBase) -> None:
        """
        Initialize (duplicate and convert) from another voxelImageT object
        """
    @typing.overload
    def __init__(
        self, filepath: typing.Any, processKeys: bool = True, max_nz: typing.SupportsInt | typing.SupportsIndex = -1
    ) -> None:
        """
        Read image dimensions/metadata from a (header) file. Supported file types are .am, .raw
        """
    def __isub__(self, arg0: VxlImgF32) -> VxlImgF32: ...
    def __release_buffer__(self, buffer):
        """
        Release the buffer object that exposes the underlying memory of the object.
        """
    def __repr__(self) -> str: ...
    def __setitem__(
        self, arg0: image3kit._core.sirun.int3, arg1: typing.SupportsFloat | typing.SupportsIndex
    ) -> None: ...
    def __sub__(self, arg0: VxlImgF32) -> VxlImgF32: ...
    def add_surf_noise(
        self,
        mask1: typing.SupportsInt | typing.SupportsIndex,
        mask2: typing.SupportsInt | typing.SupportsIndex,
        threshold: typing.SupportsInt | typing.SupportsIndex,
        seed: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Add surface noise.
        """
    def adjust_brightness_with(self, image_file: str) -> bool: ...
    def adjust_slice_brightness(
        self,
        mask_a: VxlImgU8,
        mask_b: VxlImgU8,
        ref_image: VxlImgF32,
        smooth_iter: typing.SupportsInt | typing.SupportsIndex = 3,
        smooth_kernel: typing.SupportsInt | typing.SupportsIndex = 20,
    ) -> bool: ...
    def and_(self, image2: VxlImgF32) -> None:
        """
        Voxel-by-voxel inplace AND operation.
        """
    def average_with(self, arg0: collections.abc.Sequence[str]) -> bool: ...
    def average_with_skip_extremes(self, filenames: collections.abc.Sequence[str]) -> bool:
        """
        average image with list of other images (read from disk) while skipping the two outliers in color range, per voxel
        """
    def bilateral_gauss(
        self,
        iterations: typing.SupportsInt | typing.SupportsIndex = 1,
        kernel_radius: typing.SupportsInt | typing.SupportsIndex = 1,
        sigma_val: typing.SupportsFloat | typing.SupportsIndex = 16.0,
        sharpness: typing.SupportsFloat | typing.SupportsIndex = 0.1,
        sigma_spatial: typing.SupportsFloat | typing.SupportsIndex = 2.0,
    ) -> bool: ...
    def bilateral_wide(
        self,
        iterations: typing.SupportsInt | typing.SupportsIndex = 1,
        kernel_radius: typing.SupportsInt | typing.SupportsIndex = 1,
        x_step: typing.SupportsInt | typing.SupportsIndex = 2,
        sigma_val: typing.SupportsFloat | typing.SupportsIndex = 16.0,
        sharpness: typing.SupportsFloat | typing.SupportsIndex = 0.1,
        sigma_spatial: typing.SupportsFloat | typing.SupportsIndex = 2.0,
    ) -> bool:
        """
        Bilateral filter with Xtra large kernel radius, actual kernel size is: kernel_radius * x_step cubed.
        """
    def circle_out(
        self,
        x: typing.SupportsInt | typing.SupportsIndex,
        y: typing.SupportsInt | typing.SupportsIndex,
        r: typing.SupportsInt | typing.SupportsIndex,
        normal_axis: str,
        val: typing.SupportsFloat | typing.SupportsIndex,
    ) -> None:
        """
        set values outside circular region centered at (x,y) of radius r along normal_axis to val
        """
    def copy(self) -> VxlImgF32:
        """
        duplicate the image data
        """
    def crop(
        self,
        begin: image3kit._core.sirun.int3,
        end: image3kit._core.sirun.int3,
        emptyLayers: typing.SupportsInt | typing.SupportsIndex = 0,
        emptyLayersValue: typing.SupportsFloat | typing.SupportsIndex = 1,
        verbose: bool = False,
    ) -> None:
        """
        Crop the image (inplace) from begin index tupe ix,iy,iz (inclusive) to and and end index (not inclusive) tuple.
        """
    def cut_outside(
        self,
        axis: str = "z",
        extra_out: typing.SupportsInt | typing.SupportsIndex = 0,
        threshold: typing.SupportsInt | typing.SupportsIndex = -1,
        cut_highs: typing.SupportsInt | typing.SupportsIndex = 0,
        shift_x: typing.SupportsInt | typing.SupportsIndex = 0,
        shift_y: typing.SupportsInt | typing.SupportsIndex = 0,
        fill_val: typing.SupportsInt | typing.SupportsIndex = 0,
    ) -> None: ...
    def delense032(
        self,
        iterations: typing.SupportsInt | typing.SupportsIndex,
        nAdj0: typing.SupportsInt | typing.SupportsIndex,
        nAdj1: typing.SupportsInt | typing.SupportsIndex,
        lbl0: typing.SupportsFloat | typing.SupportsIndex,
        lbl1: typing.SupportsFloat | typing.SupportsIndex,
    ) -> None:
        """
        Delense operation.
        """
    def dering(
        self,
        x0: typing.SupportsInt | typing.SupportsIndex,
        y0: typing.SupportsInt | typing.SupportsIndex,
        x1: typing.SupportsInt | typing.SupportsIndex,
        y1: typing.SupportsInt | typing.SupportsIndex,
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = 255,
        nr: typing.SupportsInt | typing.SupportsIndex = 0,
        ntheta: typing.SupportsInt | typing.SupportsIndex = 18,
        nz: typing.SupportsInt | typing.SupportsIndex = 0,
        scale_dif_val: typing.SupportsFloat | typing.SupportsIndex = 1,
        bilateral_shape: typing.SupportsFloat | typing.SupportsIndex = 0.05,
        n_grow_box: typing.SupportsInt | typing.SupportsIndex = 10,
        write_dumps: bool = True,
    ) -> None: ...
    def direction(self, arg0: str) -> None:
        """
        Get direction?
        """
    def face_median06(
        self, nAdj0: typing.SupportsInt | typing.SupportsIndex, nAdj1: typing.SupportsInt | typing.SupportsIndex
    ) -> int:
        """
        Set voxel value to 0/1 if it has more than nAdj0/1 neighbours with value 0/1, in its 6 nearest voxels
        """
    def face_median_grow(
        self,
        label_to: typing.SupportsFloat | typing.SupportsIndex,
        label_from: typing.SupportsFloat | typing.SupportsIndex,
        n_diff: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Face median grow to/from labels.
        """
    def fill_holes(self, maxHoleRadius: typing.SupportsInt | typing.SupportsIndex) -> None:
        """
        Fill closed holes in the image.
        """
    def flip_endian(self) -> None: ...
    def grow0(self) -> None:
        """
        Grow pore phase (voxel values of 0).
        """
    def grow_box(self, num_layers: typing.SupportsInt | typing.SupportsIndex) -> None:
        """
        Expand the image boundaries, increasing its size by `num_layers` in all directions
        """
    def grow_label(self, arg0: typing.SupportsFloat | typing.SupportsIndex) -> None: ...
    def grow_outside_value(
        self,
        val_old: typing.SupportsInt | typing.SupportsIndex = 0,
        val_new: typing.SupportsInt | typing.SupportsIndex = 2,
        hole_size: typing.SupportsInt | typing.SupportsIndex = 5,
    ) -> bool: ...
    def growing_threshold(
        self,
        startMin: typing.SupportsFloat | typing.SupportsIndex,
        startMax: typing.SupportsFloat | typing.SupportsIndex,
        finalMin: typing.SupportsFloat | typing.SupportsIndex,
        finalMax: typing.SupportsFloat | typing.SupportsIndex,
        iterations: typing.SupportsInt | typing.SupportsIndex = 4,
    ) -> None: ...
    def keep_largest(
        self, min: typing.SupportsFloat | typing.SupportsIndex, max: typing.SupportsFloat | typing.SupportsIndex
    ) -> None:
        """
        Keep largest singly-connected region with values in [min, max].
        """
    def map_from(
        self,
        source_image: VxlImgF32,
        vmin: typing.SupportsFloat | typing.SupportsIndex,
        vmax: typing.SupportsFloat | typing.SupportsIndex,
        scale: typing.SupportsFloat | typing.SupportsIndex,
        shift: typing.SupportsFloat | typing.SupportsIndex,
    ) -> None:
        """
        Map values from another image.
        """
    def mean_wide(
        self,
        width: typing.SupportsInt | typing.SupportsIndex = 0,
        noise_val: typing.SupportsInt | typing.SupportsIndex = 4,
        average: typing.SupportsInt | typing.SupportsIndex = 0,
        delta: typing.SupportsInt | typing.SupportsIndex = 20,
        iterations: typing.SupportsInt | typing.SupportsIndex = 15,
        smooth_image: str = "",
    ) -> bool:
        """
        computes a background image, used to correct for lens artifacts
        """
    def median_filter(self) -> None:
        """
        Apply a 1+6-neighbour median filter.
        """
    def median_x(self) -> None:
        """
        Apply median filter with kernel size of 1 voxels in x-direction
        """
    def median_y(self) -> None:
        """
        Apply median filter with kernel size of 1 voxels in y-direction
        """
    def median_z(self) -> None:
        """
        Apply median filter with kernel size of 1 voxels in z-direction
        """
    def mode26(self, arg0: typing.SupportsInt | typing.SupportsIndex) -> None: ...
    def mode6(self, n_same_neighbors: typing.SupportsInt | typing.SupportsIndex) -> int:
        """
        Apply mode filter based on nearest 6 neighbor voxels.
        """
    def not_(self, image2: VxlImgF32) -> None:
        """
        Voxel-by-voxel inplace NOT operation, alias for img = img & !img2.
        """
    def or_(self, image2: VxlImgF32) -> None:
        """
        Voxel-by-voxel inplace OR operation.
        """
    def otsu_threshold(
        self,
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = 256,
    ) -> typing.Annotated[list[float], "FixedSize(5)"]: ...
    def paint(self, shape: shape) -> None:
        """
        Paint (set values of) a shape into the image.
        """
    def paint_add(self, shape: shape) -> None:
        """
        Add (+) a shape's value to the image.
        """
    def paint_add_after(self, shape: shape) -> None:
        """
        Add (+) a shape's value after the shape (plane...)
        """
    def paint_add_before(self, shape: shape) -> None:
        """
        Add (+) a shape's value before the shape (plane...)
        """
    def paint_after(self, shape: shape) -> None:
        """
        Paint after the shape (plane...)
        """
    def paint_before(self, shape: shape) -> None:
        """
        Paint before the shape (plane...)
        """
    def plot_all(
        self,
        filename: str = "pltAll",
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = -1000001,
        slice_index: typing.SupportsInt | typing.SupportsIndex = -1000000,
        histogram_bins: typing.SupportsInt | typing.SupportsIndex = 128,
        normal_axis: str = "xyz",
        grey: bool = True,
        color: bool = True,
        histogram: bool = True,
        z_profile: bool = True,
        alpha_image: VxlImgF32 = None,
        alpha_min: typing.SupportsInt | typing.SupportsIndex = 0,
        alpha_max: typing.SupportsInt | typing.SupportsIndex = -1000001,
    ) -> bool:
        """
        Plot all visualizations (Histogram, ZProfile, Slices) with various options, hackish for debugging
        """
    def plot_histogram(
        self,
        filename: str = "aa.svg",
        bins: typing.SupportsInt | typing.SupportsIndex = 128,
        min_val: typing.SupportsFloat | typing.SupportsIndex = 3e38,
        max_val: typing.SupportsFloat | typing.SupportsIndex = -3e38,
    ) -> int: ...
    def plot_slice(
        self,
        filename: str,
        normal_axis: str = "xyz",
        slice_index: typing.SupportsInt | typing.SupportsIndex = -1000000,
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = -1000001,
        color_map: str = "gray",
    ) -> None:
        """
        Save a 2D slice as a PNG image.
        normalAxis can be 'x', 'y', 'z', 'xy', 'xz', 'yz', 'xyz'.
        color_map can be 'gray' or 'RGB'
        """
    def plot_z_profile(
        self,
        filename: str = "aa.svg",
        min_val: typing.SupportsFloat | typing.SupportsIndex = 0,
        max_val: typing.SupportsFloat | typing.SupportsIndex = 255,
    ) -> int: ...
    def point_median032(
        self,
        nAdj0: typing.SupportsInt | typing.SupportsIndex,
        nAdj1: typing.SupportsInt | typing.SupportsIndex,
        lbl0: typing.SupportsFloat | typing.SupportsIndex,
        lbl1: typing.SupportsFloat | typing.SupportsIndex,
    ) -> None:
        """
        Set voxel value to lbl0/1 if it has more than nAdj0/1 neighbours with value lbl0/1, in its 6+26 nearest voxels
        """
    def print_info(self) -> None: ...
    def range_to(
        self,
        min: typing.SupportsFloat | typing.SupportsIndex,
        max: typing.SupportsFloat | typing.SupportsIndex,
        val: typing.SupportsFloat | typing.SupportsIndex,
    ) -> None:
        """
        Set values in range [min, max] to val.
        """
    def read_ascii(self, filename: str, n_skip_bytes: typing.SupportsInt | typing.SupportsIndex = 0) -> bool:
        """
        Read image data from an ASCII file.
        """
    def read_bin(
        self,
        filename: str,
        n_skip_bytes: typing.SupportsInt | typing.SupportsIndex = 0,
        max_nz: typing.SupportsInt | typing.SupportsIndex = -1,
    ) -> None:
        """
        Read image data from  a .raw, .raw/.raw.gz, or reset and read from a .am or .tif file.
        """
    def read_from_float(
        self,
        filename: str,
        scale: typing.SupportsFloat | typing.SupportsIndex = 1.0,
        shift: typing.SupportsFloat | typing.SupportsIndex = 0.0,
    ) -> bool: ...
    def read_from_header(self, filename: str, max_nz: typing.SupportsInt | typing.SupportsIndex = -1) -> None:
        """
        Reset and read image from file.
        """
    def redirect(self, arg0: str) -> None:
        """
        Swap X axis with the given axis (y or z).
        """
    def replace_by_image_range(
        self,
        min_val: typing.SupportsFloat | typing.SupportsIndex,
        max_val: typing.SupportsFloat | typing.SupportsIndex,
        image_file: str,
    ) -> None: ...
    def replace_range(
        self,
        min: typing.SupportsFloat | typing.SupportsIndex,
        max: typing.SupportsFloat | typing.SupportsIndex,
        val: typing.SupportsFloat | typing.SupportsIndex,
    ) -> None:
        """
        Replace values in range [min, max] with val.
        """
    def replace_range_by_image(
        self,
        min_val: typing.SupportsFloat | typing.SupportsIndex,
        max_val: typing.SupportsFloat | typing.SupportsIndex,
        image_file: str,
    ) -> None: ...
    def resample(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> None:
        """
        Resample image by factor f, using averaging (downsampling, f>1) or nearest when upsampling (f<1)
        """
    def resample_max(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> VxlImgF32:
        """
        Downsample the image, setting voxel values to maximum of original encompassing voxel values.
        """
    def resample_mean(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> VxlImgF32:
        """
        Resample the image using mean interpolation.
        """
    def resample_mode(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> VxlImgF32:
        """
        Downsample the image, setting voxel values to mode of original encompassing voxel values.
        """
    def rescale_values(
        self, min: typing.SupportsFloat | typing.SupportsIndex, max: typing.SupportsFloat | typing.SupportsIndex
    ) -> None:
        """
        Rescale image values to [min, max].
        """
    def reslice_z(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> VxlImgF32:
        """
        Reslice along the Z axis.
        """
    def shrink0(self) -> None:
        """
        Shrink pore phase (voxel values of 0).
        """
    def shrink_box(self, num_layers: typing.SupportsInt | typing.SupportsIndex) -> None:
        """
        Shrink the image boundaries, decreasing its size by the given num_layers in all directions
        """
    def smooth_bilateral(
        self,
        iterations: typing.SupportsInt | typing.SupportsIndex = 1,
        kernel_radius: typing.SupportsInt | typing.SupportsIndex = 1,
        sigma_val: typing.SupportsFloat | typing.SupportsIndex = 16.0,
        sharpness: typing.SupportsFloat | typing.SupportsIndex = 0.1,
    ) -> bool:
        """
        bilateral smoothing filter
        """
    def threshold101(
        self, min: typing.SupportsFloat | typing.SupportsIndex, max: typing.SupportsFloat | typing.SupportsIndex
    ) -> None:
        """
        Apply a threshold to binarize the image, set voxel-values to convert to 0 in between the min and max thresholds and 1 outside of it
        """
    def variance(
        self,
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = 255,
    ) -> float:
        """
        Set outer tubing of a circular core-holder image to fill_val
        """
    def write(self, filename: str) -> None:
        """
        Write the image to a file (.mhd, .raw, .ra.gz formats).
        """
    def write_8bit(
        self,
        filename: str,
        min: typing.SupportsFloat | typing.SupportsIndex = 0.0,
        max: typing.SupportsFloat | typing.SupportsIndex = -0.5,
    ) -> None:
        """
        Write as 8-bit image scaled between min(=>0) and max(=>255).
        """
    def write_a_connected_void_voxel(self, filename: str) -> None:
        """
        Write a specific connected pore voxel to file.
        """
    def write_contour(self, out_surf: str) -> None:
        """
        Write contour extraction to a surface file.
        """
    def write_no_header(self, filename: str) -> None:
        """
        Write the raw image data without a header.
        """
    def xor_(self, image2: VxlImgF32) -> None:
        """
        Voxel-by-voxel inplace XOR operation.
        """
    @property
    def data(self) -> typing.Any:
        """
        Get the raw data buffer as a numpy array.
        """
    @property
    def nx(self) -> int: ...
    @property
    def ny(self) -> int: ...
    @property
    def nz(self) -> int: ...
    @property
    def origin(self) -> tuple:
        """
        Get/set the origin value (x0, y0, z0).
        """
    @origin.setter
    def origin(self, arg1: image3kit._core.sirun.dbl3) -> None: ...
    @property
    def shape(self) -> tuple: ...
    @property
    def spacing(self) -> tuple:
        """
        Get/set the voxel size (dx, dy, dz).
        """
    @spacing.setter
    def spacing(self, arg1: image3kit._core.sirun.dbl3) -> None: ...

class VxlImgI32(voxelImageTBase):
    def __add__(self, arg0: VxlImgI32) -> VxlImgI32: ...
    def __array__(self) -> numpy.typing.NDArray[numpy.int32]:
        """
        Get the raw data buffer as a numpy array.
        """
    def __buffer__(self, flags):
        """
        Return a buffer object that exposes the underlying memory of the object.
        """
    def __getitem__(self, arg0: image3kit._core.sirun.int3) -> int: ...
    def __iadd__(self, arg0: VxlImgI32) -> VxlImgI32: ...
    @typing.overload
    def __init__(
        self, shape: image3kit._core.sirun.int3 = ..., value: typing.SupportsInt | typing.SupportsIndex = 0
    ) -> None:
        """
        Initialize a new image of size (nx, ny, nz) with the fill value.
        """
    @typing.overload
    def __init__(self, image: voxelImageTBase) -> None:
        """
        Initialize (duplicate and convert) from another voxelImageT object
        """
    @typing.overload
    def __init__(
        self, filepath: typing.Any, processKeys: bool = True, max_nz: typing.SupportsInt | typing.SupportsIndex = -1
    ) -> None:
        """
        Read image dimensions/metadata from a (header) file. Supported file types are .am, .raw
        """
    def __isub__(self, arg0: VxlImgI32) -> VxlImgI32: ...
    def __release_buffer__(self, buffer):
        """
        Release the buffer object that exposes the underlying memory of the object.
        """
    def __repr__(self) -> str: ...
    def __setitem__(
        self, arg0: image3kit._core.sirun.int3, arg1: typing.SupportsInt | typing.SupportsIndex
    ) -> None: ...
    def __sub__(self, arg0: VxlImgI32) -> VxlImgI32: ...
    def add_surf_noise(
        self,
        mask1: typing.SupportsInt | typing.SupportsIndex,
        mask2: typing.SupportsInt | typing.SupportsIndex,
        threshold: typing.SupportsInt | typing.SupportsIndex,
        seed: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Add surface noise.
        """
    def adjust_brightness_with(self, image_file: str) -> bool: ...
    def adjust_slice_brightness(
        self,
        mask_a: VxlImgU8,
        mask_b: VxlImgU8,
        ref_image: VxlImgI32,
        smooth_iter: typing.SupportsInt | typing.SupportsIndex = 3,
        smooth_kernel: typing.SupportsInt | typing.SupportsIndex = 20,
    ) -> bool: ...
    def and_(self, image2: VxlImgI32) -> None:
        """
        Voxel-by-voxel inplace AND operation.
        """
    def average_with(self, arg0: collections.abc.Sequence[str]) -> bool: ...
    def average_with_skip_extremes(self, filenames: collections.abc.Sequence[str]) -> bool:
        """
        average image with list of other images (read from disk) while skipping the two outliers in color range, per voxel
        """
    def bilateral_gauss(
        self,
        iterations: typing.SupportsInt | typing.SupportsIndex = 1,
        kernel_radius: typing.SupportsInt | typing.SupportsIndex = 1,
        sigma_val: typing.SupportsFloat | typing.SupportsIndex = 16.0,
        sharpness: typing.SupportsFloat | typing.SupportsIndex = 0.1,
        sigma_spatial: typing.SupportsFloat | typing.SupportsIndex = 2.0,
    ) -> bool: ...
    def bilateral_wide(
        self,
        iterations: typing.SupportsInt | typing.SupportsIndex = 1,
        kernel_radius: typing.SupportsInt | typing.SupportsIndex = 1,
        x_step: typing.SupportsInt | typing.SupportsIndex = 2,
        sigma_val: typing.SupportsFloat | typing.SupportsIndex = 16.0,
        sharpness: typing.SupportsFloat | typing.SupportsIndex = 0.1,
        sigma_spatial: typing.SupportsFloat | typing.SupportsIndex = 2.0,
    ) -> bool:
        """
        Bilateral filter with Xtra large kernel radius, actual kernel size is: kernel_radius * x_step cubed.
        """
    def circle_out(
        self,
        x: typing.SupportsInt | typing.SupportsIndex,
        y: typing.SupportsInt | typing.SupportsIndex,
        r: typing.SupportsInt | typing.SupportsIndex,
        normal_axis: str,
        val: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        set values outside circular region centered at (x,y) of radius r along normal_axis to val
        """
    def copy(self) -> VxlImgI32:
        """
        duplicate the image data
        """
    def crop(
        self,
        begin: image3kit._core.sirun.int3,
        end: image3kit._core.sirun.int3,
        emptyLayers: typing.SupportsInt | typing.SupportsIndex = 0,
        emptyLayersValue: typing.SupportsInt | typing.SupportsIndex = 1,
        verbose: bool = False,
    ) -> None:
        """
        Crop the image (inplace) from begin index tupe ix,iy,iz (inclusive) to and and end index (not inclusive) tuple.
        """
    def cut_outside(
        self,
        axis: str = "z",
        extra_out: typing.SupportsInt | typing.SupportsIndex = 0,
        threshold: typing.SupportsInt | typing.SupportsIndex = -1,
        cut_highs: typing.SupportsInt | typing.SupportsIndex = 0,
        shift_x: typing.SupportsInt | typing.SupportsIndex = 0,
        shift_y: typing.SupportsInt | typing.SupportsIndex = 0,
        fill_val: typing.SupportsInt | typing.SupportsIndex = 0,
    ) -> None: ...
    def delense032(
        self,
        iterations: typing.SupportsInt | typing.SupportsIndex,
        nAdj0: typing.SupportsInt | typing.SupportsIndex,
        nAdj1: typing.SupportsInt | typing.SupportsIndex,
        lbl0: typing.SupportsInt | typing.SupportsIndex,
        lbl1: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Delense operation.
        """
    def dering(
        self,
        x0: typing.SupportsInt | typing.SupportsIndex,
        y0: typing.SupportsInt | typing.SupportsIndex,
        x1: typing.SupportsInt | typing.SupportsIndex,
        y1: typing.SupportsInt | typing.SupportsIndex,
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = 255,
        nr: typing.SupportsInt | typing.SupportsIndex = 0,
        ntheta: typing.SupportsInt | typing.SupportsIndex = 18,
        nz: typing.SupportsInt | typing.SupportsIndex = 0,
        scale_dif_val: typing.SupportsFloat | typing.SupportsIndex = 1,
        bilateral_shape: typing.SupportsFloat | typing.SupportsIndex = 0.05,
        n_grow_box: typing.SupportsInt | typing.SupportsIndex = 10,
        write_dumps: bool = True,
    ) -> None: ...
    def direction(self, arg0: str) -> None:
        """
        Get direction?
        """
    def face_median06(
        self, nAdj0: typing.SupportsInt | typing.SupportsIndex, nAdj1: typing.SupportsInt | typing.SupportsIndex
    ) -> int:
        """
        Set voxel value to 0/1 if it has more than nAdj0/1 neighbours with value 0/1, in its 6 nearest voxels
        """
    def face_median_grow(
        self,
        label_to: typing.SupportsInt | typing.SupportsIndex,
        label_from: typing.SupportsInt | typing.SupportsIndex,
        n_diff: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Face median grow to/from labels.
        """
    def fill_holes(self, maxHoleRadius: typing.SupportsInt | typing.SupportsIndex) -> None:
        """
        Fill closed holes in the image.
        """
    def flip_endian(self) -> None: ...
    def grow0(self) -> None:
        """
        Grow pore phase (voxel values of 0).
        """
    def grow_box(self, num_layers: typing.SupportsInt | typing.SupportsIndex) -> None:
        """
        Expand the image boundaries, increasing its size by `num_layers` in all directions
        """
    def grow_label(self, arg0: typing.SupportsInt | typing.SupportsIndex) -> None: ...
    def grow_outside_value(
        self,
        val_old: typing.SupportsInt | typing.SupportsIndex = 0,
        val_new: typing.SupportsInt | typing.SupportsIndex = 2,
        hole_size: typing.SupportsInt | typing.SupportsIndex = 5,
    ) -> bool: ...
    def growing_threshold(
        self,
        startMin: typing.SupportsInt | typing.SupportsIndex,
        startMax: typing.SupportsInt | typing.SupportsIndex,
        finalMin: typing.SupportsInt | typing.SupportsIndex,
        finalMax: typing.SupportsInt | typing.SupportsIndex,
        iterations: typing.SupportsInt | typing.SupportsIndex = 4,
    ) -> None: ...
    def keep_largest(
        self, min: typing.SupportsInt | typing.SupportsIndex, max: typing.SupportsInt | typing.SupportsIndex
    ) -> None:
        """
        Keep largest singly-connected region with values in [min, max].
        """
    def map_from(
        self,
        source_image: VxlImgI32,
        vmin: typing.SupportsInt | typing.SupportsIndex,
        vmax: typing.SupportsInt | typing.SupportsIndex,
        scale: typing.SupportsFloat | typing.SupportsIndex,
        shift: typing.SupportsFloat | typing.SupportsIndex,
    ) -> None:
        """
        Map values from another image.
        """
    def mean_wide(
        self,
        width: typing.SupportsInt | typing.SupportsIndex = 0,
        noise_val: typing.SupportsInt | typing.SupportsIndex = 4,
        average: typing.SupportsInt | typing.SupportsIndex = 0,
        delta: typing.SupportsInt | typing.SupportsIndex = 20,
        iterations: typing.SupportsInt | typing.SupportsIndex = 15,
        smooth_image: str = "",
    ) -> bool:
        """
        computes a background image, used to correct for lens artifacts
        """
    def median_filter(self) -> None:
        """
        Apply a 1+6-neighbour median filter.
        """
    def median_x(self) -> None:
        """
        Apply median filter with kernel size of 1 voxels in x-direction
        """
    def median_y(self) -> None:
        """
        Apply median filter with kernel size of 1 voxels in y-direction
        """
    def median_z(self) -> None:
        """
        Apply median filter with kernel size of 1 voxels in z-direction
        """
    def mode26(self, arg0: typing.SupportsInt | typing.SupportsIndex) -> None: ...
    def mode6(self, n_same_neighbors: typing.SupportsInt | typing.SupportsIndex) -> int:
        """
        Apply mode filter based on nearest 6 neighbor voxels.
        """
    def not_(self, image2: VxlImgI32) -> None:
        """
        Voxel-by-voxel inplace NOT operation, alias for img = img & !img2.
        """
    def or_(self, image2: VxlImgI32) -> None:
        """
        Voxel-by-voxel inplace OR operation.
        """
    def otsu_threshold(
        self,
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = 256,
    ) -> typing.Annotated[list[float], "FixedSize(5)"]: ...
    def paint(self, shape: shape) -> None:
        """
        Paint (set values of) a shape into the image.
        """
    def paint_add(self, shape: shape) -> None:
        """
        Add (+) a shape's value to the image.
        """
    def paint_add_after(self, shape: shape) -> None:
        """
        Add (+) a shape's value after the shape (plane...)
        """
    def paint_add_before(self, shape: shape) -> None:
        """
        Add (+) a shape's value before the shape (plane...)
        """
    def paint_after(self, shape: shape) -> None:
        """
        Paint after the shape (plane...)
        """
    def paint_before(self, shape: shape) -> None:
        """
        Paint before the shape (plane...)
        """
    def plot_all(
        self,
        filename: str = "pltAll",
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = -1000001,
        slice_index: typing.SupportsInt | typing.SupportsIndex = -1000000,
        histogram_bins: typing.SupportsInt | typing.SupportsIndex = 128,
        normal_axis: str = "xyz",
        grey: bool = True,
        color: bool = True,
        histogram: bool = True,
        z_profile: bool = True,
        alpha_image: VxlImgI32 = None,
        alpha_min: typing.SupportsInt | typing.SupportsIndex = 0,
        alpha_max: typing.SupportsInt | typing.SupportsIndex = -1000001,
    ) -> bool:
        """
        Plot all visualizations (Histogram, ZProfile, Slices) with various options, hackish for debugging
        """
    def plot_histogram(
        self,
        filename: str = "aa.svg",
        bins: typing.SupportsInt | typing.SupportsIndex = 128,
        min_val: typing.SupportsFloat | typing.SupportsIndex = 3e38,
        max_val: typing.SupportsFloat | typing.SupportsIndex = -3e38,
    ) -> int: ...
    def plot_slice(
        self,
        filename: str,
        normal_axis: str = "xyz",
        slice_index: typing.SupportsInt | typing.SupportsIndex = -1000000,
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = -1000001,
        color_map: str = "gray",
    ) -> None:
        """
        Save a 2D slice as a PNG image.
        normalAxis can be 'x', 'y', 'z', 'xy', 'xz', 'yz', 'xyz'.
        color_map can be 'gray' or 'RGB'
        """
    def plot_z_profile(
        self,
        filename: str = "aa.svg",
        min_val: typing.SupportsFloat | typing.SupportsIndex = 0,
        max_val: typing.SupportsFloat | typing.SupportsIndex = 255,
    ) -> int: ...
    def point_median032(
        self,
        nAdj0: typing.SupportsInt | typing.SupportsIndex,
        nAdj1: typing.SupportsInt | typing.SupportsIndex,
        lbl0: typing.SupportsInt | typing.SupportsIndex,
        lbl1: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Set voxel value to lbl0/1 if it has more than nAdj0/1 neighbours with value lbl0/1, in its 6+26 nearest voxels
        """
    def print_info(self) -> None: ...
    def range_to(
        self,
        min: typing.SupportsInt | typing.SupportsIndex,
        max: typing.SupportsInt | typing.SupportsIndex,
        val: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Set values in range [min, max] to val.
        """
    def read_ascii(self, filename: str, n_skip_bytes: typing.SupportsInt | typing.SupportsIndex = 0) -> bool:
        """
        Read image data from an ASCII file.
        """
    def read_bin(
        self,
        filename: str,
        n_skip_bytes: typing.SupportsInt | typing.SupportsIndex = 0,
        max_nz: typing.SupportsInt | typing.SupportsIndex = -1,
    ) -> None:
        """
        Read image data from  a .raw, .raw/.raw.gz, or reset and read from a .am or .tif file.
        """
    def read_from_float(
        self,
        filename: str,
        scale: typing.SupportsFloat | typing.SupportsIndex = 1.0,
        shift: typing.SupportsFloat | typing.SupportsIndex = 0.0,
    ) -> bool: ...
    def read_from_header(self, filename: str, max_nz: typing.SupportsInt | typing.SupportsIndex = -1) -> None:
        """
        Reset and read image from file.
        """
    def redirect(self, arg0: str) -> None:
        """
        Swap X axis with the given axis (y or z).
        """
    def replace_by_image_range(
        self,
        min_val: typing.SupportsFloat | typing.SupportsIndex,
        max_val: typing.SupportsFloat | typing.SupportsIndex,
        image_file: str,
    ) -> None: ...
    def replace_range(
        self,
        min: typing.SupportsInt | typing.SupportsIndex,
        max: typing.SupportsInt | typing.SupportsIndex,
        val: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Replace values in range [min, max] with val.
        """
    def replace_range_by_image(
        self,
        min_val: typing.SupportsFloat | typing.SupportsIndex,
        max_val: typing.SupportsFloat | typing.SupportsIndex,
        image_file: str,
    ) -> None: ...
    def resample(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> None:
        """
        Resample image by factor f, using averaging (downsampling, f>1) or nearest when upsampling (f<1)
        """
    def resample_max(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> VxlImgI32:
        """
        Downsample the image, setting voxel values to maximum of original encompassing voxel values.
        """
    def resample_mean(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> VxlImgI32:
        """
        Resample the image using mean interpolation.
        """
    def resample_mode(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> VxlImgI32:
        """
        Downsample the image, setting voxel values to mode of original encompassing voxel values.
        """
    def rescale_values(
        self, min: typing.SupportsInt | typing.SupportsIndex, max: typing.SupportsInt | typing.SupportsIndex
    ) -> None:
        """
        Rescale image values to [min, max].
        """
    def reslice_z(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> VxlImgI32:
        """
        Reslice along the Z axis.
        """
    def shrink0(self) -> None:
        """
        Shrink pore phase (voxel values of 0).
        """
    def shrink_box(self, num_layers: typing.SupportsInt | typing.SupportsIndex) -> None:
        """
        Shrink the image boundaries, decreasing its size by the given num_layers in all directions
        """
    def smooth_bilateral(
        self,
        iterations: typing.SupportsInt | typing.SupportsIndex = 1,
        kernel_radius: typing.SupportsInt | typing.SupportsIndex = 1,
        sigma_val: typing.SupportsFloat | typing.SupportsIndex = 16.0,
        sharpness: typing.SupportsFloat | typing.SupportsIndex = 0.1,
    ) -> bool:
        """
        bilateral smoothing filter
        """
    def threshold101(
        self, min: typing.SupportsInt | typing.SupportsIndex, max: typing.SupportsInt | typing.SupportsIndex
    ) -> None:
        """
        Apply a threshold to binarize the image, set voxel-values to convert to 0 in between the min and max thresholds and 1 outside of it
        """
    def variance(
        self,
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = 255,
    ) -> float:
        """
        Set outer tubing of a circular core-holder image to fill_val
        """
    def write(self, filename: str) -> None:
        """
        Write the image to a file (.mhd, .raw, .ra.gz formats).
        """
    def write_8bit(
        self,
        filename: str,
        min: typing.SupportsFloat | typing.SupportsIndex = 0.0,
        max: typing.SupportsFloat | typing.SupportsIndex = -0.5,
    ) -> None:
        """
        Write as 8-bit image scaled between min(=>0) and max(=>255).
        """
    def write_a_connected_void_voxel(self, filename: str) -> None:
        """
        Write a specific connected pore voxel to file.
        """
    def write_contour(self, out_surf: str) -> None:
        """
        Write contour extraction to a surface file.
        """
    def write_no_header(self, filename: str) -> None:
        """
        Write the raw image data without a header.
        """
    def xor_(self, image2: VxlImgI32) -> None:
        """
        Voxel-by-voxel inplace XOR operation.
        """
    @property
    def data(self) -> typing.Any:
        """
        Get the raw data buffer as a numpy array.
        """
    @property
    def nx(self) -> int: ...
    @property
    def ny(self) -> int: ...
    @property
    def nz(self) -> int: ...
    @property
    def origin(self) -> tuple:
        """
        Get/set the origin value (x0, y0, z0).
        """
    @origin.setter
    def origin(self, arg1: image3kit._core.sirun.dbl3) -> None: ...
    @property
    def shape(self) -> tuple: ...
    @property
    def spacing(self) -> tuple:
        """
        Get/set the voxel size (dx, dy, dz).
        """
    @spacing.setter
    def spacing(self, arg1: image3kit._core.sirun.dbl3) -> None: ...

class VxlImgU16(voxelImageTBase):
    def __add__(self, arg0: VxlImgU16) -> VxlImgU16: ...
    def __array__(self) -> numpy.typing.NDArray[numpy.uint16]:
        """
        Get the raw data buffer as a numpy array.
        """
    def __buffer__(self, flags):
        """
        Return a buffer object that exposes the underlying memory of the object.
        """
    def __getitem__(self, arg0: image3kit._core.sirun.int3) -> int: ...
    def __iadd__(self, arg0: VxlImgU16) -> VxlImgU16: ...
    @typing.overload
    def __init__(
        self, shape: image3kit._core.sirun.int3 = ..., value: typing.SupportsInt | typing.SupportsIndex = 0
    ) -> None:
        """
        Initialize a new image of size (nx, ny, nz) with the fill value.
        """
    @typing.overload
    def __init__(self, image: voxelImageTBase) -> None:
        """
        Initialize (duplicate and convert) from another voxelImageT object
        """
    @typing.overload
    def __init__(
        self, filepath: typing.Any, processKeys: bool = True, max_nz: typing.SupportsInt | typing.SupportsIndex = -1
    ) -> None:
        """
        Read image dimensions/metadata from a (header) file. Supported file types are .am, .raw
        """
    def __isub__(self, arg0: VxlImgU16) -> VxlImgU16: ...
    def __release_buffer__(self, buffer):
        """
        Release the buffer object that exposes the underlying memory of the object.
        """
    def __repr__(self) -> str: ...
    def __setitem__(
        self, arg0: image3kit._core.sirun.int3, arg1: typing.SupportsInt | typing.SupportsIndex
    ) -> None: ...
    def __sub__(self, arg0: VxlImgU16) -> VxlImgU16: ...
    def add_surf_noise(
        self,
        mask1: typing.SupportsInt | typing.SupportsIndex,
        mask2: typing.SupportsInt | typing.SupportsIndex,
        threshold: typing.SupportsInt | typing.SupportsIndex,
        seed: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Add surface noise.
        """
    def adjust_brightness_with(self, image_file: str) -> bool: ...
    def adjust_slice_brightness(
        self,
        mask_a: VxlImgU8,
        mask_b: VxlImgU8,
        ref_image: VxlImgU16,
        smooth_iter: typing.SupportsInt | typing.SupportsIndex = 3,
        smooth_kernel: typing.SupportsInt | typing.SupportsIndex = 20,
    ) -> bool: ...
    def and_(self, image2: VxlImgU16) -> None:
        """
        Voxel-by-voxel inplace AND operation.
        """
    def average_with(self, arg0: collections.abc.Sequence[str]) -> bool: ...
    def average_with_skip_extremes(self, filenames: collections.abc.Sequence[str]) -> bool:
        """
        average image with list of other images (read from disk) while skipping the two outliers in color range, per voxel
        """
    def bilateral_gauss(
        self,
        iterations: typing.SupportsInt | typing.SupportsIndex = 1,
        kernel_radius: typing.SupportsInt | typing.SupportsIndex = 1,
        sigma_val: typing.SupportsFloat | typing.SupportsIndex = 16.0,
        sharpness: typing.SupportsFloat | typing.SupportsIndex = 0.1,
        sigma_spatial: typing.SupportsFloat | typing.SupportsIndex = 2.0,
    ) -> bool: ...
    def bilateral_wide(
        self,
        iterations: typing.SupportsInt | typing.SupportsIndex = 1,
        kernel_radius: typing.SupportsInt | typing.SupportsIndex = 1,
        x_step: typing.SupportsInt | typing.SupportsIndex = 2,
        sigma_val: typing.SupportsFloat | typing.SupportsIndex = 16.0,
        sharpness: typing.SupportsFloat | typing.SupportsIndex = 0.1,
        sigma_spatial: typing.SupportsFloat | typing.SupportsIndex = 2.0,
    ) -> bool:
        """
        Bilateral filter with Xtra large kernel radius, actual kernel size is: kernel_radius * x_step cubed.
        """
    def blend_min_variance(
        self,
        image2: VxlImgU16,
        bgn: typing.SupportsInt | typing.SupportsIndex = 0,
        end: typing.SupportsInt | typing.SupportsIndex = 65535,
        shift: typing.SupportsFloat | typing.SupportsIndex = -1.0,
        span: typing.SupportsFloat | typing.SupportsIndex = 1.0,
    ) -> None:
        """
        Search for an optimum weight, w, that minimizes variance of img1*w+(1-w)*img2
        """
    def circle_out(
        self,
        x: typing.SupportsInt | typing.SupportsIndex,
        y: typing.SupportsInt | typing.SupportsIndex,
        r: typing.SupportsInt | typing.SupportsIndex,
        normal_axis: str,
        val: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        set values outside circular region centered at (x,y) of radius r along normal_axis to val
        """
    def copy(self) -> VxlImgU16:
        """
        duplicate the image data
        """
    def crop(
        self,
        begin: image3kit._core.sirun.int3,
        end: image3kit._core.sirun.int3,
        emptyLayers: typing.SupportsInt | typing.SupportsIndex = 0,
        emptyLayersValue: typing.SupportsInt | typing.SupportsIndex = 1,
        verbose: bool = False,
    ) -> None:
        """
        Crop the image (inplace) from begin index tupe ix,iy,iz (inclusive) to and and end index (not inclusive) tuple.
        """
    def cut_outside(
        self,
        axis: str = "z",
        extra_out: typing.SupportsInt | typing.SupportsIndex = 0,
        threshold: typing.SupportsInt | typing.SupportsIndex = -1,
        cut_highs: typing.SupportsInt | typing.SupportsIndex = 0,
        shift_x: typing.SupportsInt | typing.SupportsIndex = 0,
        shift_y: typing.SupportsInt | typing.SupportsIndex = 0,
        fill_val: typing.SupportsInt | typing.SupportsIndex = 0,
    ) -> None: ...
    def delense032(
        self,
        iterations: typing.SupportsInt | typing.SupportsIndex,
        nAdj0: typing.SupportsInt | typing.SupportsIndex,
        nAdj1: typing.SupportsInt | typing.SupportsIndex,
        lbl0: typing.SupportsInt | typing.SupportsIndex,
        lbl1: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Delense operation.
        """
    def dering(
        self,
        x0: typing.SupportsInt | typing.SupportsIndex,
        y0: typing.SupportsInt | typing.SupportsIndex,
        x1: typing.SupportsInt | typing.SupportsIndex,
        y1: typing.SupportsInt | typing.SupportsIndex,
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = 255,
        nr: typing.SupportsInt | typing.SupportsIndex = 0,
        ntheta: typing.SupportsInt | typing.SupportsIndex = 18,
        nz: typing.SupportsInt | typing.SupportsIndex = 0,
        scale_dif_val: typing.SupportsFloat | typing.SupportsIndex = 1,
        bilateral_shape: typing.SupportsFloat | typing.SupportsIndex = 0.05,
        n_grow_box: typing.SupportsInt | typing.SupportsIndex = 10,
        write_dumps: bool = True,
    ) -> None: ...
    def direction(self, arg0: str) -> None:
        """
        Get direction?
        """
    def face_median06(
        self, nAdj0: typing.SupportsInt | typing.SupportsIndex, nAdj1: typing.SupportsInt | typing.SupportsIndex
    ) -> int:
        """
        Set voxel value to 0/1 if it has more than nAdj0/1 neighbours with value 0/1, in its 6 nearest voxels
        """
    def face_median_grow(
        self,
        label_to: typing.SupportsInt | typing.SupportsIndex,
        label_from: typing.SupportsInt | typing.SupportsIndex,
        n_diff: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Face median grow to/from labels.
        """
    def fill_holes(self, maxHoleRadius: typing.SupportsInt | typing.SupportsIndex) -> None:
        """
        Fill closed holes in the image.
        """
    def flip_endian(self) -> None: ...
    def grow0(self) -> None:
        """
        Grow pore phase (voxel values of 0).
        """
    def grow_box(self, num_layers: typing.SupportsInt | typing.SupportsIndex) -> None:
        """
        Expand the image boundaries, increasing its size by `num_layers` in all directions
        """
    def grow_label(self, arg0: typing.SupportsInt | typing.SupportsIndex) -> None: ...
    def grow_outside_value(
        self,
        val_old: typing.SupportsInt | typing.SupportsIndex = 0,
        val_new: typing.SupportsInt | typing.SupportsIndex = 2,
        hole_size: typing.SupportsInt | typing.SupportsIndex = 5,
    ) -> bool: ...
    def growing_threshold(
        self,
        startMin: typing.SupportsInt | typing.SupportsIndex,
        startMax: typing.SupportsInt | typing.SupportsIndex,
        finalMin: typing.SupportsInt | typing.SupportsIndex,
        finalMax: typing.SupportsInt | typing.SupportsIndex,
        iterations: typing.SupportsInt | typing.SupportsIndex = 4,
    ) -> None: ...
    def keep_largest(
        self, min: typing.SupportsInt | typing.SupportsIndex, max: typing.SupportsInt | typing.SupportsIndex
    ) -> None:
        """
        Keep largest singly-connected region with values in [min, max].
        """
    def map_from(
        self,
        source_image: VxlImgU16,
        vmin: typing.SupportsInt | typing.SupportsIndex,
        vmax: typing.SupportsInt | typing.SupportsIndex,
        scale: typing.SupportsFloat | typing.SupportsIndex,
        shift: typing.SupportsFloat | typing.SupportsIndex,
    ) -> None:
        """
        Map values from another image.
        """
    def mean_wide(
        self,
        width: typing.SupportsInt | typing.SupportsIndex = 0,
        noise_val: typing.SupportsInt | typing.SupportsIndex = 4,
        average: typing.SupportsInt | typing.SupportsIndex = 0,
        delta: typing.SupportsInt | typing.SupportsIndex = 20,
        iterations: typing.SupportsInt | typing.SupportsIndex = 15,
        smooth_image: str = "",
    ) -> bool:
        """
        computes a background image, used to correct for lens artifacts
        """
    def median_filter(self) -> None:
        """
        Apply a 1+6-neighbour median filter.
        """
    def median_x(self) -> None:
        """
        Apply median filter with kernel size of 1 voxels in x-direction
        """
    def median_y(self) -> None:
        """
        Apply median filter with kernel size of 1 voxels in y-direction
        """
    def median_z(self) -> None:
        """
        Apply median filter with kernel size of 1 voxels in z-direction
        """
    def mode26(self, arg0: typing.SupportsInt | typing.SupportsIndex) -> None: ...
    def mode6(self, n_same_neighbors: typing.SupportsInt | typing.SupportsIndex) -> int:
        """
        Apply mode filter based on nearest 6 neighbor voxels.
        """
    def not_(self, image2: VxlImgU16) -> None:
        """
        Voxel-by-voxel inplace NOT operation, alias for img = img & !img2.
        """
    def or_(self, image2: VxlImgU16) -> None:
        """
        Voxel-by-voxel inplace OR operation.
        """
    def otsu_threshold(
        self,
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = 256,
    ) -> typing.Annotated[list[float], "FixedSize(5)"]: ...
    def paint(self, shape: shape) -> None:
        """
        Paint (set values of) a shape into the image.
        """
    def paint_add(self, shape: shape) -> None:
        """
        Add (+) a shape's value to the image.
        """
    def paint_add_after(self, shape: shape) -> None:
        """
        Add (+) a shape's value after the shape (plane...)
        """
    def paint_add_before(self, shape: shape) -> None:
        """
        Add (+) a shape's value before the shape (plane...)
        """
    def paint_after(self, shape: shape) -> None:
        """
        Paint after the shape (plane...)
        """
    def paint_before(self, shape: shape) -> None:
        """
        Paint before the shape (plane...)
        """
    def plot_all(
        self,
        filename: str = "pltAll",
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = -1000001,
        slice_index: typing.SupportsInt | typing.SupportsIndex = -1000000,
        histogram_bins: typing.SupportsInt | typing.SupportsIndex = 128,
        normal_axis: str = "xyz",
        grey: bool = True,
        color: bool = True,
        histogram: bool = True,
        z_profile: bool = True,
        alpha_image: VxlImgU16 = None,
        alpha_min: typing.SupportsInt | typing.SupportsIndex = 0,
        alpha_max: typing.SupportsInt | typing.SupportsIndex = -1000001,
    ) -> bool:
        """
        Plot all visualizations (Histogram, ZProfile, Slices) with various options, hackish for debugging
        """
    def plot_histogram(
        self,
        filename: str = "aa.svg",
        bins: typing.SupportsInt | typing.SupportsIndex = 128,
        min_val: typing.SupportsFloat | typing.SupportsIndex = 3e38,
        max_val: typing.SupportsFloat | typing.SupportsIndex = -3e38,
    ) -> int: ...
    def plot_slice(
        self,
        filename: str,
        normal_axis: str = "xyz",
        slice_index: typing.SupportsInt | typing.SupportsIndex = -1000000,
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = -1000001,
        color_map: str = "gray",
    ) -> None:
        """
        Save a 2D slice as a PNG image.
        normalAxis can be 'x', 'y', 'z', 'xy', 'xz', 'yz', 'xyz'.
        color_map can be 'gray' or 'RGB'
        """
    def plot_z_profile(
        self,
        filename: str = "aa.svg",
        min_val: typing.SupportsFloat | typing.SupportsIndex = 0,
        max_val: typing.SupportsFloat | typing.SupportsIndex = 255,
    ) -> int: ...
    def point_median032(
        self,
        nAdj0: typing.SupportsInt | typing.SupportsIndex,
        nAdj1: typing.SupportsInt | typing.SupportsIndex,
        lbl0: typing.SupportsInt | typing.SupportsIndex,
        lbl1: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Set voxel value to lbl0/1 if it has more than nAdj0/1 neighbours with value lbl0/1, in its 6+26 nearest voxels
        """
    def print_info(self) -> None: ...
    def range_to(
        self,
        min: typing.SupportsInt | typing.SupportsIndex,
        max: typing.SupportsInt | typing.SupportsIndex,
        val: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Set values in range [min, max] to val.
        """
    def read_ascii(self, filename: str, n_skip_bytes: typing.SupportsInt | typing.SupportsIndex = 0) -> bool:
        """
        Read image data from an ASCII file.
        """
    def read_bin(
        self,
        filename: str,
        n_skip_bytes: typing.SupportsInt | typing.SupportsIndex = 0,
        max_nz: typing.SupportsInt | typing.SupportsIndex = -1,
    ) -> None:
        """
        Read image data from  a .raw, .raw/.raw.gz, or reset and read from a .am or .tif file.
        """
    def read_from_float(
        self,
        filename: str,
        scale: typing.SupportsFloat | typing.SupportsIndex = 1.0,
        shift: typing.SupportsFloat | typing.SupportsIndex = 0.0,
    ) -> bool: ...
    def read_from_header(self, filename: str, max_nz: typing.SupportsInt | typing.SupportsIndex = -1) -> None:
        """
        Reset and read image from file.
        """
    def redirect(self, arg0: str) -> None:
        """
        Swap X axis with the given axis (y or z).
        """
    def replace_by_image_range(
        self,
        min_val: typing.SupportsFloat | typing.SupportsIndex,
        max_val: typing.SupportsFloat | typing.SupportsIndex,
        image_file: str,
    ) -> None: ...
    def replace_range(
        self,
        min: typing.SupportsInt | typing.SupportsIndex,
        max: typing.SupportsInt | typing.SupportsIndex,
        val: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Replace values in range [min, max] with val.
        """
    def replace_range_by_image(
        self,
        min_val: typing.SupportsFloat | typing.SupportsIndex,
        max_val: typing.SupportsFloat | typing.SupportsIndex,
        image_file: str,
    ) -> None: ...
    def resample(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> None:
        """
        Resample image by factor f, using averaging (downsampling, f>1) or nearest when upsampling (f<1)
        """
    def resample_max(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> VxlImgU16:
        """
        Downsample the image, setting voxel values to maximum of original encompassing voxel values.
        """
    def resample_mean(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> VxlImgU16:
        """
        Resample the image using mean interpolation.
        """
    def resample_mode(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> VxlImgU16:
        """
        Downsample the image, setting voxel values to mode of original encompassing voxel values.
        """
    def rescale_values(
        self, min: typing.SupportsInt | typing.SupportsIndex, max: typing.SupportsInt | typing.SupportsIndex
    ) -> None:
        """
        Rescale image values to [min, max].
        """
    def reslice_z(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> VxlImgU16:
        """
        Reslice along the Z axis.
        """
    def scaled_diff(
        self,
        image2: VxlImgU16,
        scaletype: str = "log",
        shift3: typing.SupportsFloat | typing.SupportsIndex = 1.0,
        scale3: typing.SupportsFloat | typing.SupportsIndex = 1.0,
        shift1: typing.SupportsFloat | typing.SupportsIndex = 0.0,
        scale1: typing.SupportsFloat | typing.SupportsIndex = 1.0,
        shift2: typing.SupportsFloat | typing.SupportsIndex = 0.0,
        scale2: typing.SupportsFloat | typing.SupportsIndex = 1.0,
    ) -> None:
        """
        Calculate difference between two images, linear or logarithmic scale
        """
    def segment(
        self,
        n_segments: typing.SupportsInt | typing.SupportsIndex = 2,
        thresholds: collections.abc.Sequence[typing.SupportsInt | typing.SupportsIndex] = [],
        min_sizes: collections.abc.Sequence[typing.SupportsInt | typing.SupportsIndex] = [],
        smooth_image: str = "",
        noise_val: typing.SupportsFloat | typing.SupportsIndex = 16.0,
        resolution_sq: typing.SupportsFloat | typing.SupportsIndex = 2.0,
        write_dumps: typing.SupportsInt | typing.SupportsIndex = 0,
    ) -> bool: ...
    def segment2(
        self,
        thresholds: collections.abc.Sequence[typing.SupportsInt | typing.SupportsIndex] = [],
        min_sizes: collections.abc.Sequence[typing.SupportsInt | typing.SupportsIndex] = [],
        noise_val: typing.SupportsFloat | typing.SupportsIndex = 2.0,
        local_factor: typing.SupportsFloat | typing.SupportsIndex = 0.05,
        flatnes: typing.SupportsFloat | typing.SupportsIndex = 0.1,
        effective_resolution: typing.SupportsFloat | typing.SupportsIndex = 2.0,
        gradient_factor: typing.SupportsFloat | typing.SupportsIndex = 0.0,
        kernel_radius: typing.SupportsInt | typing.SupportsIndex = 2,
        n_iterations: typing.SupportsInt | typing.SupportsIndex = 13,
        write_dumps: typing.SupportsInt | typing.SupportsIndex = 0,
    ) -> bool: ...
    def shrink0(self) -> None:
        """
        Shrink pore phase (voxel values of 0).
        """
    def shrink_box(self, num_layers: typing.SupportsInt | typing.SupportsIndex) -> None:
        """
        Shrink the image boundaries, decreasing its size by the given num_layers in all directions
        """
    def smooth_bilateral(
        self,
        iterations: typing.SupportsInt | typing.SupportsIndex = 1,
        kernel_radius: typing.SupportsInt | typing.SupportsIndex = 1,
        sigma_val: typing.SupportsFloat | typing.SupportsIndex = 16.0,
        sharpness: typing.SupportsFloat | typing.SupportsIndex = 0.1,
    ) -> bool:
        """
        bilateral smoothing filter
        """
    def threshold101(
        self, min: typing.SupportsInt | typing.SupportsIndex, max: typing.SupportsInt | typing.SupportsIndex
    ) -> None:
        """
        Apply a threshold to binarize the image, set voxel-values to convert to 0 in between the min and max thresholds and 1 outside of it
        """
    def variance(
        self,
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = 255,
    ) -> float:
        """
        Set outer tubing of a circular core-holder image to fill_val
        """
    def write(self, filename: str) -> None:
        """
        Write the image to a file (.mhd, .raw, .ra.gz formats).
        """
    def write_8bit(
        self,
        filename: str,
        min: typing.SupportsFloat | typing.SupportsIndex = 0.0,
        max: typing.SupportsFloat | typing.SupportsIndex = -0.5,
    ) -> None:
        """
        Write as 8-bit image scaled between min(=>0) and max(=>255).
        """
    def write_a_connected_void_voxel(self, filename: str) -> None:
        """
        Write a specific connected pore voxel to file.
        """
    def write_contour(self, out_surf: str) -> None:
        """
        Write contour extraction to a surface file.
        """
    def write_no_header(self, filename: str) -> None:
        """
        Write the raw image data without a header.
        """
    def xor_(self, image2: VxlImgU16) -> None:
        """
        Voxel-by-voxel inplace XOR operation.
        """
    @property
    def data(self) -> typing.Any:
        """
        Get the raw data buffer as a numpy array.
        """
    @property
    def nx(self) -> int: ...
    @property
    def ny(self) -> int: ...
    @property
    def nz(self) -> int: ...
    @property
    def origin(self) -> tuple:
        """
        Get/set the origin value (x0, y0, z0).
        """
    @origin.setter
    def origin(self, arg1: image3kit._core.sirun.dbl3) -> None: ...
    @property
    def shape(self) -> tuple: ...
    @property
    def spacing(self) -> tuple:
        """
        Get/set the voxel size (dx, dy, dz).
        """
    @spacing.setter
    def spacing(self, arg1: image3kit._core.sirun.dbl3) -> None: ...

class VxlImgU8(voxelImageTBase):
    def __add__(self, arg0: VxlImgU8) -> VxlImgU8: ...
    def __array__(self) -> numpy.typing.NDArray[numpy.uint8]:
        """
        Get the raw data buffer as a numpy array.
        """
    def __buffer__(self, flags):
        """
        Return a buffer object that exposes the underlying memory of the object.
        """
    def __getitem__(self, arg0: image3kit._core.sirun.int3) -> int: ...
    def __iadd__(self, arg0: VxlImgU8) -> VxlImgU8: ...
    @typing.overload
    def __init__(
        self, shape: image3kit._core.sirun.int3 = ..., value: typing.SupportsInt | typing.SupportsIndex = 0
    ) -> None:
        """
        Initialize a new image of size (nx, ny, nz) with the fill value.
        """
    @typing.overload
    def __init__(self, image: voxelImageTBase) -> None:
        """
        Initialize (duplicate and convert) from another voxelImageT object
        """
    @typing.overload
    def __init__(
        self, filepath: typing.Any, processKeys: bool = True, max_nz: typing.SupportsInt | typing.SupportsIndex = -1
    ) -> None:
        """
        Read image dimensions/metadata from a (header) file. Supported file types are .am, .raw
        """
    def __isub__(self, arg0: VxlImgU8) -> VxlImgU8: ...
    def __release_buffer__(self, buffer):
        """
        Release the buffer object that exposes the underlying memory of the object.
        """
    def __repr__(self) -> str: ...
    def __setitem__(
        self, arg0: image3kit._core.sirun.int3, arg1: typing.SupportsInt | typing.SupportsIndex
    ) -> None: ...
    def __sub__(self, arg0: VxlImgU8) -> VxlImgU8: ...
    def add_surf_noise(
        self,
        mask1: typing.SupportsInt | typing.SupportsIndex,
        mask2: typing.SupportsInt | typing.SupportsIndex,
        threshold: typing.SupportsInt | typing.SupportsIndex,
        seed: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Add surface noise.
        """
    def adjust_brightness_with(self, image_file: str) -> bool: ...
    def adjust_slice_brightness(
        self,
        mask_a: VxlImgU8,
        mask_b: VxlImgU8,
        ref_image: VxlImgU8,
        smooth_iter: typing.SupportsInt | typing.SupportsIndex = 3,
        smooth_kernel: typing.SupportsInt | typing.SupportsIndex = 20,
    ) -> bool: ...
    def and_(self, image2: VxlImgU8) -> None:
        """
        Voxel-by-voxel inplace AND operation.
        """
    def average_with(self, arg0: collections.abc.Sequence[str]) -> bool: ...
    def average_with_skip_extremes(self, filenames: collections.abc.Sequence[str]) -> bool:
        """
        average image with list of other images (read from disk) while skipping the two outliers in color range, per voxel
        """
    def bilateral_gauss(
        self,
        iterations: typing.SupportsInt | typing.SupportsIndex = 1,
        kernel_radius: typing.SupportsInt | typing.SupportsIndex = 1,
        sigma_val: typing.SupportsFloat | typing.SupportsIndex = 16.0,
        sharpness: typing.SupportsFloat | typing.SupportsIndex = 0.1,
        sigma_spatial: typing.SupportsFloat | typing.SupportsIndex = 2.0,
    ) -> bool: ...
    def bilateral_wide(
        self,
        iterations: typing.SupportsInt | typing.SupportsIndex = 1,
        kernel_radius: typing.SupportsInt | typing.SupportsIndex = 1,
        x_step: typing.SupportsInt | typing.SupportsIndex = 2,
        sigma_val: typing.SupportsFloat | typing.SupportsIndex = 16.0,
        sharpness: typing.SupportsFloat | typing.SupportsIndex = 0.1,
        sigma_spatial: typing.SupportsFloat | typing.SupportsIndex = 2.0,
    ) -> bool:
        """
        Bilateral filter with Xtra large kernel radius, actual kernel size is: kernel_radius * x_step cubed.
        """
    def blend_min_variance(
        self,
        image2: VxlImgU8,
        bgn: typing.SupportsInt | typing.SupportsIndex = 0,
        end: typing.SupportsInt | typing.SupportsIndex = 255,
        shift: typing.SupportsFloat | typing.SupportsIndex = -1.0,
        span: typing.SupportsFloat | typing.SupportsIndex = 1.0,
    ) -> None:
        """
        Search for an optimum weight, w, that minimizes variance of img1*w+(1-w)*img2
        """
    def circle_out(
        self,
        x: typing.SupportsInt | typing.SupportsIndex,
        y: typing.SupportsInt | typing.SupportsIndex,
        r: typing.SupportsInt | typing.SupportsIndex,
        normal_axis: str,
        val: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        set values outside circular region centered at (x,y) of radius r along normal_axis to val
        """
    def copy(self) -> VxlImgU8:
        """
        duplicate the image data
        """
    def crop(
        self,
        begin: image3kit._core.sirun.int3,
        end: image3kit._core.sirun.int3,
        emptyLayers: typing.SupportsInt | typing.SupportsIndex = 0,
        emptyLayersValue: typing.SupportsInt | typing.SupportsIndex = 1,
        verbose: bool = False,
    ) -> None:
        """
        Crop the image (inplace) from begin index tupe ix,iy,iz (inclusive) to and and end index (not inclusive) tuple.
        """
    def cut_outside(
        self,
        axis: str = "z",
        extra_out: typing.SupportsInt | typing.SupportsIndex = 0,
        threshold: typing.SupportsInt | typing.SupportsIndex = -1,
        cut_highs: typing.SupportsInt | typing.SupportsIndex = 0,
        shift_x: typing.SupportsInt | typing.SupportsIndex = 0,
        shift_y: typing.SupportsInt | typing.SupportsIndex = 0,
        fill_val: typing.SupportsInt | typing.SupportsIndex = 0,
    ) -> None: ...
    def delense032(
        self,
        iterations: typing.SupportsInt | typing.SupportsIndex,
        nAdj0: typing.SupportsInt | typing.SupportsIndex,
        nAdj1: typing.SupportsInt | typing.SupportsIndex,
        lbl0: typing.SupportsInt | typing.SupportsIndex,
        lbl1: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Delense operation.
        """
    def dering(
        self,
        x0: typing.SupportsInt | typing.SupportsIndex,
        y0: typing.SupportsInt | typing.SupportsIndex,
        x1: typing.SupportsInt | typing.SupportsIndex,
        y1: typing.SupportsInt | typing.SupportsIndex,
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = 255,
        nr: typing.SupportsInt | typing.SupportsIndex = 0,
        ntheta: typing.SupportsInt | typing.SupportsIndex = 18,
        nz: typing.SupportsInt | typing.SupportsIndex = 0,
        scale_dif_val: typing.SupportsFloat | typing.SupportsIndex = 1,
        bilateral_shape: typing.SupportsFloat | typing.SupportsIndex = 0.05,
        n_grow_box: typing.SupportsInt | typing.SupportsIndex = 10,
        write_dumps: bool = True,
    ) -> None: ...
    def direction(self, arg0: str) -> None:
        """
        Get direction?
        """
    def extrude_dist_map(
        self,
        config: dict = {},
        offset: typing.SupportsFloat | typing.SupportsIndex = 0.5,
        scale: typing.SupportsFloat | typing.SupportsIndex = 1.0,
        power: typing.SupportsFloat | typing.SupportsIndex = 1.0,
    ) -> None:
        """
        Extrude proportional to distance map
        """
    def face_median06(
        self, nAdj0: typing.SupportsInt | typing.SupportsIndex, nAdj1: typing.SupportsInt | typing.SupportsIndex
    ) -> int:
        """
        Set voxel value to 0/1 if it has more than nAdj0/1 neighbours with value 0/1, in its 6 nearest voxels
        """
    def face_median_grow(
        self,
        label_to: typing.SupportsInt | typing.SupportsIndex,
        label_from: typing.SupportsInt | typing.SupportsIndex,
        n_diff: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Face median grow to/from labels.
        """
    def fill_holes(self, maxHoleRadius: typing.SupportsInt | typing.SupportsIndex) -> None:
        """
        Fill closed holes in the image.
        """
    def flip_endian(self) -> None: ...
    def grow0(self) -> None:
        """
        Grow pore phase (voxel values of 0).
        """
    def grow_box(self, num_layers: typing.SupportsInt | typing.SupportsIndex) -> None:
        """
        Expand the image boundaries, increasing its size by `num_layers` in all directions
        """
    def grow_label(self, arg0: typing.SupportsInt | typing.SupportsIndex) -> None: ...
    def grow_outside_value(
        self,
        val_old: typing.SupportsInt | typing.SupportsIndex = 0,
        val_new: typing.SupportsInt | typing.SupportsIndex = 2,
        hole_size: typing.SupportsInt | typing.SupportsIndex = 5,
    ) -> bool: ...
    def growing_threshold(
        self,
        startMin: typing.SupportsInt | typing.SupportsIndex,
        startMax: typing.SupportsInt | typing.SupportsIndex,
        finalMin: typing.SupportsInt | typing.SupportsIndex,
        finalMax: typing.SupportsInt | typing.SupportsIndex,
        iterations: typing.SupportsInt | typing.SupportsIndex = 4,
    ) -> None: ...
    def keep_largest(
        self, min: typing.SupportsInt | typing.SupportsIndex, max: typing.SupportsInt | typing.SupportsIndex
    ) -> None:
        """
        Keep largest singly-connected region with values in [min, max].
        """
    def map_from(
        self,
        source_image: VxlImgU8,
        vmin: typing.SupportsInt | typing.SupportsIndex,
        vmax: typing.SupportsInt | typing.SupportsIndex,
        scale: typing.SupportsFloat | typing.SupportsIndex,
        shift: typing.SupportsFloat | typing.SupportsIndex,
    ) -> None:
        """
        Map values from another image.
        """
    def mean_wide(
        self,
        width: typing.SupportsInt | typing.SupportsIndex = 0,
        noise_val: typing.SupportsInt | typing.SupportsIndex = 4,
        average: typing.SupportsInt | typing.SupportsIndex = 0,
        delta: typing.SupportsInt | typing.SupportsIndex = 20,
        iterations: typing.SupportsInt | typing.SupportsIndex = 15,
        smooth_image: str = "",
    ) -> bool:
        """
        computes a background image, used to correct for lens artifacts
        """
    def median_filter(self) -> None:
        """
        Apply a 1+6-neighbour median filter.
        """
    def median_x(self) -> None:
        """
        Apply median filter with kernel size of 1 voxels in x-direction
        """
    def median_y(self) -> None:
        """
        Apply median filter with kernel size of 1 voxels in y-direction
        """
    def median_z(self) -> None:
        """
        Apply median filter with kernel size of 1 voxels in z-direction
        """
    def mode26(self, arg0: typing.SupportsInt | typing.SupportsIndex) -> None: ...
    def mode6(self, n_same_neighbors: typing.SupportsInt | typing.SupportsIndex) -> int:
        """
        Apply mode filter based on nearest 6 neighbor voxels.
        """
    def not_(self, image2: VxlImgU8) -> None:
        """
        Voxel-by-voxel inplace NOT operation, alias for img = img & !img2.
        """
    def or_(self, image2: VxlImgU8) -> None:
        """
        Voxel-by-voxel inplace OR operation.
        """
    def otsu_threshold(
        self,
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = 256,
    ) -> typing.Annotated[list[float], "FixedSize(5)"]: ...
    def paint(self, shape: shape) -> None:
        """
        Paint (set values of) a shape into the image.
        """
    def paint_add(self, shape: shape) -> None:
        """
        Add (+) a shape's value to the image.
        """
    def paint_add_after(self, shape: shape) -> None:
        """
        Add (+) a shape's value after the shape (plane...)
        """
    def paint_add_before(self, shape: shape) -> None:
        """
        Add (+) a shape's value before the shape (plane...)
        """
    def paint_after(self, shape: shape) -> None:
        """
        Paint after the shape (plane...)
        """
    def paint_before(self, shape: shape) -> None:
        """
        Paint before the shape (plane...)
        """
    def plot_all(
        self,
        filename: str = "pltAll",
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = -1000001,
        slice_index: typing.SupportsInt | typing.SupportsIndex = -1000000,
        histogram_bins: typing.SupportsInt | typing.SupportsIndex = 128,
        normal_axis: str = "xyz",
        grey: bool = True,
        color: bool = True,
        histogram: bool = True,
        z_profile: bool = True,
        alpha_image: VxlImgU8 = None,
        alpha_min: typing.SupportsInt | typing.SupportsIndex = 0,
        alpha_max: typing.SupportsInt | typing.SupportsIndex = -1000001,
    ) -> bool:
        """
        Plot all visualizations (Histogram, ZProfile, Slices) with various options, hackish for debugging
        """
    def plot_histogram(
        self,
        filename: str = "aa.svg",
        bins: typing.SupportsInt | typing.SupportsIndex = 128,
        min_val: typing.SupportsFloat | typing.SupportsIndex = 3e38,
        max_val: typing.SupportsFloat | typing.SupportsIndex = -3e38,
    ) -> int: ...
    def plot_slice(
        self,
        filename: str,
        normal_axis: str = "xyz",
        slice_index: typing.SupportsInt | typing.SupportsIndex = -1000000,
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = -1000001,
        color_map: str = "gray",
    ) -> None:
        """
        Save a 2D slice as a PNG image.
        normalAxis can be 'x', 'y', 'z', 'xy', 'xz', 'yz', 'xyz'.
        color_map can be 'gray' or 'RGB'
        """
    def plot_z_profile(
        self,
        filename: str = "aa.svg",
        min_val: typing.SupportsFloat | typing.SupportsIndex = 0,
        max_val: typing.SupportsFloat | typing.SupportsIndex = 255,
    ) -> int: ...
    def point_median032(
        self,
        nAdj0: typing.SupportsInt | typing.SupportsIndex,
        nAdj1: typing.SupportsInt | typing.SupportsIndex,
        lbl0: typing.SupportsInt | typing.SupportsIndex,
        lbl1: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Set voxel value to lbl0/1 if it has more than nAdj0/1 neighbours with value lbl0/1, in its 6+26 nearest voxels
        """
    def print_info(self) -> None: ...
    def range_to(
        self,
        min: typing.SupportsInt | typing.SupportsIndex,
        max: typing.SupportsInt | typing.SupportsIndex,
        val: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Set values in range [min, max] to val.
        """
    def read_ascii(self, filename: str, n_skip_bytes: typing.SupportsInt | typing.SupportsIndex = 0) -> bool:
        """
        Read image data from an ASCII file.
        """
    def read_bin(
        self,
        filename: str,
        n_skip_bytes: typing.SupportsInt | typing.SupportsIndex = 0,
        max_nz: typing.SupportsInt | typing.SupportsIndex = -1,
    ) -> None:
        """
        Read image data from  a .raw, .raw/.raw.gz, or reset and read from a .am or .tif file.
        """
    def read_from_float(
        self,
        filename: str,
        scale: typing.SupportsFloat | typing.SupportsIndex = 1.0,
        shift: typing.SupportsFloat | typing.SupportsIndex = 0.0,
    ) -> bool: ...
    def read_from_header(self, filename: str, max_nz: typing.SupportsInt | typing.SupportsIndex = -1) -> None:
        """
        Reset and read image from file.
        """
    def redirect(self, arg0: str) -> None:
        """
        Swap X axis with the given axis (y or z).
        """
    def replace_by_image_range(
        self,
        min_val: typing.SupportsFloat | typing.SupportsIndex,
        max_val: typing.SupportsFloat | typing.SupportsIndex,
        image_file: str,
    ) -> None: ...
    def replace_range(
        self,
        min: typing.SupportsInt | typing.SupportsIndex,
        max: typing.SupportsInt | typing.SupportsIndex,
        val: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        Replace values in range [min, max] with val.
        """
    def replace_range_by_image(
        self,
        min_val: typing.SupportsFloat | typing.SupportsIndex,
        max_val: typing.SupportsFloat | typing.SupportsIndex,
        image_file: str,
    ) -> None: ...
    def resample(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> None:
        """
        Resample image by factor f, using averaging (downsampling, f>1) or nearest when upsampling (f<1)
        """
    def resample_max(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> VxlImgU8:
        """
        Downsample the image, setting voxel values to maximum of original encompassing voxel values.
        """
    def resample_mean(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> VxlImgU8:
        """
        Resample the image using mean interpolation.
        """
    def resample_mode(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> VxlImgU8:
        """
        Downsample the image, setting voxel values to mode of original encompassing voxel values.
        """
    def rescale_values(
        self, min: typing.SupportsInt | typing.SupportsIndex, max: typing.SupportsInt | typing.SupportsIndex
    ) -> None:
        """
        Rescale image values to [min, max].
        """
    def reslice_z(self, factor: typing.SupportsFloat | typing.SupportsIndex) -> VxlImgU8:
        """
        Reslice along the Z axis.
        """
    def scaled_diff(
        self,
        image2: VxlImgU8,
        scaletype: str = "log",
        shift3: typing.SupportsFloat | typing.SupportsIndex = 1.0,
        scale3: typing.SupportsFloat | typing.SupportsIndex = 1.0,
        shift1: typing.SupportsFloat | typing.SupportsIndex = 0.0,
        scale1: typing.SupportsFloat | typing.SupportsIndex = 1.0,
        shift2: typing.SupportsFloat | typing.SupportsIndex = 0.0,
        scale2: typing.SupportsFloat | typing.SupportsIndex = 1.0,
    ) -> None:
        """
        Calculate difference between two images, linear or logarithmic scale
        """
    def segment(
        self,
        n_segments: typing.SupportsInt | typing.SupportsIndex = 2,
        thresholds: collections.abc.Sequence[typing.SupportsInt | typing.SupportsIndex] = [],
        min_sizes: collections.abc.Sequence[typing.SupportsInt | typing.SupportsIndex] = [],
        smooth_image: str = "",
        noise_val: typing.SupportsFloat | typing.SupportsIndex = 16.0,
        resolution_sq: typing.SupportsFloat | typing.SupportsIndex = 2.0,
        write_dumps: typing.SupportsInt | typing.SupportsIndex = 0,
    ) -> bool: ...
    def segment2(
        self,
        thresholds: collections.abc.Sequence[typing.SupportsInt | typing.SupportsIndex] = [],
        min_sizes: collections.abc.Sequence[typing.SupportsInt | typing.SupportsIndex] = [],
        noise_val: typing.SupportsFloat | typing.SupportsIndex = 2.0,
        local_factor: typing.SupportsFloat | typing.SupportsIndex = 0.05,
        flatnes: typing.SupportsFloat | typing.SupportsIndex = 0.1,
        effective_resolution: typing.SupportsFloat | typing.SupportsIndex = 2.0,
        gradient_factor: typing.SupportsFloat | typing.SupportsIndex = 0.0,
        kernel_radius: typing.SupportsInt | typing.SupportsIndex = 2,
        n_iterations: typing.SupportsInt | typing.SupportsIndex = 13,
        write_dumps: typing.SupportsInt | typing.SupportsIndex = 0,
    ) -> bool: ...
    def shrink0(self) -> None:
        """
        Shrink pore phase (voxel values of 0).
        """
    def shrink_box(self, num_layers: typing.SupportsInt | typing.SupportsIndex) -> None:
        """
        Shrink the image boundaries, decreasing its size by the given num_layers in all directions
        """
    def smooth_bilateral(
        self,
        iterations: typing.SupportsInt | typing.SupportsIndex = 1,
        kernel_radius: typing.SupportsInt | typing.SupportsIndex = 1,
        sigma_val: typing.SupportsFloat | typing.SupportsIndex = 16.0,
        sharpness: typing.SupportsFloat | typing.SupportsIndex = 0.1,
    ) -> bool:
        """
        bilateral smoothing filter
        """
    def threshold101(
        self, min: typing.SupportsInt | typing.SupportsIndex, max: typing.SupportsInt | typing.SupportsIndex
    ) -> None:
        """
        Apply a threshold to binarize the image, set voxel-values to convert to 0 in between the min and max thresholds and 1 outside of it
        """
    def to_foam(self) -> None:
        """
        Convert image to OpenFOAM mesh
        """
    def to_foam_par(
        self, n_par: image3kit._core.sirun.int3 = ..., reset_x0: bool = False, keep_bcs: bool = False
    ) -> None:
        """
        Convert image to a parallel OpenFOAM mesh
        """
    def to_foam_par_incremental(self, n_par: image3kit._core.sirun.int3 = ..., reset_x0: bool = False) -> None:
        """
        Convert image to a parallel OpenFOAM mesh sequentially (one processor mesh at a time)
        """
    def to_surf_mesh(self, inp: dict = {}, outputSurface: str = "surface.obj") -> None:
        """
        Convert image to surface mesh
        """
    def variance(
        self,
        min_val: typing.SupportsInt | typing.SupportsIndex = 0,
        max_val: typing.SupportsInt | typing.SupportsIndex = 255,
    ) -> float:
        """
        Set outer tubing of a circular core-holder image to fill_val
        """
    def write(self, filename: str) -> None:
        """
        Write the image to a file (.mhd, .raw, .ra.gz formats).
        """
    def write_8bit(
        self,
        filename: str,
        min: typing.SupportsFloat | typing.SupportsIndex = 0.0,
        max: typing.SupportsFloat | typing.SupportsIndex = -0.5,
    ) -> None:
        """
        Write as 8-bit image scaled between min(=>0) and max(=>255).
        """
    def write_a_connected_void_voxel(self, filename: str) -> None:
        """
        Write a specific connected pore voxel to file.
        """
    def write_contour(self, out_surf: str) -> None:
        """
        Write contour extraction to a surface file.
        """
    def write_no_header(self, filename: str) -> None:
        """
        Write the raw image data without a header.
        """
    def xor_(self, image2: VxlImgU8) -> None:
        """
        Voxel-by-voxel inplace XOR operation.
        """
    @property
    def data(self) -> typing.Any:
        """
        Get the raw data buffer as a numpy array.
        """
    @property
    def nx(self) -> int: ...
    @property
    def ny(self) -> int: ...
    @property
    def nz(self) -> int: ...
    @property
    def origin(self) -> tuple:
        """
        Get/set the origin value (x0, y0, z0).
        """
    @origin.setter
    def origin(self, arg1: image3kit._core.sirun.dbl3) -> None: ...
    @property
    def shape(self) -> tuple: ...
    @property
    def spacing(self) -> tuple:
        """
        Get/set the voxel size (dx, dy, dz).
        """
    @spacing.setter
    def spacing(self, arg1: image3kit._core.sirun.dbl3) -> None: ...

class cube(shape):
    def __init__(self, p1: tuple, size: tuple, val: typing.SupportsInt | typing.SupportsIndex) -> None:
        """
        p1: first point, size: size of cuboid sides, val: paint value
        """

class cylinder(shape):
    def __init__(
        self,
        p1: tuple,
        p2: tuple,
        r: typing.SupportsFloat | typing.SupportsIndex,
        val: typing.SupportsInt | typing.SupportsIndex,
    ) -> None:
        """
        p1: first point on axis, p2: second point on axis, r: radius, val: paint value
        """

class shape:
    pass

class sphere(shape):
    def __init__(
        self,
        arg0: tuple,
        arg1: typing.SupportsFloat | typing.SupportsIndex,
        arg2: typing.SupportsInt | typing.SupportsIndex,
    ) -> None: ...

class voxelImageTBase:
    pass

def connected_components(
    arg0: VxlImgU8, arg1: typing.SupportsFloat | typing.SupportsIndex, arg2: typing.SupportsFloat | typing.SupportsIndex
) -> VxlImgI32: ...
def read_image(filename: typing.Any, max_nz: typing.SupportsInt | typing.SupportsIndex = -1) -> voxelImageTBase:
    """
    Global helper to read an image from a file, use VxlImg..() constructors if you know image type.
    """
