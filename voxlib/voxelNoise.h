#pragma once
/*-------------------------------------------------------------------------*\
Add surface roughness / change surface morphology

Developed by:
 - Ali Raeini (2021)
 - Luke Giudici (2021)
\*-------------------------------------------------------------------------*/

#include <cstdint>
#include "globals.h"
#include "voxelImage.h"

#define forAll_kji_m_seq(_nNei,_vxls)   \
   for (int k=(_vxls).nz()-_nNei-1; k>=_nNei; --k)   \
   for (int j=(_vxls).ny()-_nNei-1; j>=_nNei; --j)   \
   for (int i=(_vxls).nx()-_nNei-1; i>=_nNei; --i)


// taken from https://en.wikipedia.org/wiki/Xorshift

//inline uint64_t xorshift64(uint64_t &x)  {  x^= x<<13;   x^= x>>7;    x^= x<<17;   return x;  }
#define xorshift64(_x)  (_x^= (_x^= (_x^= _x<<13)>>7)<<17) //*UINT64_C(0x2545F4914F6CDD1D))
//Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs":
#define xorshift32(_x)  (_x^= (_x^= (_x^= _x<<13)>>17)<<5) //*0x25454F1D)

// The state array must be initialized to not be all zero
struct xorshift128_state {  uint32_t a, b, c, d;  };
inline uint32_t xorshift128(struct xorshift128_state& st)
{
  uint32_t t = st.d;  // Algorithm "xor128" from p. 5 of Marsaglia, "Xorshift RNGs"
  uint32_t const a = st.a;
  st.d = st.c;   st.c = st.b;   st.b = a;
  t ^= t<<11;    t ^= t>>8;
  return st.a = t ^ a ^ (a >> 19);
}

template<typename T>
size_t addSurfNoise(voxelImageT<T>& img, const int randMask1, const int randMask2, int trsh, int randseed=-1)
{ /// \param randMask1 randMask2: mask for rand to equal for changing vxels, adjust to get desired probability
  using namespace std;


  if (randseed<0)  srand(int(time(NULL)));
  else              srand(randseed);
  uint64_t seed=rand();

  cout<<"seed"<<seed<<endl;
  {
    voxelImageT<T> vxls=img;
    vxls.growBox(2);
    const T* v0=&vxls(0,0,0);
    size_t nChanges(0);
    OMPFor(reduction(+:nChanges))
    forAllkji_1(img) {
      //T* vp=&vxls(i,j,k); T * vj=&vxls.v_i(+1,vp); _addNois_

      T* vp=&vxls(i+2,j+2,k+2);
      int rnd=vp-v0+seed; xorshift32(rnd); xorshift32(rnd); xorshift32(rnd); // use   int rnd=xorshift64(seed);  to increase entropy and make rand a function of number of processors (unreproducible)
      auto sameBits = [rnd](int mask)  { return (rnd&(mask<<(rnd&7)))==(mask<<(rnd&7)); };
      if(sameBits(randMask1) || sameBits(randMask2)) {

        auto addnoise = [&vxls, &img, i,j,k, vp, trsh, &nChanges](int ii, int jj, int kk) {
          T* vj=&vxls(i+ii, j+jj, k+kk);
          if(*vp!=*vj) {
            int nSam40=nSam6Nei(vxls,vj)+nSam6Nei2(vxls,vj)-1;
            forAllNei(-1,1)  if(_nei(vxls,i+ii,j+jj,k+kk)==*vj) ++nSam40;
            if(trsh<=nSam40)  { img(i,j,k)=*vj; ++nChanges; return 1; } }
          return 0; };

        if(addnoise(1,2,2)) continue;
        if(addnoise(2,1,2)) continue;
        if(addnoise(2,2,1)) continue;
      }
    }
    std::cout<<"addSurfNoise  nChanges: "<<nChanges<<" ";
  }

  {
    voxelImageT<T> vxls=img;
    vxls.growBox(2);
    const T* v0=&vxls(0,0,0);
    size_t nChanges(0);
    OMPFor(reduction(+:nChanges))
    forAll_kji_m_seq(1,img) { // loop in reverse order _kji
      //T* vp=&vxls(i,j,k); T * vj=&vxls.v_i(+1,vp); _addNois_

      T* vp=&vxls(i+2,j+2,k+2);
      int rnd=vp-v0+seed+1; xorshift32(rnd); xorshift32(rnd); xorshift32(rnd); // use   int rnd=xorshift64(seed);  to increase entropy and make rand a function of number of processors (unreproducible)
      auto sameBits = [rnd](int mask)  { return (rnd&(mask<<(rnd&7)))==(mask<<(rnd&7)); };
      if(sameBits(randMask1) || sameBits(randMask2)) {

        auto addnoise = [&vxls, &img, i,j,k, vp, trsh, &nChanges](int ii, int jj, int kk) {
          T* vj=&vxls(i+ii, j+jj, k+kk);
          if(*vp!=*vj) {
            int nSam40=nSam6Nei(vxls,vj)+nSam6Nei2(vxls,vj)-1;
            forAllNei(-1,1)  if(_nei(vxls,i+ii,j+jj,k+kk)==*vj) ++nSam40;
            if(trsh<=nSam40)  { img(i,j,k)=*vj; ++nChanges; return 1; } }
          return 0; };

        if(addnoise(3,2,2)) continue;
        if(addnoise(2,3,2)) continue;
        if(addnoise(2,2,3)) continue;
      }
    }
    std::cout<<"+ "<<nChanges<<endl;
  }

  return 0;
}


namespace MCTProcessing {

  template<typename T>
  bool addSurfNoise( std::stringstream& ins, voxelImageT<T>& vImg)  {
    KeyHint("randMask1 randMask2 nSam40Trsh nItr randSeed");
    int randMask1=0x00000003; ins>>randMask1; //!< mask of random number controlling probability
    int randMask2=randMask1;     ins>>randMask2;
    int nSam40Trsh=15;     ins>>nSam40Trsh;
    int nItr=1;              ins>>nItr;
    int randSeed=-1000;           ins>>randSeed;
    std::cout<<"addSurfNoise:   "<<randMask1<<' '<<randMask2<<' '<<nSam40Trsh<<' '<<nItr<<' '<<randSeed<<std::endl;

    randMask1=randMask1<<2;
    randMask2=randMask2<<2;
    for_i_(0,nItr) addSurfNoise(vImg,randMask1,randMask2,nSam40Trsh, randSeed+i*10);

    return true;
  }


} // namespace MCTProcessing
