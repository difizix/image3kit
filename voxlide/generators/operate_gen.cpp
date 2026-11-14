// Halide ports of Python `operate` for 8/16-bit images (voxlib/voxelImageI.h operateUnary,
// operateImg, operateScalar). The operation character arrives as an int32 `op`; the double
// arithmetic of voxlib is reproduced in strict double (FP64) and clamped to [0, maxT]; the
// conversion to the voxel type goes through int32 (C++ truncation).

#include <cmath>
#include "vxhl_gen.h"

namespace {

using namespace Halide;

Expr dbl(double v) { return Internal::make_const(Float(64), v); }

struct Helpers {
    Type t;
    Target target;
    Expr div(Expr a, Expr b) const { return vxhl_gen::div64(a, b, target); }
    Expr maxVal() const { return dbl(std::ldexp(1.0, t.bits()) - 1.0); }  // unsigned 8/16-bit voxels
    //! T(std::max(0.0, std::min(v, maxVal)))
    Expr clampToT(Expr v) const { return cast(t, cast<int32_t>(max(dbl(0.0), min(v, maxVal())))); }
};

// ---- unary: '~' '!' '-'
class OperateUnary : public Generator<OperateUnary> {
public:
    VXHL_STENCIL_INPUTS
    Input<int32_t> op{"op"};
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};
    void generate() {
        const Helpers h{input.type(), get_target()};
        const Expr v = input(x, y, z);
        const Expr bitNot = cast(h.t, ~cast<int32_t>(v));  // C++: ~ promotes to int, truncated to T
        const Expr logNot = select(v == cast(h.t, 0), cast(h.t, 1), cast(h.t, 0));
        const Expr neg = h.clampToT(dbl(0.0) - cast<double>(v));
        output(x, y, z) = select(op == int32_t('~'), bitNot, select(op == int32_t('!'), logNot, neg));
    }
    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), input.type()); }
};

// ---- with another image: '+' '-' '*' '/' '&' '|' '%' and shift
class OperateImg : public Generator<OperateImg> {
public:
    VXHL_STENCIL_INPUTS
    Input<Buffer<void, 3>> img2{"img2"};  // Aux image
    Input<int32_t> op{"op"};
    Input<double> shift{"shift"};
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};
    void generate() {
        const Helpers h{input.type(), get_target()};
        const Expr m = input(x, y, z), d = img2(x, y, z);
        const Expr md = cast<double>(m), dd = cast<double>(d);
        const Expr sFact = select(shift != dbl(0.0), shift + dbl(1.0), dbl(1.0));
        const Expr add = h.clampToT(md + (dd - shift));
        const Expr sub = h.clampToT(md - (dd - shift));
        const Expr mul = h.clampToT(md * dd * sFact);
        const Expr div = h.clampToT(select(dd != dbl(0.0), h.div(md, dd) * sFact, dbl(0.0)));
        const Expr band = cast(h.t, cast<int32_t>(m) & cast<int32_t>(d));
        const Expr bor = cast(h.t, cast<int32_t>(m) | cast<int32_t>(d));
        const Expr rem = select(d != cast(h.t, 0), cast(h.t, cast<int32_t>(m) % max(cast<int32_t>(d), 1)), cast(h.t, 0));
        output(x, y, z) = select(op == int32_t('+'), add, select(op == int32_t('-'), sub, select(op == int32_t('*'), mul, select(op == int32_t('/'), div,
                          select(op == int32_t('&'), band, select(op == int32_t('|'), bor, rem))))));
    }
    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), input.type()); }
};

// ---- with a scalar: '=' '+' '-' '*' '/' '&' '|' '%' 'b' 'e'
class OperateScalar : public Generator<OperateScalar> {
public:
    VXHL_STENCIL_INPUTS
    Input<int32_t> op{"op"};
    Input<double> val{"val"};
    Output<Buffer<void, 3>> output{"output"};
    Var x{"x"}, y{"y"}, z{"z"};
    void generate() {
        const Helpers h{input.type(), get_target()};
        const Expr m = input(x, y, z), md = cast<double>(m);
        const Expr valT = h.clampToT(val);  // T(clamp(val)): '=' fill value, '&' '|' '%' operand, 'b' 'e' bounds
        const Expr valI = cast<int32_t>(valT);
        const Expr set = valT;
        const Expr add = h.clampToT(md + val);
        const Expr sub = h.clampToT(md - val);
        const Expr mul = h.clampToT(md * val);
        const Expr div = h.clampToT(select(val != dbl(0.0), h.div(md, val), dbl(0.0)));
        const Expr band = cast(h.t, cast<int32_t>(m) & valI);
        const Expr bor = cast(h.t, cast<int32_t>(m) | valI);
        const Expr rem = select(valI != 0, cast(h.t, cast<int32_t>(m) % max(valI, 1)), m);  // valInt == 0: unchanged
        const Expr lo = max(valT, m), hi = min(valT, m);
        output(x, y, z) = select(op == int32_t('='), set, select(op == int32_t('+'), add, select(op == int32_t('-'), sub, select(op == int32_t('*'), mul,
                          select(op == int32_t('/'), div, select(op == int32_t('&'), band, select(op == int32_t('|'), bor,
                          select(op == int32_t('%'), rem, select(op == int32_t('b'), lo, hi)))))))));
    }
    void schedule() { vxhl_gen::schedule_stencil(gnx, output, get_target(), input.type()); }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(OperateUnary, operateUnary)
HALIDE_REGISTER_GENERATOR(OperateImg, operateImg)
HALIDE_REGISTER_GENERATOR(OperateScalar, operateScalar)
