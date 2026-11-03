#pragma once
//! voxlide: Halide-accelerated versions of voxlib ops, with voxlib as the fallback.
//! Each vxhl::<op> takes the same arguments as voxlib's ::<op> and gives the same result;
//! the backend comes from set_backend(), else $IMAGE3KIT_BACKEND, else Auto.
//! See docs/plans/parallelism_shared.md.

#include <array>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <string>
#include <vector>
#include "voxelImage.h"
#include "shapeToVoxel.h"

namespace vxhl {

enum class Backend {
    Auto,       //!< GPU if usable, else Halide CPU, else voxlib; a failing GPU kernel falls back (with a warning)
    Voxlib,     //!< reference OpenMP C++ implementation (voxlib)
    HalideCpu,  //!< Halide AOT vectorised + parallel CPU schedule
    Gpu         //!< Halide AOT GPU schedule (Vulkan; Metal on macOS)
};

//! Parse "auto", "voxlib"/"cpu"/"omp", "halide"/"halide_cpu", "gpu"/"vulkan"/"metal"; throws on others.
Backend parse_backend(const std::string& name);
//! Canonical lower-case name: "auto", "voxlib", "halide", "gpu".
const char* backend_name(Backend b);

//! Process-wide backend for all vxhl:: ops (not thread-local).
void    set_backend(Backend b);
Backend get_backend();

//! Backends usable on this machine and build: voxlib always, then halide, gpu.
std::vector<Backend> available_backends();

//! Max bytes per piece when an image is processed in z-slabs; 0 restores the default
//! (1 GiB on CPU, 512 MiB on GPU). Images larger than this are processed slab by slab.
void set_max_slab_bytes(std::size_t bytes);

//! An image that stays on the GPU between vxhl:: ops (docs/plans/parallelism_shared.md §2.7):
//!     vxhl::DeviceImage<uint8_t> d(img);           // no copy to the GPU yet
//!     vxhl::mode26(d, 2); vxhl::median(d, 3);      // on the GPU: one upload, no downloads
//!     double m = vxhl::meanDbl(d);                 // reductions download only their results
//!     img = d.host();                              // one download
//! Every ported op has a DeviceImage overload. On the GPU backend, with an image that fits in one
//! piece (max_slab_bytes), ops pass the device buffer on without host transfers; otherwise
//! (CPU backends, slabs, voxlib fallback, growBox) they use the host copy, synced as needed.
//! The host copy is only up to date through host()/host_mut(): never read it any other way.
template <typename T>
class DeviceImage {
public:
    explicit DeviceImage(VoxelImageT<T> img);
    DeviceImage(DeviceImage&&) noexcept;
    DeviceImage& operator=(DeviceImage&&) noexcept;
    ~DeviceImage();

    const VoxelImageT<T>& host() const;  //!< the current image; downloads if the device copy is newer
    VoxelImageT<T>& host_mut();          //!< same, to modify on the host: re-uploaded by the next GPU op
    int3 size3() const { return img_.size3(); }
    void growBox(int n);    //!< vxhl::growBox: on the device when the image is there
    void shrinkBox(int n);  //!< vxhl::shrinkBox

    struct Transfers {
        long long up = 0, down = 0;  //!< whole-image host→device / device→host copies so far
    };
    Transfers transfers() const;

    struct State;  //!< voxlide runtime state (device mirror); runtime.h
    State& state() const { return *st_; }
    VoxelImageT<T>& unsynced() const { return img_; }  //!< voxlide internal: host storage, not synced

private:
    mutable VoxelImageT<T> img_;
    std::unique_ptr<State> st_;
};

template <typename I> struct voxel_of;
template <typename T> struct voxel_of<VoxelImageT<T>> { using type = T; };
template <typename T> struct voxel_of<DeviceImage<T>> { using type = T; };
//! The voxel type of an image argument (VoxelImageT<T> or DeviceImage<T>).
template <typename I> using voxel_t = typename voxel_of<std::remove_const_t<I>>::type;
template <typename I> concept Image = requires { typename voxel_of<std::remove_const_t<I>>::type; };

// ---- ops: one declaration per ported op: voxlib's arguments, with the image as I& (Image I: a
// VoxelImageT<T> or a DeviceImage<T>) and voxlib's T as voxel_t<I>.
template <Image I> void mode26(I& vImg, short minDif);
//! Returns the number of changed voxels (voxlib: Python mode6).
template <Image I> long long modeNSames(I& vImage, short nSameNei, bool verbose = false);
//! segment2's shrink: voxels of seged in [min, maks] next to a different value take vImg's value.
template <Image I, Image J> void shrink(I& seged, voxel_t<I> min, voxel_t<I> maks, const J& vImg);
//! Voxels in [minvi, maxvi] become midvi (Python replace_range / range_to).
template <Image I> void replaceRange(I& vImage, voxel_t<I> minvi, voxel_t<I> maxvi, voxel_t<I> midvi);
//! vImg.zeroGrad(nLayers): the outer nLayers layers take the nearest interior layer's values.
template <Image I> void zeroGrad(I& vImg, int nLayers);
//! segment2: grad < avgGrad -> 10, else scaled by 10/avgGrad (integer types; floats use voxlib).
template <Image I> void scaleGradient(I& grad, double avgGrad);
//! (128 + Σ v) / (1 + #v) over voxels v >= minV, in integer arithmetic (segment2).
template <Image I> int average2p(const I& vm, voxel_t<I> minV = 2);
//! Relabel by neighbour counts (labels lbl0/lbl1 within the 27+6 voxels); voxlib/voxelImageI.h PointMedian032.
template <Image I> void median032(I& vImg, int nAdj0, int nAdj1, voxel_t<I> lbl0, voxel_t<I> lbl1);
//! voxels equal to lblFrm with more face neighbours lblTo than lblFrm (+ndif) become lblTo.
template <Image I> void FaceMedGrowToFrom(I& vImg, voxel_t<I> lblTo, voxel_t<I> lblFrm, int ndif = 0);
//! Face-neighbour (sum of values) median of a 0/1 image; returns the number of changed voxels.
template <Image I> std::size_t median06(I& vImg, int nAdj0, int nAdj1);
//! Voxels next to a vl voxel become vl.
template <Image I> void growLabel(I& vImg, voxel_t<I> vl);
//! Pore (value 0) erosion/dilation by one voxel with voxlib's face/edge rules (Python grow0 / shrink0).
template <Image I> void growPore(I& vImg);
//! voxlib grow(vImg, midv, minv, maxv) (voxelImageML.h): interior voxels in [minv, maxv] next to a midv voxel become midv.
template <Image I> void grow(I& vImg, voxel_t<I> midv, voxel_t<I> minv, voxel_t<I> maxv);
//! VoxLib::meanWide (Python mean_wide): u8/u16/i32 on DeviceImages with host steps for the dump*.raw files and keepLargest; float runs voxlib.
template <Image I> bool meanWide(I& vImg, int nW, int noisev, int avg, int delta, int nItrs, std::string smoothImg);
//! Building blocks of meanWide (internal, not bound in Python): the border planes after growBox(nW+5), ::mean (3x3x3), v = value
//! where g > thresh, and one meanWide iteration (voxlib/voxelImageML.h meanWide(vImg, nW, lowerB, upperB, grad, gradthresh)).
template <Image I> void padEdges(I& vImg);
template <Image I> void meanFilter(I& vImg);
template <Image I, Image J> void setWhereAbove(I& vImg, const J& g, voxel_t<I> thresh, voxel_t<I> value);
template <Image I, Image J> void meanWideStep(I& vImg, int nW, voxel_t<I> lowerB, voxel_t<I> upperB, const J& g, voxel_t<I> gradthresh);
//! VoxLib::growOutSideValue (Python grow_outside_value).
template <Image I> bool growOutSideValue(I& vImg, int vo, int vnew, int nHoleSize);
template <Image I> void shrinkPore(I& vImg);
//! Median of a voxel and its two neighbours along x / y / z; returns a new image (Python median_x/y/z).
template <typename T> VoxelImageT<T> medianx(const VoxelImageT<T>& vImage);
template <typename T> VoxelImageT<T> mediany(const VoxelImageT<T>& vImage);
template <typename T> VoxelImageT<T> medianz(const VoxelImageT<T>& vImage);
template <typename T> void medianx(DeviceImage<T>& vImage);  //!< in place
template <typename T> void mediany(DeviceImage<T>& vImage);
template <typename T> void medianz(DeviceImage<T>& vImage);
//! Chains over the ops above, run on DeviceImages (no host round trips on the GPU between steps):
//! fillHoles: Python fill_holes (small closed holes, voxlib VoxelImageT::fillHoles); _delense032: Python delense032
//! (labels outside the voxel type's range run voxlib).
template <Image I> void fillHoles(I& vImg, int maxHoleRadius);
template <Image I> bool _delense032(I& vImg, int nItrs, int nAdj0, int nAdj1, intOr<voxel_t<I>> lbl0, intOr<voxel_t<I>> lbl1);
//! a = b (data only, same size): the device-side copy chains use.
template <Image I, Image J> void copyFrom(I& a, const J& b);
//! In-place pointwise ops with a second image of the same size and type (a VoxelImageT or DeviceImage).
//! and_: a && b; or_: a || b; xor_: a != b (values differ); not_: a && !b; addEq / subEq: a += b / a -= b in the voxel type.
template <Image I, Image J> void and_(I& a, const J& b);
template <Image I, Image J> void or_(I& a, const J& b);
template <Image I, Image J> void xor_(I& a, const J& b);
template <Image I, Image J> void not_(I& a, const J& b);
template <Image I, Image J> void addEq(I& a, const J& b);
template <Image I, Image J> void subEq(I& a, const J& b);
//! Python `operate` (u8/u16): unary ('~' '!' '-'), with an image ('+' '-' '*' '/' '&' '|' '%' and shift), with a scalar.
template <Image I> void operateUnary(I& m, const std::string& opr);
template <Image I, Image J> void operateImg(I& m, const std::string& opr, const J& img2, double shift);
template <Image I> void operateScalar(I& m, const std::string& opr, double val);
//! v -> (v < bgn || end < v) ? 1 : 0.
template <Image I> void threshold101(I& vImg, voxel_t<I> bgn, voxel_t<I> end);
//! Voxels outside the circle (dir 'z': x,y around X0,Y0; 'x': y,z around X0,Y0) get outVal.
template <Image I> void circleOut(I& vImage, int X0, int Y0, int R, char dir = 'z', voxel_t<I> outVal = maxT(voxel_t<I>));
//! Voxels in [vmin, vmax] take the source image's value; then v -> v·scale + shift if scale > 1e-16 (Python map_from).
//! The source may have another size or origin: that case runs voxlib.
template <Image I, Image J> void mapToFrom(I& vImage, const J& vimg2, voxel_t<I> vmin, voxel_t<I> vmax, double scale = 0, double shift = 0.5);
//! Python paint / paint_add with a shape (sphere, cylinder, cube: GPU and CPU kernels; triangular: Halide CPU only;
//! other shape types run voxlib). paint_before / _after variants stay voxlib: for the shapes Python exposes they change nothing.
template <Image I> void paint(I& vImg, const VoxLib::shape& sh);
template <Image I> void paintAdd(I& vImg, const VoxLib::shape& sh);
// statistics (reductions; voxlib/voxelImageI.h)
template <Image I> double varianceDbl(const I& img, int bgn, int end, bool verbose = false);
template <Image I> double stdevDbl(const I& img, int bgn, int end);
template <Image I> double meanDbl(const I& img);
template <Image I> double minDbl(const I& img);
template <Image I> double maxDbl(const I& img);
template <Image I> double porosityDbl(const I& img, voxel_t<I> threshold = 0);
// ---- outputs of another size (F6): the image is replaced; a DeviceImage stays on the device on the GPU.
//! voxlib VoxelImageT::cropD: keep [frm, to) (to <= 0 counts from the end), plus emptylyrs layers of eLyrsValue around it.
template <Image I> void cropD(I& img, int3 frm, int3 to, int emptylyrs = 0, voxel_t<I> eLyrsValue = 1, bool verbose = false);
//! voxlib VoxelImageT::growBox: nLayers more layers on every side, copies of the boundary layers.
template <Image I> void growBox(I& img, int nLayers);
//! voxlib VoxelImageT::shrinkBox: nLayers fewer layers on every side.
template <Image I> void shrinkBox(I& img, int nLayers);
//! voxlib VoxelImageT::rotate (Python redirect / direction): swap x with y ('y') or z ('z'), or flip x ('-').
template <Image I> void redirect(I& img, char direction);
//! voxlib resampleMean (Python resample / resample_mean) and resliceZ (reslice_z): a new image of another size.
template <Image I> std::remove_const_t<I> resampleMean(const I& img, double factor);
template <Image I> std::remove_const_t<I> resliceZ(const I& img, double factor);
//! A u8 image of the same kind as I (VoxelImageT or DeviceImage).
template <Image I> using MaskOf = std::conditional_t<std::is_same_v<std::remove_const_t<I>, DeviceImage<voxel_t<I>>>,
                                                     DeviceImage<uint8_t>, VoxelImageT<uint8_t>>;
//! voxlib otsu_minAvgThresholdAvgMax(img, minvi, maxvi, skipFrac, maxBins): {min, mean_0, threshold, mean_1, max};
//! the histogram on the device (skipFrac != 0: voxlib on the host).
template <Image I> std::array<double, 5> otsu_minAvgThresholdAvgMax(const I& img, intOr<voxel_t<I>> minvi, intOr<voxel_t<I>> maxvi,
                                                                   double skipFrac = 0., int maxBins = 256);
//! voxlib threshold01_otsu: u8 mask, 0 where v < the Otsu threshold, 1 elsewhere.
template <Image I> MaskOf<I> threshold01_otsu(const I& img, int minvi = 0, int maxvi = maxT(voxel_t<I>), double skipFrac = 0.);
template <typename T> void DeviceImage<T>::growBox(int n) { vxhl::growBox(*this, n); }
template <typename T> void DeviceImage<T>::shrinkBox(int n) { vxhl::shrinkBox(*this, n); }
//! voxlib histogram(img, HistBins{shift, span, nBins}, lo, hi) (no skipFrac margins): counts of the voxels lo <= v < hi per bin; nBins <= 256.
template <Image I> vars<long long> histogram(const I& img, double shift, double span, int nBins, double lo, double hi);
//! One segment2 voting pass (voxlib segmentVote); returns the number of changed voxels. u8/u16.
template <Image I, Image J, Image K>
std::size_t segmentVote(I& vImg, const J& origimg, const K& grad, const vars<voxel_t<I>>& meds, intOr<voxel_t<I>> capmin,
                        intOr<voxel_t<I>> capmax, int krnl, double localF, double flatnes, double gradFactor,
                        double resolutionSqr, double avgGradInv, double noisv);
//! voxlib segment2 (Python segment2), run on DeviceImages: u8/u16.
template <typename T>
bool segment2(VoxelImageT<T>& vImg, int nSegs, vars<intOr<T>> th, vars<int> minSizs, double noisev, double localF,
              double flatnes, double resolution, double gradFactor, int krnl, int nItrs, int writedumps);
template <typename T> VoxelImageT<T> magGradient(const VoxelImageT<T>& vImg, double diffuseLSqr);
template <typename T> void magGradient(DeviceImage<T>& vImg, double diffuseLSqr);  //!< in place
template <typename T> VoxelImageT<T> median(const VoxelImageT<T>& vImg, int nIter = 1);
template <typename T> void median(DeviceImage<T>& vImg, int nIter = 1);  //!< in place
template <Image I> bool smoothBilateral(I& vImg, int nItrs, int kernRad, double sigmavv, double sharpFact);
template <Image I> void bilateralX(I& vImg, int kernRad, int Xstp, float sigmaSqr = 16, float sharpFact = 0.05,
                                   float sigmaGaussSqr = 2., float capmin = 0, float capmax = fmaxT(voxel_t<I>));
template <Image I> void bilateralGauss(I& vImg, int kernRad, float sigmaSqr = 16, float sharpFact = 0.05,
                                       float sigmaGaussSqr = 2., float capmin = 0, float capmax = fmaxT(voxel_t<I>));
template <Image I> bool _bilateralGauss(I& vImg, int nItrs, int kernRad, double sigmavv, double sharpFact, double sigmadd);
template <Image I> bool _bilateralX(I& vImg, int nItrs, int kernRad, int Xstp, double sigmavv, double sharpFact,
                                    double sigmadd);

}  // namespace vxhl
