#pragma once


#include "InputFile.h"
#include "globals.h"
#include "voxelImage.h"
#include <algorithm>

using namespace std;

struct medbal;

struct strip {
  strip(const strip&) = delete;
  strip& operator=(const strip&) = default; // check for mb_a, mb_b
  strip() :  iCh0(0), iCh1(0), mb_a(nullptr), mb_b(nullptr) { }
  strip(int ibgn, unsigned char value) : i0(ibgn), vv(value), iCh0(0), iCh1(0), mb_a(nullptr), mb_b(nullptr) { }

  int            i0; // i0
  int            i1()  const { return i0+1; }
  int            nSkip;
  unsigned char  vv;
  unsigned char  iCh0; // flag indicating recent change or being adjacent to throat surface
  unsigned char  iCh1; // flag indicating recent change or being adjacent to throat surface
  medbal*       mb_a; // set:mb_a=
  medbal*       mb_b; // set:mb_a=
};

struct strips  {
  strips(): sts(nullptr), cnt(0) {};

  strips(const strips& c) : sts(nullptr), cnt(c.cnt) {
    if (c.sts) {
      sts = new strip[cnt+1];
      std::copy(c.sts, c.sts+cnt+1, sts);
    }
  }

  strips& operator =(const strips& c) {
    dAsrt(sts==nullptr);
    dAsrt(c.sts);
    cnt = c.cnt;
    sts = new strip[cnt+1];
    std::copy(c.sts, c.sts+cnt+1, sts);
    return *this;
  }

  strips(strips&& c) noexcept : sts(c.sts), cnt(c.cnt) {
    c.sts = nullptr; c.cnt = 0;
  }

  strips& operator=(strips&& c) {
    dAsrt(sts==nullptr);
    dAsrt(c.sts);
    sts = c.sts;
    cnt = c.cnt;
    c.sts = nullptr;
    c.cnt = 0;
    return *this;
  }

  void reSize(int size) { dAsrt(!sts);  sts = new strip[size+1]; cnt=size; }

  ~strips() { if(sts) delete[] sts; sts = nullptr; }

  // const voxel* voxl(int i) const { // obsolete
    // for (int p=0; p<cnt; ++p)  if (i>=sts[p].i0 && i<sts[p+1].i0)  return sV0_+i-sts[p].nSkip;
    // return nullptr; }

  const strip* stp0(int i) const {
    auto it = std::upper_bound(sts, sts + cnt + 1, i,
      [](int val, const strip& s) { return val < s.i0; });
    if (it == sts) return sts + cnt;
    return it - 1;
  }
  strip* stpCh(int i) { return const_cast<strip*>(stp0(i)); }

  strip*       sts; // strips
  int          cnt; // count
};


class VxlStrips  {
public:

  VxlStrips(const InputFile& inp);

  void setImageInfo(const InputFile& inp, const voxelImage& VImage);
  void createStripsX(const voxelImage& VImage);

public:

  int                    nx, ny, nz;
  int                    nBP6;
  double                 vxlSize;
  dbl3                   X0;
  string                 imgfrmt;
  long long              nInside;
  constexpr static int   _1ExtraSegX = 1;

  stvec<stvec<strips>>   segXs_;

  stvec<int>             segValues;
  stvec<int2>            rockTypes_;
  stvec<size_t>          nVxlVs;
};

class DistMap { //: baseof MedSurf
public:

  DistMap(const InputFile& inp, VxlStrips& cfg, size_t ivVal);

  friend class VxlStrips; // debugging vtuWriteBSurfAnalyseCrnrs()

  float calc_distmapf(dbl3 mb, int3& dj);
  float calc_distmap(int i, int j, int k, int3& dj);


  bool isInside(int i, int j, int k) const {  return (i>=0 && j>=0 && k>=0 && i<nx && j<ny && k<nz);  }

  bool isJInside(int j) const {  return (j>=0 && j<ny);  }

  bool isInside(int i) const {  return (0<=i && i<nx); }

  const strip& segX0(int i, int j, int k) const { // 0 is for zero-based indexing
    if (i<0 || j<0 || k<0 || i>=nx || j>=ny || k>=nz)  return invalidSeg;
    return  *segXs_[k+1][j+1].stp0(i);
  }

  int nx, ny, nz;
  const unsigned char    inVxVal_;   // DAR:;

  double _clipROutx;
  double _clipROutyz;

  int    _nRSmoothing;

  stvec<stvec<strips>>&  segXs_;
  strip                  invalidSeg;

};

voxelImage distMapExtrude(const voxelImage& VImage, const InputFile& inp, double offsetR, double scaleR, double powerR);
