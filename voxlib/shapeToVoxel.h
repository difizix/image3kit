#pragma once

/*-------------------------------------------------------------------------*\

This file is part of libvoxel, a C++ template library for handelling 3D images.

Developed by:
 - Ali Q Raeini (2010-2022)

\*-------------------------------------------------------------------------*/

#include <cmath>
#include <cassert>
#include <iostream>
#include "voxelImage.h"


namespace VoxLib  {
using namespace std;

struct shape {
 public:
  constexpr static int invalidv = 0x80000000; // 2147483648 largest negative
  const char polyType; // shape // TODO use enum instead (?)
  int insidev=0;

  shape(char _polyType): polyType(_polyType), insidev(0){}
  shape(char _polyType, int _insidev): polyType(_polyType), insidev(_insidev){}

  int isBefore(dbl3) const { return false; };
  int isAfter(dbl3) const { return false; };

  template<typename T, class Shape>
  static void setIn(voxelImageT<T>& vImg, const Shape& shp) {
    (cout <<__FUNCTION__<<'_'<<shp.polyType<<": "<<shp.insidev<<",  ").flush();
    dbl3 xmin = vImg.X0(),  dx = vImg.dx();
    forAllkji_(vImg) {
      int vv = shp.value(dbl3(i+0.5,j+0.5,k+0.5)*dx+xmin);
      if(vv!=shape::invalidv) vImg(i,j,k)  = vv;
    }
  }
  template<typename T, class Shape>
  static void addTo(voxelImageT<T>& vImg, const Shape& shp) {
    (cout <<__FUNCTION__<<'_'<<shp.polyType<<": "<<shp.insidev<<",  ").flush();
    dbl3 xmin = vImg.X0(),  dx = vImg.dx();
    forAllkji_(vImg)
      if(int vv = shp.value(dbl3(i+0.5,j+0.5,k+0.5)*dx+xmin); vv!=shape::invalidv) vImg(i,j,k) += vv;
  }
  template<typename T, class Shape>
  static void setBefor(voxelImageT<T>& vImg, const Shape& shp) {
    (cout <<__FUNCTION__<<'_'<<shp.polyType<<": "<<shp.insidev<<",  ").flush();
    dbl3 xmin = vImg.X0(),  dx = vImg.dx();
    forAllkji_(vImg)
      if(shp.isBefore(dbl3(i+0.5,j+0.5,k+0.5)*dx+xmin))  vImg(i,j,k)  = shp.insidev;
  }
  template<typename T, class Shape>
  static void setAfter(voxelImageT<T>& vImg, const Shape& shp) {
    (cout <<__FUNCTION__<<'_'<<shp.polyType<<": "<<shp.insidev<<",  ").flush();
    dbl3 xmin = vImg.X0(),  dx = vImg.dx();
    forAllkji_(vImg)
      if(shp.isAfter (dbl3(i+0.5,j+0.5,k+0.5)*dx+xmin))  vImg(i,j,k) = shp.insidev;
  }
  template<typename T, class Shape>
  static void addBefor(voxelImageT<T>& vImg, const Shape& shp) {
    (cout <<__FUNCTION__<<'_'<<shp.polyType<<": "<<shp.insidev<<",  ").flush();
    dbl3 xmin = vImg.X0(),  dx = vImg.dx();
    forAllkji_(vImg)
      if(shp.isBefore(dbl3(i+0.5,j+0.5,k+0.5)*dx+xmin))  vImg(i,j,k) += shp.insidev;
  }
  template<typename T, class Shape>
  static void addAfter(voxelImageT<T>& vImg, const Shape& shp) {
    (cout <<__FUNCTION__<<'_'<<shp.polyType<<": "<<shp.insidev<<",  ").flush();
    dbl3 xmin = vImg.X0(),  dx = vImg.dx();
    forAllkji_(vImg)
      if(shp.isAfter(dbl3(i+0.5,j+0.5,k+0.5)*dx+xmin))  vImg(i,j,k) += shp.insidev;
  }
};


class cylinder : public shape {
  dbl3 p1, p2;
  double r2mag2p12;
 public:
  cylinder(stringstream & ins) : shape(/*polyType*/'c') {
    //http://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html
    double rr;
    ins>>p1 >> p2 >>rr >>insidev;
    r2mag2p12=magSqr(p2-p1)*rr*rr;
    cout <<"cylinder  p1="<<p1<<",   p2="<<p2<<",   r="<<rr<<"   value="<<insidev;
  };
  cylinder(dbl3 _p1, dbl3 _p2, double rr, int _insidev): shape(/*polyType*/'c',_insidev),
      p1(_p1), p2(_p2), r2mag2p12(magSqr(_p2-_p1)*rr*rr)
    {}

  int value(dbl3 ij) const { return ( magSqr((ij-p1)^(ij-p2)) <= r2mag2p12 )  ?  insidev : shape::invalidv; }
 };

class triangular : public shape {
   //        ..
   //      ./| \.
   //    ./  |  \.
   //  ./    h   \.
   // /_L1__po_L2__\.
  dbl3 po;
  double L1, L2, h_, hInv, LtInv, ch; // lt is throat length, ch is contraction ratio (<1)
 public:
  triangular(stringstream & ins) : shape(/*polyType*/'t') {
    //http://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html
    ins>>po >> L1 >>L2 >>h_ >>LtInv >>ch >>insidev;
    cout <<"cylinder po="<<po<<", L1="<<L1<<" L2="<<L2<<" h="<<h_<<" Lt="<<LtInv<<" rt/rp="<<ch<<"   value="<<insidev;
    ch = h_*(1.-1./ch);
    hInv = 1./h_;
    LtInv = 1./LtInv;
    L1 *= -1;
  }
  int value(dbl3 ij) const {
    double dx = LtInv*(ij.x-po.x);
    if (ij.y<po.y-h_) return shape::invalidv;
    if (ij.y>po.y) return shape::invalidv;
    double dy = std::cos((0.5*PI)*dx);
    dy =  hInv*(po.y - ch*dy*dy - ij.y);
    double dz=ij.z-po.z;
    if (dz <  L1*dy) return shape::invalidv;
    if (dz >  L2*dy) return shape::invalidv;
    return insidev; }
};


class layer : public shape {
  double p1, p2;  //!< p1: location on bottom plane. p2: location on top plane.
  dbl3 nrm; //!< normal vector to planes, along which locations are recorded
  public:
  layer(stringstream & ins): shape(/*polyType*/'l'), nrm(0,1,0) {
    dbl3 Pl; //point through bottom plate,
    ins>>Pl>>nrm>>insidev; p2=mag(nrm); nrm/=p2;  p1=Pl&nrm; p2+=p1;
    cout <<"layer  p1="<<Pl<<",  height="<<p2-p1<<",  normal="<<nrm<<",  insidev="<<insidev;
  }
  int value(dbl3 ij)    const {  return  (ij&nrm)>=p1 && (ij&nrm)<p2  ? insidev : shape::invalidv;  }
  int isBefore(dbl3 ij) const {  return  (ij&nrm) < p1; }// used for network cut
  int isAfter (dbl3 ij) const {  return  (ij&nrm) >= p2;  }
};



class gridLines : public shape {
  double spacing, hInvThic, bgn, end; dbl3 nrm;

 public:
  gridLines(stringstream & ins): shape(/*polyType*/'g'),
      spacing(10), hInvThic(2), bgn(-1000), end(1000), nrm(1,0,0)
  {
    ins>>spacing>>hInvThic>>nrm>>bgn>>end>>insidev;
    cout<<"gridLines  spacing="<<spacing<<"thick="<<hInvThic<<",  norm="<<nrm
        <<",  span"<<bgn<<" "<<end<<",  in_v="<<insidev;
    hInvThic=1./hInvThic;
    spacing*=hInvThic;
    end-=bgn;
    bgn*=hInvThic;
    end*=hInvThic;
  };
  int value(dbl3 ij)    const {
    double dist = (ij&nrm)*hInvThic-bgn;
    return 0.<dist && dist<end && (int(dist)%int(spacing)==0) ? insidev : shape::invalidv; }

    // mod (dist/thic, spac/thic)

};


 // outdated
class paraPlates : public shape {
  /// input mX dy mZ y1, mX and mZ are slopes, dy is separation, y1 is elevation of bottom plane
  dbl3 p1, p2;

  public:
  paraPlates(stringstream & ins): shape(/*polyType*/'f') {
    ins>>p1;   p2 = p1;   p1.y=0.; ins>>p1.y;
    cout <<"paraPlates slope1,mX="<<p1.x<<"    separation,dy="<<p2.y<<"   slope2,mZ="<<p1.z<<"   shift, y1:"<<p1.y;
  };
  int value(dbl3 ij)   const  {  return ( ij.y > p1.x*ij.x+p1.z*ij.z+p1.y
                                       && ij.y < p2.x*ij.x+p2.z*ij.z+p2.y )  ? insidev : shape::invalidv;  }
  int isBefore(dbl3 ij) const {  return ( ij.y < p2.x*ij.x+p2.z*ij.z+p2.y ); }// used for network cut
  int isAfter (dbl3 ij) const {  return ( ij.y > p1.x*ij.x+p1.z*ij.z+p1.y );  }
};

class kube : public shape {
  /// TODO  generalize to hexahedron, TODO optimize
  dbl3 p1,p2;
 public:
  kube(stringstream & ins): shape(/*polyType*/'k') {
    ins>>p1>>p2>>insidev;
    cout <<"kube "<<p1<<" + "<<p2<<", value="<<insidev;
    p2+=p1;
  };

  kube(dbl3 _p1, dbl3 _size, int _insidev): shape(/*polyType*/'k',_insidev),
      p1(_p1), p2(_p1 + _size)
    {}

  int value(dbl3 ij)  const {  return ij.x >= p1.x && ij.y >= p1.y && ij.z >= p1.z &&   ij.x < p2.x && ij.y < p2.y && ij.z < p2.z
                                     ? insidev : shape::invalidv;  }
};

class plate : public shape {
  //! plane capped by sphere
  dbl3 p1,p2, po_;
 public:

  plate(stringstream & ins): shape(/*polyType*/'p') {
    cout<<"Error: fix me saqdakjoigfgfgfg "<<endl;
    ins>>p1>>po_; p2=p1;
    cout <<"\n plate: slope_x="<<p1.x<<"  r_cap="<<p1.y<<"  slope_z="<<p1.z<<"\n"<<endl;
    p1.y=0.;    po_.z = po_.y;  po_.y = (p1.x*po_.x+p1.z*po_.z+p1.y);
    cout <<"plate a1="<<p1.x<<"   a2="<<p2.x<<"    r="<<p1.y<<"    b2="<<p2.y<<';';
  };
  int value(dbl3 ij) const {  return  ( ij.y > p1.x*ij.x + p1.z*ij.z+p1.y && mag(ij-po_) <= p2.y )  ?  insidev : shape::invalidv;  }
};

class sphere : public shape {
  dbl3 p1; double r2;
  public:
  sphere(stringstream & ins): shape(/*polyType*/'s') {
    double rr;
    ins>>p1 >>rr >>insidev;   r2=rr*rr;
    cout <<"sphere  c="<<p1<<",  r="<<rr;
  }
  sphere(dbl3 _p1, double rr, int _insidev): shape(/*polyType*/'s',_insidev),
      p1(_p1), r2(rr*rr)
    {}

  int value(dbl3 ij) const {  return  ( magSqr(ij-p1)<r2 )  ?  insidev : shape::invalidv;  }
};


class roughSphere : public shape {

  dbl3 p1; double rr, r2, r2min;
  int nXimg, nYimg; //< image sizes after cut to make image size the largest integer fraction of 2PI/2
  voxelImageT<float> height;
 public:
  roughSphere(stringstream & ins): shape(/*polyType*/'r') {
    string fnam;
    ins>>p1 >>rr >>fnam;   r2=rr*rr; r2min=0.1;
     ins>>insidev;

    //voxelImageT<float> omg();

    //dbl3 dx=omg.dx();
    //nXimg=omg.size3().x;  { int nResampl = max(1.,ceil(PI*rr/(nXimg*dx.x)))+0.01;  // nMirrors=2*nResampl
      //ensure(nXimg>=PI*rr/(nResampl*dx.x));
      //nXimg=PI*rr/(nResampl*dx.x); }
    //imin=(omg.size3().x-nXimg)/2;
    //nYimg=omg.size3().y;  { int nResampl = max(1.,ceil(PI*rr/(nYimg*dx.y)))+0.01;  // nMirrors=2*nResampl
      //ensure(nYimg>=PI*rr/(nResampl*dx.y));
    //nYimg=PI*rr/(nResampl*dx.y); }
    //jmin=(omg.size3().y-nYimg)/2;
    //height.reset({nXimg,nYimg,1},0.);
    //height=omg;
    cout <<"\nroughSphere  p1="<<p1<<"    r^2="<<std::sqrt(r2)<<"    fnam="<<fnam<<" not implemented "<<endl;
    cout <<" nXimg "<<nXimg<<" nYimg "<<nYimg<<endl;
  }
  int value(dbl3 ij) const {
    dbl3 xyz=ij-p1;
    double r=mag(xyz)+1e-64; double phi=acos(xyz.z/r); double tta=abs(atan2(xyz.y, xyz.x));
    int iy=rr*phi/height.dx().y; iy%=nYimg; //iy+=jmin;

    int ix=rr*tta/height.dx().x; ix%=nXimg; //ix+=imin;
    double delRough=height(ix,iy,0);
    return (magSqr(ij-p1)<r2+delRough)  ?  insidev : shape::invalidv;  }
};

} //namespace VoxLib


namespace MCTProcessing {
using namespace std;
using namespace VoxLib;

//template<typename T, typename _operate_>
//void applyShapeOper(voxelImageT<T> & vImg, const shape& sh, _operate_& func) {
#define _SHAPERATEPy(vImg, sh, _operate_)  \
  switch (sh.polyType) {                 \
    case 's':  shape::_operate_(vImg, static_cast<const      sphere&>(sh)); break;  \
    case 'r':  shape::_operate_(vImg, static_cast<const roughSphere&>(sh)); break;  \
    case 'p':  shape::_operate_(vImg, static_cast<const       plate&>(sh)); break;  \
    case 'f':  shape::_operate_(vImg, static_cast<const  paraPlates&>(sh)); break;  \
    case 'k':  shape::_operate_(vImg, static_cast<const        kube&>(sh)); break;  \
    case 'l':  shape::_operate_(vImg, static_cast<const       layer&>(sh)); break;  \
    case 'g':  shape::_operate_(vImg, static_cast<const   gridLines&>(sh)); break;  \
    case 'c':  shape::_operate_(vImg, static_cast<const    cylinder&>(sh)); break;  \
    case 't':  shape::_operate_(vImg, static_cast<const  triangular&>(sh)); break;  \
    default: cout <<"\n unregistered shape type: "<<sh.polyType<<"\n"<<endl;        \
  }
//}

#define _SHAPERATE(_operate_)  \
  if(ins.peek()=='?') { ins.str("\"s(phere), p(late, capped), f(flat-plates), c(ylinder), k(ube) or t(riangular sinusidal duct)\" position..."); return true; } \
  std::string tmpc;  ins>>tmpc; \
  if      (tmpc[0]=='s')  shape::_operate_(vImg, sphere(     ins)); \
  else if (tmpc[0]=='r')  shape::_operate_(vImg, roughSphere(ins)); \
  else if (tmpc[0]=='p')  shape::_operate_(vImg, plate(      ins)); \
  else if (tmpc[0]=='f')  shape::_operate_(vImg, paraPlates( ins)); \
  else if (tmpc[0]=='k')  shape::_operate_(vImg, kube(       ins)); \
  else if (tmpc[0]=='l')  shape::_operate_(vImg, layer(      ins)); \
  else if (tmpc[0]=='g')  shape::_operate_(vImg, gridLines(  ins)); \
  else if (tmpc[0]=='c')  shape::_operate_(vImg, cylinder(   ins)); \
  else if (tmpc[0]=='t')  shape::_operate_(vImg, triangular( ins)); \
  else cout <<"\n unsupported shape type: "<<tmpc<<"\n"<<endl

template<typename T>  bool Paint( stringstream& ins, voxelImageT<T> & vImg)  {
  _SHAPERATE(setIn);
  return true;
}

template<typename T>  bool PaintAdd( stringstream& ins, voxelImageT<T> & vImg) {
  _SHAPERATE(addTo);
  return true;
}

template<typename T>  bool PaintBefore( stringstream& ins, voxelImageT<T> & vImg) {
  _SHAPERATE(setBefor);
  return true;
}

template<typename T>  bool PaintAfter( stringstream& ins, voxelImageT<T> & vImg) {
  _SHAPERATE(setAfter);
  return true;
}

template<typename T>  bool PaintAddBefore( stringstream& ins, voxelImageT<T> & vImg) {
  _SHAPERATE(addBefor);
  return true;
}

template<typename T>  bool PaintAddAfter( stringstream& ins, voxelImageT<T> & vImg) {
  _SHAPERATE(addAfter);
  return true;
}

} // namespace MCTProcessing
