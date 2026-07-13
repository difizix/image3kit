#pragma once

// Convinience vector classes: var3, var2, piece and Vars...


#include <cstddef>
#include <iomanip>
#include <string>
#include <vector>
#include <utility>
#include <initializer_list>
#include <sstream>
#include <array>
#include <cmath>
#include <algorithm>
#include <map>
#include <iostream>
#include <cassert>
#include <regex>
#include <numeric>


#ifdef VMMLIB
#include "Vctr.h" // order is important for typse.h
#endif


template<typename T> T strTo(const std::string &s){  std::istringstream ss(s);  T t;  ss>>t;  return t; }

#ifndef verySmall
  #define  verySmall  1e-31
  #define   maxT(Typ)        (std::numeric_limits<Typ>::max())
  #define   minT(Typ)        (std::numeric_limits<Typ>::min())
  #define  iminT(Typ) (int   (std::numeric_limits<Typ>::min()))
  #define  dminT(Typ) (double(std::numeric_limits<Typ>::min()))
  #define  imaxT(Typ) (int   (std::numeric_limits<Typ>::max()))
  #define  fmaxT(Typ) (float (std::numeric_limits<Typ>::max()))
  #define  dmaxT(Typ) (double(std::numeric_limits<Typ>::max()))
  #define  TImax(Typ)   (Tint(std::numeric_limits<Typ>::max()))
  #define  epsT(Typ)          std::numeric_limits<Typ>::epsilon()
#endif

// #include globals.h leads to compile errors in gcc-13
#ifndef TOSTRING
#define STRINGIFY(xpandd) #xpandd
#define TOSTRING(x_macro) STRINGIFY(x_macro)
#endif

#ifdef OpenMP
 #ifdef _debugCompile_
  #define OMPragma(...)
 #else
  #define OMPragma(...) _Pragma(__VA_ARGS__)
 #endif
#else
 #define OMPragma(...)
#endif
#define OMPFor(...)  OMPragma(TOSTRING(omp parallel for __VA_ARGS__))

#define for_(_vector_m, _i_m)  for(size_t _i_m=0; _i_m<_vector_m.size(); ++_i_m)
#define for_0(_i_end_m, _ind_)  for(int _ind_=0; _ind_<_i_end_m; ++_ind_)
#define for_i_(_i_bgn_m,_i_end_m)  for(int i=_i_bgn_m; i<_i_end_m; ++i)


constexpr double PI = 3.141592653589793;

//! 3D vector class template
template<class T>
struct var3  {
  T  x, y, z;
  var3() = default;// Warning: not zero initialized in opt mode
  template< typename std::enable_if<std::is_arithmetic<T>::value,int>::type = 0>
  explicit var3(T r)            { x = r;     y = r;     z = r; } // use this to zero-initialize
  constexpr var3(T r, T s, T t) { x = r;     y = s;     z = t; }
  explicit var3(const T* d)     { x = d[0];  y = d[1];  z = d[2]; }
  var3(std::nullptr_t) = delete;
  template<class U>
  var3(var3<U> n)               { x = T(n.x);  y = T(n.y);  z = T(n.z); }
  #ifdef VMMLIB__VECTOR__HPP
  var3(const Vctr<3,T>& v3)     { x = v3[0]; y = v3[1]; z = v3[2]; }
  #endif

  T&       operator [](int k)         { return ((&x)[k]); }
  const T& operator [](int k)  const  { return ((&x)[k]); }
  T _0()                       const  { return x; }
  T _1()                       const  { return y; }
  T _2()                       const  { return z; }


  var3&  operator += (const var3& v)  { x += v.x;  y += v.y;  z += v.z;  return  *this; }
  var3&  operator -= (const var3& v)  { x -= v.x;  y -= v.y;  z -= v.z;  return  *this; }
  var3&  operator += (const T& t)     { x += t;    y += t;    z += t;    return  *this; } // clumsy
  var3&  operator -= (const T& t)     { x -= t;    y -= t;    z -= t;    return  *this; } // clumsy
  var3&  operator *= (double t)       { x *= t;    y *= t;    z *= t;    return  *this; }
  var3&  operator /= (double t)       { return (*this *= 1./t);  }
  var3&  operator ^= (const var3& v)  { double r=y*v.z-z*v.y, s=z*v.x-x*v.z;  z=x*v.y-y*v.x;  x=r; y=s;   return *this; }
  var3&  operator *= (const var3& v)  { x *= v.x;  y *= v.y;  z *= v.z;  return *this; }
  var3   operator -  (void)          const { return  var3(-x,    -y,    -z); }
  var3   operator +  (const var3& v) const { return  var3(x+v.x, y+v.y, z+v.z); }
  var3   operator -  (const var3& v) const { return  var3(x-v.x, y-v.y, z-v.z); }
  var3   operator +  (const T& t)    const { return  var3(x+t,   y+t,   z+t  ); } // clumsy
  var3   operator -  (const T& t)    const { return  var3(x-t,   y-t,   z-t  ); } // clumsy
  var3   operator *  (double t)      const { return  var3(x*t,   y*t,   z*t  ); }
  var3   operator *  (const var3& v) const { return  var3(x*v.x, y*v.y, z*v.z); }
  var3   operator /  (const var3& v) const { return  var3(x/v.x, y/v.y, z/v.z); }
  var3   operator /  (double t)      const { double  f = 1./t;  return var3(x*f, y*f, z*f); }
  double operator &  (const var3& v) const { return  x*v.x+y*v.y+z*v.z; }
  var3   operator ^  (const var3& v) const { return  var3(y*v.z-z*v.y,  z*v.x-x*v.z,  x*v.y-y*v.x); }
  bool   operator == (const var3& v) const { return  (x-v.x)*(x-v.x) + (y-v.y)*(y-v.y) + (z-v.z)*(z-v.z) < verySmall; }
  bool   operator != (const var3& v) const { return  !(v==*this); }
};

typedef  double           dbl;
typedef  var3<int>        int3;
typedef  var3<int3>       int3x3;
typedef  var3<float>      float3;
typedef  var3<double>     dbl3;

template<class T>  var3<T> rotateAroundVec(const var3<T> b, double gamma, var3<T> n)  {
  //! Rotate b around line in the direction of n passing through centre, http://inside.mines.edu/~gmurray/ArbitraryAxisRotation
  double sg = sinf(gamma),   cg = cosf(gamma),  nb = (n.x*b.x + n.y*b.y + n.z*b.z)*(1.-cg);
  return var3<T>( n.x*nb + b.x*cg + (n.y*b.z-n.z*b.y)*sg,
                  n.y*nb + b.y*cg + (n.z*b.x-n.x*b.z)*sg,
                  n.z*nb + b.z*cg + (n.x*b.y-n.y*b.x)*sg );
}


//! 2D vector class template
template<class T>
struct var2  {
  T a;
  T b;
  var2() = default;//not zero initialized
  var2(T r, T s)          { a = r;        b = s; }
  var2(int r)             { a = r;        b = r; }  // use this to zero-initialize //ERROR REMOVE ME
  var2(const T* d)        { a = d[0];     b = d[1]; }
  var2(std::pair<T,T> d)  { a = d.first;  b = d.second; }

  T&       operator [](long k)        { return ((&a)[k]); }
  const T& operator [](long k) const  { return ((&a)[k]); }

  explicit operator int()    const { return a; }
  explicit operator double() const { return a; }
  explicit operator std::pair<T,T>&() const { return *this; }

  T x() const  { return a; }
  T y() const  { return b; }

  T first() const  { return a; }
  T second() const { return b; }

  var2&  operator += (const var2& v)  { a += v.b;  b += v.b; return *this; }
  var2&  operator -= (const var2& v)  { a -= v.b;  b -= v.b; return *this; }
  var2&  operator *= (double t)       { a *= t  ;  b *= t  ; return *this; }
  var2&  operator *= (const var2& v)  { a *= v.b;  b *= v.b; return *this; }
  var2&  operator /= (double t)       { double f = 1./t;  a *= f;  b *= f;  return *this; }
  var2   operator -  (void) const          { return (var2(-a   , -b   )); }
  var2   operator +  (const var2& v) const { return (var2(a+v.b, b+v.b)); }
  var2   operator -  (const var2& v) const { return (var2(a-v.b, b-v.b)); }
  var2   operator *  (double t)      const { return (var2(a*t  , b*t  )); }
  var2   operator /  (double t)      const { double f = 1./t;  return (var2(a*f, b*f)); }
  double operator &  (const var2& v) const { return (a*v.b+b*v.b); }
  var2   operator *  (const var2& v) const { return (var2(a*v.b, b*v.b)); }
  var2   operator /  (const var2& v) const { return (var2(a/v.b, b/v.b)); }
  bool   operator == (const var2& v) const { return ((a-v.b)*(a-v.b) <  verySmall) && ((b-v.b)*(b-v.b) <  verySmall) ; }
  bool   operator != (const var2& v) const { return ((a-v.b)*(a-v.b) >= verySmall) || ((b-v.b)*(b-v.b) >= verySmall) ; }
};

typedef   var2<float>  float2;
typedef   var2<double> dbl2;
typedef   var2<int>    int2;

template<class T>  var3<T> operator *(double t, const var3<T>& v) { return var3<T>(t*v.x, t*v.y, t*v.z); }
inline  dbl3   operator *(const int3& v, const dbl3& d) { return dbl3(v.x*d.x, v.y*d.y, v.z*d.z); } // boost int to T
template<class T>  T       mag(const var3<T>& v)        { return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z); }
template<class T>  T       sum(const var3<T>& v)        { return (v.x+v.y+v.z); }
template<class T>  double  magSqr(const var3<T>& v)     { return (v.x*v.x+v.y*v.y+v.z*v.z); }
template<class T>  var3<T> norm(const var3<T>& v)       { return  v/(mag(v)+1e-300); }


//. component-wise max and min
template<class T> var3<T> max(const var3<T>& a, const var3<T>& b)  { return var3<T>(std::max(a.x,b.x),std::max(a.y,b.y),std::max(a.z,b.z)); }
template<class T> var3<T> min(const var3<T>& a, const var3<T>& b)  { return var3<T>(std::min(a.x,b.x),std::min(a.y,b.y),std::min(a.z,b.z)); }

template<class T>  T  toScalar(const T& v)       { return v; } // NOW IMPLICITly CONVERTED
template<class T>  T  toScalar(const var3<T>& v) { return mag(v); }



//! a piece of a contiguous array, used for efficient and prettier pass of array contents than using iterators, similar to std::string_view
template<class T>
struct piece  {

  piece()                   noexcept: d(0) ,   dn(0)     {}
  piece(T* dd, size_t n)    noexcept: d(dd),   dn(d+n)   {}
  piece(T* dd, T* de)       noexcept: d(dd),   dn(de)    {}
  piece(const piece& p)     noexcept: d(p.d),  dn(p.dn) {}; //! Note: different from operator=(), note: casting away unenforced const
  piece(std::vector<T>& vs) noexcept: d(&*vs.begin()), dn(d+vs.size()) {};
  void reset(T* dd, size_t n)    {    d=dd;     dn=d+n; };
  void reset(const piece& vs)    {    d=&vs[0]; dn=d+vs.size(); }//! note data hold by piece are not const unless piece is const itself
  void reset(std::vector<T>& vs) {    d=&vs[0]; dn=d+vs.size(); }

  T*       begin()    const { return d; }
  T*       end()      const { return dn; }
  const T& back()     const { return *(dn-1); }
  const T* cbegin()   const { return d; }
  const T* cend()     const { return dn; }
  T& operator[](size_t i)const { return d[i]; }
  //const T& operator [](size_t i) const { return d[i]; }
  size_t   size()     const { return dn-d; }
  bool     empty()    const { return dn<=d; }
  size_t   capacity() const { return dn-d; }
  T*       data()           { return d; }
  const T* data()     const { return d; }

  piece&        fill(const     T& v)  { std::fill(d, dn, v);                  return *this; }
  piece& operator  =(const     T& v)  { std::fill(d, dn, v);                  return *this; }
  piece& operator  =(const piece& v)  { std::copy(v.d, v.dn, d); /*risky*/    return *this; }
  piece& operator +=(const piece& v)  { for(auto& a:*this){ a += v[&a-d]; };  return *this; }
  piece& operator -=(const piece& v)  { for(auto& a:*this){ a -= v[&a-d]; };  return *this; }
  piece& operator *=(const piece& v)  { for(auto& a:*this){ a *= v[&a-d]; };  return *this; }
  piece& operator /=(const piece& v)  { for(auto& a:*this){ a /= v[&a-d]; };  return *this; }
  piece& operator +=(T            v)  { for(auto& a:*this){ a += v; };        return *this; }
  piece& operator -=(T            v)  { for(auto& a:*this){ a -= v; };        return *this; }
  piece& operator *=(double       t)  { for(auto& a:*this){ a *= t; };        return *this; }
  piece& operator *=(size_t       t)  { for(auto& a:*this){ a *= t; };        return *this; }
  piece& operator /=(double       t)  { return (*this)*=(1./t); }
  T sum() const                       { return std::reduce(d,dn); }
  T avg() const                       { return sum()/size(); } // see also sumdbl

//protected:
  T*  d;
  T*  dn;
};


template<class T>  piece<T> rang(T* t, size_t n) { return piece<T>(t, n); }
template<class T, size_t N>  piece<T> allof(std::array<T, N>& vs) { return piece<T>(&*vs.begin(), &*vs.end()); }
template<class T, template<class ...> class C>  piece<T> allof(C<T>& vs) { return piece<T>(&*vs.begin(), &*vs.end()); }
template<class T, template<class ...> class C>  piece<T const> allof(const C<T>& vs) { return piece<T const>(&*vs.cbegin(), &*vs.cend()); }

template <typename T>
class Alocer20pc {
public:
    T* aloc(size_t n) const {
        if (n == 0) return nullptr;
        return new T[n];
    }

    void deallocate(T* p, size_t)  const noexcept {
        delete[] p;
    }
};

//! Vars can be used to hold a contiguous array, less ugly than std::vallarray!
//template<class T>
template<class T, class Alocer = Alocer20pc<T>>
struct Vars: public piece<T>  {
  using piece<T>::d;
  using piece<T>::dn;
  static constexpr Alocer20pc<T> A{};

  Vars(): piece<T>() {};
  Vars(size_t siz): piece<T>(A.aloc(siz), siz) {};
  Vars(size_t siz, const T& val): piece<T>(A.aloc(siz), siz) {  std::fill(d, dn, val); }
  Vars(const piece<T>& v):        piece<T>(A.aloc(v.size()), v.size()) {  std::copy(v.d, v.dn, d); }
  Vars(const Vars& v):            piece<T>(A.aloc(v.size()), v.size()) {  std::copy(v.d, v.dn, d); }
  Vars(const std::vector<T>& v):  piece<T>(A.aloc(v.size()), v.size()) {  std::copy(&v[0], &*v.end(), d); }
  Vars(const piece<T>& v, T(*func)(const T&)): piece<T>(A.aloc(v.size()), v.size()) { std::transform(&v[0], &*v.end(), d, func); }
  Vars(Vars&& v)  noexcept:       piece<T>(v.d, v.size())      { v.d=0; v.dn=0; }
  Vars(const T* dd, size_t nn):   piece<T>(A.aloc(nn), nn)       { std::copy(dd, dd+nn, d); }
  Vars(const T* dd, const T* de): piece<T>(A.aloc(de-dd), de-dd) { std::copy(dd, de, d); }
  ~Vars() noexcept { delete[] d; }

  void operator =(Vars&& v){ eat(v); }
  void eat(Vars& v)       { delete[] d;  d = v.d;  dn=v.dn; v.d=0; v.dn=0; };
  void operator =(const piece<T>& v){ _resiZ(v.size());  std::copy(v.d, v.dn, d); }
  void operator =(const Vars& v)    { _resiZ(v.size());  std::copy(v.d, v.dn, d); }
  void operator =(const std::vector<T>& v){ _resiZ(v.size()); std::copy(&v[0], &*v.end(), d); }
  void operator =(const std::initializer_list<T>& v){ _resiZ(v.size()); std::copy(v.begin(), v.end(), d); }


  Vars&        fill(const        T& v)  { std::fill(d, dn, v);      return *this; }
  Vars& operator  =(const        T& v)  { std::fill(d, dn, v);      return *this; }
  Vars& operator +=(const piece<T>& v)  { piece<T>::operator+=(v);  return *this; }
  //TODO add && versions
  Vars& operator -=(const piece<T>& v)  { piece<T>::operator-=(v);  return *this; }
  Vars& operator *=(const piece<T>& v)  { piece<T>::operator*=(v);  return *this; }
  Vars& operator +=(T v)       { for(auto& a:*this){ a += v; };  return *this; }
  Vars& operator -=(T v)       { for(auto& a:*this){ a -= v; };  return *this; }
  Vars& operator *=(double t)  { for(auto& a:*this){ a *= t; };  return *this; }
  Vars& operator *=(int t)     { for(auto& a:*this){ a *= t; };  return *this; }
  Vars& operator /=(double t)  { return (*this)*=(1./t); }
  template<class U> Vars& operator *=(const piece<U>& v)  { for(auto& a:*this){ a *= v[&a-d]; };  return *this; }
  template<class U> Vars& operator /=(const piece<U>& v)  { for(auto& a:*this){ a /= v[&a-d]; };  return *this; }

  void resize(size_t nn) { /// does not release the memory
    if(d){
      size_t no = dn-d;
      if (nn>no) { // || nn<(no>>1)
        T* o=d, *on=std::min(dn, o+nn);
        _newD(nn);  std::copy(o, on, d);
        delete[] o;
      }
      else dn = d+nn;
    } else { _newD(nn); }
  }
  void resize(size_t nn, const T& val)  {
    size_t no = dn-d;
    resize(nn);
    if(nn>no) std::fill(d+no, dn, val);
  }

  void pbak(T& vj) {// inefficient, use std::vector instead for dyn_array
    if(d){ T* o=d, *on=dn;  _newD(dn+1-d);  std::copy(o, on, d);  *(dn-1)=vj;  delete[] o; }
    else { _newD(1); d[0]=vj; }
  }
  void pbak(const piece<T> vs) {
    if(d){ T* o=d, *on=dn;  _newD(on+vs.size()-o);  std::copy(o, on, d);
           delete[] o;  std::copy(vs.d, vs.dn, dn-vs.size()); }
    else { _newD(vs.size());  std::copy(vs.d, vs.dn, d); }
  }
  protected:
  void _newD(size_t siz) { d = new T[siz]; dn=d+siz; }
  void _resiZ(size_t siz) {
    if (siz != size_t(dn-d)) {
      delete[] d;
      d = new T[siz];
    }
    dn=d+siz;
  }
};

typedef   Vars<double>         dbls;
typedef   Vars<float>          floats;
typedef   Vars<dbl3>           dbl3s;
typedef   Vars<float3>         float3s;
typedef   Vars<int>            ints;
typedef   Vars<unsigned char>  uchars;
template<class T>  using  vars = Vars<T>; // or std::vector<T>;
template<size_t N> using  Dbl_ = std::array<double,N>;
template<size_t NH, size_t NX> using  DDbl_ = std::array<std::array<double,NH>,NX>;


template<class T>  Vars<T>        operator -(const piece<T>& dis)   { Vars<T> rt(dis);  for(auto& a:rt){ a = -a; }  return rt;  }
template<class T>  Vars<T>        operator +(const piece<T>& dis, const piece<T>& v)  { Vars<T> rt(dis); return rt+=v;  }
template<class T>  Vars<T>        operator -(const piece<T>& dis, const piece<T>& v)  { Vars<T> rt(dis); return rt-=v;  }
//template<class T>  Vars<T>        operator *(const piece<T>& dis, const piece<T>& v)  { Vars<T> rt(dis); rt*=v;  return rt;  }
template<class T, class U> Vars<U> operator *(const T dis, const piece<U>& v) { Vars<U> rt(v); return rt*=dis;  }
template<class T> Vars<var3<T>>    operator *(const piece<T>& dis, const piece<var3<T>>& v) { Vars<var3<T>> rt(v); return rt*=dis;  }
//template<class T, typename U> Vars<T>  operator *(const piece<T>& dis, const piece<U>& v) { Vars<T> rt(dis); return rt*=v;  }
template<class T, class U> Vars<T> operator /(const piece<T>& dis, const piece<U>& v) { Vars<T> rt(dis); return rt/=v;  }
template<class T>  Vars<T>        operator +(const piece<T>& dis,T t)        { Vars<T> rt(dis); rt+=t;    return rt; }
template<class T>  Vars<T>        operator -(const piece<T>& dis,T t)        { Vars<T> rt(dis); rt-=t;    return rt; }
template<class T>  Vars<T>        operator *(const piece<T>& dis, double t)  { Vars<T> rt(dis); rt*=t;    return rt; }
template<class T>  Vars<T>        operator *(const piece<T>& dis, int t)     { Vars<T> rt(dis); rt*=t;    return rt; }
template<class T>  Vars<T>        operator /(const piece<T>& dis, double t)  { Vars<T> rt(dis); rt*=1./t; return rt; }
template<class T>  Vars<T>        operator /(double t, const piece<T>& dis)  { Vars<T> rt(dis); for(auto& a:rt){ a = t/a; } return rt; }
inline            dbls      mag(const piece<double>& dis)   { dbls rt(dis.size()); for_(dis,i){ rt[i] = std::abs(dis[i]); } return rt; }
inline            floats    mag(const piece<float>& dis)    { floats rt(dis.size()); for_(dis,i){ rt[i] = std::abs(dis[i]); } return rt; }
template<class T>  Vars<T>   mag(const piece<var3<T>>& dis)  { Vars<T> rt(dis.size()); for_(dis,i){ rt[i] = mag(dis[i]); } return rt; }

template<class T> Vars<T>  operator &(const piece<var3<T>>& dis, const piece<var3<T>>& v) { Vars<T> rt(dis.size()); for_(rt,i){ rt[i] = dis[i]&v[i]; } return rt; }



//! same as Vars but with a default value to hold a uniform vector,
//! a name and transformation information, used in #svplot only for now
template<class T>
class varsORv: public Vars<T> {
  using piece<T>::d;
  using piece<T>::dn;
 public:
  T dfult;
  std::string name;
  T Xa,  Xp; //!< scale, shift
  T (*transf)(T);
  T ax,  px; //!< scale, shift before transform

  varsORv()                        :                   dfult(0)       , Xa(1)    , Xp(0)    , transf([](double a){return a;}), ax(1)    , px(0)     {}
  varsORv(const T& val)            :                   dfult(val)     , Xa(1)    , Xp(0)    , transf([](double a){return a;}), ax(1)    , px(0)     {}
  varsORv(size_t siz, const T& val): Vars<T>(siz,val), dfult(val)     , Xa(1)    , Xp(0)    , transf([](double a){return a;}), ax(1)    , px(0)     {}
  varsORv(const Vars<T>& vs)       : Vars<T>(vs),     dfult(vs.back()), Xa(1)    , Xp(0)    , transf([](double a){return a;}), ax(1)    , px(0)     {}
  varsORv(const varsORv& vs)       : Vars<T>(vs),      dfult(vs.dfult), Xa(vs.Xa), Xp(vs.Xp), transf(vs.transf)              , ax(vs.ax), px(vs.px) {}
  varsORv(const std::vector<T>& vs): Vars<T>(vs),      dfult(vs.dfult), Xa(vs.Xa), Xp(vs.Xp), transf(vs.transf)              , ax(vs.ax), px(vs.px) {}
  varsORv(varsORv&& vs)  noexcept  : Vars<T>(vs),      dfult(vs.dfult), Xa(vs.Xa), Xp(vs.Xp), transf(vs.transf)              , ax(vs.ax), px(vs.px) {}
  varsORv(const T* dd, size_t nn)  : Vars<T>(dd,nn),   dfult(dd[nn-1]), Xa(1)    , Xp(0)    , transf([](double a){return a;}), ax(1)    , px(0)     {}
  varsORv(const T* dd, const T* de): Vars<T>(dd,de),   dfult(*(de-1)) , Xa(1)    , Xp(0)    , transf([](double a){return a;}), ax(1)    , px(0)     {}
  varsORv(const T* dd, const T* de, T(*fn)(T)): Vars<T>(dd,de,fn)     , Xa(1)    , Xp(0)    , transf([](double a){return a;}), ax(1)    , px(0)     {}

  //varsORv(Vars<T>& vs, bool move): Vars<T>(vs,move)  {} //same as above but enforced by move
  void operator =(const piece<T>& v)  { delete[] d;  d = new T[v.size()]; std::copy(v.d, v.dn, d);  dn=d+v.size(); };
  void operator =(const Vars<T>& v)   { delete[] d;  d = new T[v.size()]; std::copy(v.d, v.dn, d);  dn=d+v.size(); };
  void operator =(const varsORv& v)   { delete[] d;  d = new T[v.size()]; std::copy(v.d, v.dn, d);  dn=d+v.size();
                                        dfult=v.dfult; Xa=v.Xa; Xp=v.Xp; transf=v.transf; ax=v.ax; px=v.px; };
  void operator =(const std::vector<T>& v){ delete[] d;  d = new T[v.size()]; std::copy(&v[0], &*v.end(), d);  dn=d+v.size(); };
  void operator =(const std::initializer_list<T>& v){ delete[] d;  d = new T[v.size()]; std::copy(v.begin(), v.end(), d);  dn=d+v.size(); };

  void operator =(Vars<T>&& v) noexcept { Vars<T>::eat(v); }

  T&       operator[](size_t i)       { if(d+i<dn) return d[i]; else return dfult; }
  const T& operator[](size_t i) const { if(d+i<dn) return d[i]; else return dfult; }
  T scalefrom01(T val) const { return Xp+Xa*transf(val*ax+px); }

  ///fn shall be inverse of transf
  void rescaledata(T d0, T del, T(*fn)(T))             { Xp=d0; Xa=del; T* dd=d-1;  while(++dd<dn)  *dd = fn((*dd-Xp)/Xa);  }
  void rescaledata(T d0, T del, T(*fn)(T), T p0, T a0) { Xp=d0; Xa=del; px=fn((p0-Xp)/Xa); ax=fn((p0+a0-Xp)/Xa)-px;
                                                           T* dd=d-1; while(++dd<dn)  *dd = (fn((*dd-Xp)/Xa)-px)/ax;  }
  void rescaledata(T d0, T del)                         { Xp=d0; Xa=del; T* dd=d-1;  while(++dd<dn)  *dd = (*dd-d0)/del;  }
};

inline double rescaleval(double val, double d0, double del, double(*fn)(double), double p0, double a0) {
  double px=fn((p0-d0)/del); double ax=fn((p0+a0-d0)/del)-px;  return (fn((val-d0)/del)-px)/ax; }

template<class T>  Vars<T> operator -(T v, Vars<T> vs)  { Vars<T> rt(vs); for(auto& a:rt){ a =v-a; };  return rt; }

template<class T>  var3<T> round(const var3<T>& vec)   {   var3<T> rt(vec);   rt.x=round(vec.x); rt.y=round(vec.y); rt.z=round(vec.z);   return rt;   }
template<class T>  Vars<T> round(const Vars<T>& vec)   {   Vars<T> rt(vec);   for(auto& v:rt) v=round(v);   return rt;   }




#ifndef TYPSES_OERATIONS_H
#define TYPSES_OERATIONS_H
#define _T_ typename std::remove_const<T>::type



template<class C> int len(const C& vec) { return vec.size(); } // casts size to int, deprecated

template<class T> double sumdbl(const piece<T>& ps)  { double sm=1e-300; for(auto a:ps){ sm += a; }  return sm; }
//template<class T> double sumagdbl(const piece<T>& ps, const piece<T>& ws) { double sm=1e-300; const T* p=ps.d-1, *w=ws.d-1; while(++p<ps.dn){ sm+= *(++w) * *p; }  return sm; }
//inline            double sumdbl(const dbl3& ps, const dbl3& ws)  { return ps.x*ws.x + ps.y*ws.y + ps.z*ws.z; }
inline            double sum(const dbl3& ps)                  { return std::abs(ps.x)+ std::abs(ps.y)+ std::abs(ps.z); }

template<class T> T sumq(piece<T> ps)  {  T sm = *ps.d; while(++ps.d<ps.dn){ sm += *ps.d; }  return sm; }//! quick sum, assumes non-empty vec

template<class T>          T sum(const piece<T>& ps)  {  _T_ sm = (T()*0.); const T* p= ps.d-1; while(++p<ps.dn){ sm+= *p; }  return sm; }
template<class T, class U> T sum(const piece<T>& ps, const piece<U>& ws)  {  _T_ sm= (T()*0.);  T* p= ps.d-1;  U *w=ws.d-1; while(++p<ps.dn){ sm+= *(++w) * *p; }  return sm; }
template<class T>    vars<T> sum(piece<piece<T>> ps)  { Vars<T> sm(ps[0]); while(++ps.d<ps.dn){ sm+= *ps.d; }  return sm;  } // does not work for size zero

template<class T>           T sumSqrs(const piece<T>& ps)  {  T sm= (T()*0.); const T* p= ps.d-1; while(++p<ps.dn){ sm+= *p * *p; }  return sm; }
template<class T, class U>  T sumSqrs(const piece<T> ps, const piece<U> ws)  { //TODO remove refs
                                      T sm = (T()*0.); const T* p = ps.d-1; const U *w=ws.d-1; while(++p<ps.dn){ sm += *(++w) * *p * *p; }  return sm; }
template<class T>   double sumdblSqrs(const piece<T>& ps)  {  double sm=0.; const T* p= ps.d-1; while(++p<ps.dn){ sm+= double(*p) * *p; }  return sm; }


template<class T> double avg(const piece<T>& dis)  { return sum(dis)/double(dis.size()); }
template<class T> double avgdbl(const piece<T>& ps)  { return sumdbl(ps)/(ps.size()+1e-200); }
template<class T> double avgdbl(const piece<T>& ps, const piece<T>& ws) { return sumdbl(ps,ws)/(sumdbl(ws)+1e-200); }

template<class T>   T min(const piece<T>& pis)  { return *std::min_element(pis.d,pis.dn); }
template<class T>   T max(const piece<T>& pis)  { return *std::max_element(pis.d,pis.dn); }
template<class T, class TFunc> void transform(piece<T>& pis, TFunc func)  { for(T& dr:pis) dr=func(dr); }
template<class T>              void transform(T* d, const T* dn, T(*func)(const T&))  { --d; while (++d<dn) *d=func(*d); }


template<class T> std::vector<T>& operator *= (std::vector<T>& vs, double scale)  {  for(T& v: vs) v*=scale;   return vs;  }

using dbl33 = std::array<var3<double>,3>;
inline dbl33 operator + (const dbl33& v1, const dbl33& v2)  { return {v1[0]+v2[0], v1[1]+v2[1], v1[2]+v2[2]};  }
inline dbl33 operator * (const dbl33& v1, const dbl33& v2)  { return {v1[0]*v2[0], v1[1]*v2[1], v1[2]*v2[2]};  }
inline dbl33 operator * (const dbl33& v1, const double sc)  { return {v1[0]*sc   , v1[1]*sc   , v1[2]*sc   };  }



template<class T, template<class ...> class C1, template<class ...> class C2>
vars<vars<T> > transpose(const C1<C2<T> >& vv)  {
  if(!vv.size()) return vars<vars<T> >();
  vars<vars<T> > trans(vv[0].size(),vars<T>(vv.size()));
  for (size_t i=0; i<vv[0].size(); ++i)
    for (size_t j=0; j<vv.size(); ++j) trans[i][j] = vv[j][i] ;
  return trans;
}

template<class T, size_t N, template<class ...> class C1>
vars<vars<T> > transpose(const C1<std::array<T,N> >& vv)  {
  if(!vv.size()) return vars<vars<T> >();
  vars<vars<T> > trans(vv[0].size(),vars<T>(vv.size()));
  for (size_t i=0; i<vv[0].size(); ++i)
    for (size_t j=0; j<vv.size(); ++j) trans[i][j] = vv[j][i] ;
  return trans;
}


//! class table IO, usage: cout<< tableIO(stdVecVec)  (not TableIO, for auto detecting template params)
template<class T, template<class ...> class C2>
class TableIO  {
public:
  TableIO(piece<C2<T>> tbl, std::vector<std::string> hdrs=std::vector<std::string>(), char sepr='\t')
  :  vss_(tbl),  hds_(hdrs),  sep_(sepr) {}; // careful: tbl lifetime: don't pass rvalue tbl

  TableIO(char sepr) : sep_(sepr) {}

  std::string hdr(size_t i) const { return hds_.size()>i ? hds_[i] : ""; }

  C2<T>& operator[](int i) { return vss_[i]; }
  size_t size() const     { return vss_.size(); }

  friend std::ostream& operator << (std::ostream& out, const TableIO& tbl) {
    if(tbl.hds_.size()==tbl.vss_.size()) {
      for_(tbl.hds_,j) { out<<std::setw(12)<<std::left<< tbl.hds_[j]   <<tbl.sep_; }  out<<'\n'; }
    if(tbl.vss_.size()==0) return out;
    for_(tbl.vss_[0],i) {
      for_(tbl.vss_,j) { out<<std::setw(12)<<std::left<< tbl.vss_[j][i]<<tbl.sep_; }  out<<'\n'; }
    out << '\n';
    return out;
  }
protected:
  piece<C2<T>>             vss_; // sync with Table vss
  std::vector<std::string> hds_;
  const char               sep_;
};


//! class Table for IO, usage: cout<< Table().apnd(vec(...),"nam")...<<endl;
template<class T, template<class ...> class C2>
struct Table : public TableIO<T,C2> {
  using TableIO<T,C2>::hds_;
  using TableIO<T,C2>::vss_;

  // use tableIO() as another constructor
  Table(char sepr='\t') : TableIO<T,C2>(sepr) {}

  // FIXME: do not allow rvalue if C2 is a piece/span
  Table& apnd(const C2<T>& vs, std::string nam="")  {
    vss.push_back(vs);
    if(hds_.size() || nam.size())  hds_.push_back(nam);
    vss_.reset(vss);
    return *this;
  }

  Table& apnd(const TableIO<T,C2>& tbl) {
    for_(tbl.vss_,i) {
      vss.push_back(tbl.vss_[i]);
      if(hds_.size() && tbl.hds_.size()) hds_.push_back(tbl.hds_[i]); }
    vss_.reset(vss);
    return *this;
  }

  friend  std::istream& operator >> (std::istream& ins, Table& tbl) { // inefficient!!!
    ins >> std::ws;
    std::string row, item;
    if( std::is_arithmetic<T>::value && isalpha(ins.peek()))  {
      getline( ins, row );
      std::stringstream ss( row );
      while ( getline( ss, item, tbl.sep_ ) )   tbl.hds_.push_back(item);
    }
    std::vector<std::vector<T> > res;
    while( getline( ins, row ))  {
      std::vector<T> Ro;  std::stringstream ss( row );
      while ( getline( ss, item, tbl.sep_ ) )   Ro.push_back( strTo<T>(item) );
      res.push_back( Ro );
    }
    if(res.size()) tbl.vss.resize(res[0].size()); else return ins;
    for (size_t i=0; i<tbl.vss.size(); ++i)  {
      tbl.vss[i].resize(res.size());
      if(res[i].size()!=tbl.vss.size()) std::cout<<"Error table sizes don't match"<<std::endl;
      for (size_t j=0; j<tbl.vss[i].size(); ++j) tbl.vss[i][j]=res[j][i];
    }
    tbl.vss_.reset(tbl.vss);
    return ins;
  }
private:
  std::vector<C2<T>>  vss;
};

template<class T, class... Alloc1, class... Alloc2, template<class ...> class C1, template<class ...> class C2>
TableIO<T,C2> tableIO(C1<C2<T,Alloc2...>, Alloc1...>& vecvec, std::vector<std::string> hdrs=std::vector<std::string>(), char sepr='\t') {
  return TableIO<T,C2>(piece<C2<T,Alloc2...>>(vecvec),hdrs,sepr);  }

// workaround for dumb MSVC
template<class T>
TableIO<T,Vars> tableIO(Vars<Vars<T>>& vecvec, std::vector<std::string> hdrs=std::vector<std::string>(), char sepr='\t') {
  return TableIO<T,Vars>(piece<Vars<T>>(vecvec),hdrs,sepr);
}

template<class T>
std::ostream& operator << (std::ostream& out, const var3<T>& pos) {
  out << pos.x<<' '<< pos.y<<' '<<pos.z;   return out;  }

template<class T>
std::istream& operator >> (std::istream& ins, var3<T>& pos) {
  ins >> pos.x >> pos.y >> pos.z;  return ins; }

template<class T, class U>
std::istream& operator >> (std::istream& ins, std::pair<T,U>& ab) {
  ins >> ab.first >> ab.second;   return ins; }

template<class T, class U>
std::ostream& operator << (std::ostream& out, const std::pair<T,U>& ab) {
  out << ab.first<<' '<<ab.second;  return out;   }

template<class T>
std::istream& operator >> (std::istream& ins, var2<T>& ab) {
  ins >> ab.a >> ab.b;   return ins; }


template<class T>
std::ostream& operator << (std::ostream& out, const var2<T>& ab) {
   out << ab.a<<' '<< ab.b; return out;   }



template<class T>          std::ostream& operator << (std::ostream& out, const std::vector<T>& vec) {
  if(vec.size()<16)  for (auto v : vec) out << v << '\t';
  else               for (auto v : vec) out << v << '\n';
  return out;
}

template<class T>          std::istream& operator >> (std::istream& ins, std::vector<T>& vec) {
  if(vec.size())
    for (size_t i=0; i<vec.size(); ++i)  ins >> vec[i];
  else { T t; while (ins>>t) vec.push_back(t); }
  return ins;
}

template<class T, class U> std::ostream& operator << (std::ostream& out, const std::map<T,U>& vec) {
   for (auto v : vec) { out << v << '\n'; }  return out;   }


template<class T>          std::ostream& operator << (std::ostream& out, const piece<T>& vec)  {
  if(vec.size()<16 ) for (auto v : vec) out << v << '\t';
  else               for (auto v : vec) out << v << '\n';
  return out;
}


template<class T>          std::istream& operator >> (std::istream& ins, piece<T> vec) {// Warning relying on copy constructor not to deep copy
  for (; vec.d<vec.end(); ++vec.d)   ins >> *vec.d;
  return ins;
}

template<class T, size_t N> std::istream& operator >> (std::istream& ins, std::array<T,N>& vec) {
  for(auto& v:vec)      { ins >> v; }   return ins;   }
template<class T, size_t N> std::ostream& operator << (std::ostream& out, const std::array<T,N>& vec)  {
  for (const auto& v: vec) { out << v << '\t'; } return out;  }


// TODO: move to IOUtils.h ?

inline void        replaceInFromTo(std::string& str, const std::string& frm, const std::string& to) {
  //return str = std::regex_replace( str, std::regex(frm), to );
  size_t po = 0;
  while((po = str.find(frm, po)) != std::string::npos) {
    str.replace(po, frm.length(), to);  po += to.length(); }
}

inline void        replaceInBetweenTo(std::string& str, const std::string& bgn, const std::string& end, const std::string & to) {
  //no greedy macth
  size_t po = 0, po0=0;
  while((po = str.find(bgn, po)) != std::string::npos) { po0=po; po=po+bgn.length();
    while((po = str.find(end, po)) != std::string::npos) {
      str.replace(po0, po-po0, to); po = po0+to.length();  break; } }
}

inline std::string replaceFromTo(const std::string& str, const std::string& frm, const std::string& to) {
  return std::regex_replace(str, std::regex(frm), to);
  //size_t po = 0;
  //while((po = str.find(frm, po)) != std::string::npos) { str.replace(po, frm.length(), to);  po += to.length();   }
  //return str;
}


inline std::string basePath(const std::string& path) { /// removes file suffix
  if(auto dl=path.find_last_of('.'); dl != std::string::npos)    {
     if(auto sl=path.find_last_of('/'); sl!=std::string::npos && sl>dl)
      return path;
    return path.substr(0,dl);
  }
  return path;
}

inline std::string nameOf(const std::string& path) { // removes root dir and file suffix
  auto dl=path.find_last_of('.');
  auto sl=path.find_last_of('/');
  if( sl != std::string::npos) {
    if( dl!=std::string::npos && dl>sl) return path.substr(sl+1,dl-sl-1);
    return path.substr(sl+1);
  }
  else if( dl!=std::string::npos) return path.substr(0,dl);
  else return path;
}
inline std::string prepend(const std::string& prefix, const std::string& path) { // removes file suffix
  auto sl=path.find_last_of('/');
  if (sl == std::string::npos) return prefix+path;
  return path.substr(0,sl+1)+prefix+path.substr(sl+1);
}


template<class T> bool _1At(T n, int ibgn)  {  return n&(1<<ibgn);  }
inline            bool _1At(unsigned int n, int ibgn)  {  return n&(1<<ibgn);  }// for debugger otherwise redundant
template<class T> bool _1In(T n, int ibgn, int lnt)  {  return (n>>ibgn)&((1<<lnt)-1);  }





// CLEAN UP PLEASE:

inline double roundec(double x,int d)  { double scale=std::pow(10,int(std::log10(x)))/std::pow(10,d); return std::round(x/scale)*scale; } // round x to d significant digits

template<class T> Vars<T>  diffVars(piece<T> vs) { Vars<T> rt(vs);  piece<T>(rt.d+1,rt.dn)-=vs; rt[0]=rt[1]; return rt; }
template<class T> Vars<T>  movingAvg(piece<T> vs) { // apply rolling 3 moving average X <- ( X-- + X + X++)/3
  Vars<T> rt(vs.size());  T* d=rt.d; *d=0.5*(vs[0]+vs[1]);
  for(T* o=vs.d+2; o<vs.dn;++o) { *(++d)=(1./3.)*(*(o-2)+*(o-1)+*o); }
  *(++d)=0.5*(*(vs.dn-2)+*(vs.dn-1)); return rt; } // TODO: use vs storage instead of o

template<class T> T closer(T cv, T v1, T v2) { return (abs(cv-v1)<abs(cv-v2)) ?  v1 : v2; }


// TODO test BC handling
template<class T> Vars<T>  biMovingAvg(piece<T> vs,int krnl, int bKrnl=1) { // apply rolling 3 moving average X <- ( X-- + X + X++)/3
  if(vs.size()<7)  return vs;
  Vars<T> res(vs.size());  T* rt=res.d-1;  const double pl=1./(1+krnl);    bKrnl=std::min(bKrnl,krnl);
  for(int i=0; i<krnl; ++i)  { int krp = std::max(i,bKrnl)+1; *++rt=sumq(piece<T>(vs.d,krp)/krp); }
  vs.dn-=krnl; T* o=vs.d+krnl;
  for(; o<vs.dn;++o) { *++rt=closer(*o, sumq(piece<T>(o-krnl,1+krnl))*pl, sumq(piece<T>(o,1+krnl))*pl); }  --o; vs.dn+=krnl;
  for(int i=krnl; i>0; --i) { int krp = std::max(i,bKrnl)+1;  *++rt=sumq(piece<T>(vs.dn-krp, krp))/krp; }
  return res;
}

template<class T> void  NaNsToMean(piece<T> vs) {
  double sum=0.; size_t count=0;
  for(auto v:vs)  if(v==v) { sum+=v; ++count; } //not NaNs
  T mean = sum/count;
  for(auto& v:vs)  if(!(v==v)) { v=mean; } //NaNs
}


template<class T> T med_closer(vars<T> vs, T orig)  {
  if ((vs.size()&1)==0) {
     const auto nth = vs.begin() + vs.size()/2 - 1;
     std::nth_element(vs.begin(),   nth , vs.end());   const auto e1 = *nth;
     std::nth_element(vs.begin(), ++nth , vs.end());   const auto e2 = *nth;
     return closer(orig, e1, e2);
  } else { const auto nth = vs.begin()+vs.size()/2;   std::nth_element(vs.begin(), nth, vs.end());    return *nth;  }
}
template<class T> T med_odd(vars<T> vs)  { const auto nth = vs.begin()+vs.size()/2;  std::nth_element(vs.begin(), nth, vs.end());  return *nth;  }
template<class T> Vars<T>  median(piece<T> vs,int krnl, int bKrnl=1) { // apply rolling 3 moving average X <- ( X-- + X + X++)/3
  if(vs.size()<7)  return vs;
  Vars<T> res(vs.size());  T* rt=res.d-1;      bKrnl=std::min(bKrnl,krnl);
  for(int i=0; i<krnl; ++i)   *++rt=med_odd(vars<T>(vs.d,2*std::max(i,bKrnl)+1));
  vs.dn-=krnl; T* o=vs.d+krnl;
  for(; o<vs.dn;++o) { *++rt=med_odd(vars<T>(o-krnl,1+2*krnl)); }  --o; vs.dn+=krnl;  // vars -> piece ?
  for(int i=krnl; i>0; --i) { int krn = std::max(i,bKrnl);  *++rt=med_odd(vars<T>(vs.dn-2*krn-1, 2*krn+1)); }
  return res;
}



#ifdef VMMLIB__VECTOR__HPP
 template< size_t M, typename T >
 Vctr<M,T>::Vctr(const var3<T>& v3)   { array[ 0 ] = v3.x;  array[ 1 ] = v3.y;  array[ 2 ] = v3.z; }
#endif




template<class T>
vars<dbls> distribution(const piece<T>  xs, const piece<T> ws, int nBins=64)  {
  vars<dbls>  distrib(3, dbls(nBins,0.));
  double minU=min(xs),   maxU=max(xs),   deltaU=(maxU-minU)/nBins+1e-72;

  for (int i=0; i<nBins; ++i)  distrib[0][i] = minU+deltaU/2+i*deltaU;

  for (size_t i=0; i<ws.size(); ++i)  {
    int distInd=std::min(int((xs[i]-minU)/deltaU+0.5),nBins-1);
    ++distrib[1][distInd];
    distrib[2][distInd]+=ws[i];
  }
  distrib[1]/=distrib[1].sum()*deltaU;
  distrib[2]/=distrib[2].sum()*deltaU;

  return distrib;
}


inline double linearInterpolate(double x, double x1, double x2, double y1, double y2)    {    return y1+(x-x1)*(y2-y1)/(x2-x1);    }

inline double averageCDF(double xMin, double xMax, const std::vector<dbl2>& tabl)  {
  /// used to get average of a part of a distribution
  auto itr = tabl.begin();
  while((++itr)->a <=  xMin && itr != tabl.end()) ;

  if( !(itr->a >= xMin && (itr-1)->a <=  xMin))  {
    std::cout<<"\n LookUp Error:  "  << xMin-1.<<"     " << tabl.begin()->a-1.<<"  "  << tabl.begin()->b<<"    " << tabl.rbegin()->a-1.<<"  " << tabl.rbegin()->b
       <<"\n\n "<<(itr == tabl.end())<<std::endl;   }

  assert(itr->a > xMin && (itr-1)->a <=  xMin);

  double wSum=itr->a-xMin;
  double wxy1=(itr->a-xMin)*0.5*(itr->b+linearInterpolate(xMin,(itr-1)->a, itr->a, (itr-1)->b, itr->b));

  while((++itr)->a <=  xMax && itr != tabl.end())  {
    wSum+=itr->a-(itr-1)->a;
    wxy1+=(itr->a-(itr-1)->a)*0.5*(itr->b+(itr-1)->b);
  }
  if(itr != tabl.end())  {
    wSum+=itr->a-xMax;
    wxy1+=(itr->a-xMax)*0.5*(itr->b+linearInterpolate(xMin,(itr-1)->a, itr->a, (itr-1)->b, itr->b));
  }

  return wxy1/(wSum+1e-64);
}


inline double variance(const piece<double>& vs, piece<double> ws)  {  return sumSqrs(vs-vs.avg(), ws)/sum(ws);  } // biased
template<class T, class T2>
double corelCoeff(piece<T> X, piece<T> Y, piece<T2> ws) { return sum((X-X.avg())*(Y-Y.avg())*ws) / sqrt(sumSqrs(X-X.avg(), ws) * sumSqrs(Y-Y.avg(), ws)); }
//template<class T>
//double covarianceDbl(piece<T> X, piece<T> Y) { return sumdbl((X-X.avgdbl())*(Y-Y.avgdbl())) / (X.size()-1); }
template<class T>
double   varianceDbl(piece<T> X) { return sumdblSqrs(X-sumdbl(X)/X.size()) / (X.size()-1); }


inline double sqr(double a) { return a*a; }

inline double logTrans(double a, double s=1e+16) { return ((0.<a)-(a<0.))*std::log10(std::abs(a*s)+1.); }
inline double expTrans(double a, double s=1e-16) { return ((0.<a)-(a<0.))*(std::pow(10.,std::abs(a))-1.)*s; }

inline dbls logTrans(const piece<double>& vs, double s=1e+16) {    dbls rt(vs);   for(auto& v:rt) v=logTrans(v,s);   return rt;   }


#undef _T_
#endif //TYPSES_OERATIONS_H




/* //- Debugging, in Linux:
 Edit file: /usr/share/gcc/python/libstdcxx/v6/printers.py, for gdb pretty printing
class varsPrinter:
    "Print a vars"

    class _iterator(Iterator):
        def __init__ (self, start, finish):
            self.item = start
            self.finish = finish
            self.count = 0

        def __iter__(self):
            return self

        def __next__(self):
            count = self.count
            self.count = self.count + 1

            if self.item == self.finish:
               raise StopIteration
            elt = self.item.dereference()
            self.item = self.item + 1
            return ('[%d]' % count, elt)

    def __init__(self, typename, val):
        self.typename = typename
        self.val = val

    def children(self):
        return self._iterator(self.val['d'], self.val['dn'])

    def to_string(self):
        start = self.val['d']
        finish = self.val['dn']
        return ('%s of length %d' % (self.typename, int (finish - start)))

    def display_hint(self):
        return 'array'


in build_libstdcxx_dictionary, add:
    libstdcxx_printer.add_container('', 'piece', varsPrinter)
    libstdcxx_printer.add_container('', 'Vars', varsPrinter)
*/
