#pragma once
/*

This file is part of libvoxel, a C++ template library for handelling 3D images.

Developed by:
 - Ali Q Raeini (2010-2022)

*/

#include <map>
#include <array>

#include "globals.h"
#include "voxelImage.h"
#include "voxelImageI.h"

using namespace std;




                namespace MCTProcessing _begins_

template<typename T>  bool variance(stringstream& ins, voxelImageT<T> & img)  {
  KeyHint("minV maxV ");
  int bgn(0),end(maxT(T)); ins>>bgn>>end;
  cout<<" "<<bgn<<" "<<end<<": var = "<<varianceDbl(img,bgn,end);
  return true;
}

                _end_of_(namespace MCTProcessing)





#define _2pi  6.283185306



#define _nei1(_vxls,i_M,j_M,k_M) (_vxls)(i_M+i_nei_rel%2, j_M+j_nei_rel%2, k_M+k_nei_rel%2)
#define distSqrNei() (k_nei_rel*k_nei_rel+j_nei_rel*j_nei_rel+i_nei_rel*i_nei_rel)

#define getnNeissumNeis(nNeis_,sumNeis_, _vxls,vp_,nW_, lowerB_, upperB_) \
    {sumNei3IIf(nNeis_,sumNeis_, vp_,lowerB_, upperB_);\
     sumNei3IIf(nNeis_,sumNeis_, _vxls.p_j(-nW_,vp_),lowerB_, upperB_);\
     sumNei3IIf(nNeis_,sumNeis_, _vxls.p_j( nW_,vp_),lowerB_, upperB_);\
     sumNei3IIf(nNeis_,sumNeis_, _vxls.p_k(-nW_,vp_),lowerB_, upperB_);\
     sumNei3IIf(nNeis_,sumNeis_, _vxls.p_k( nW_,vp_),lowerB_, upperB_);\
     sumNei3IIf(nNeis_,sumNeis_, _vxls.p_k(-nW_, _vxls.p_j(-nW_,vp_)),lowerB_, upperB_);\
     sumNei3IIf(nNeis_,sumNeis_, _vxls.p_k( nW_, _vxls.p_j( nW_,vp_)),lowerB_, upperB_);\
     sumNei3IIf(nNeis_,sumNeis_, _vxls.p_k(-nW_, _vxls.p_j( nW_,vp_)),lowerB_, upperB_);\
     sumNei3IIf(nNeis_,sumNeis_, _vxls.p_k( nW_, _vxls.p_j(-nW_,vp_)),lowerB_, upperB_); }
template<typename T> void sumNei3IIf(int& nNeis,Tint& sumNeis, const T* vp,T l, T u)  {
  T vv=*(vp-1); if(l<=vv && vv<=u) {++nNeis; sumNeis+=vv;}
    vv=*(vp  ); if(l<=vv && vv<=u) {++nNeis; sumNeis+=vv;}
    vv=*(vp+1); if(l<=vv && vv<=u) {++nNeis; sumNeis+=vv;}  }


#define neis_countE_m_(j_m,k_m) \
  neiPID = &(vxls(i-1,j_m,k_m));  \
  if (*(  neiPID) == pID  ) ++nSames; \
  if (*(++neiPID) == pID  ) ++nSames; \
  if (*(++neiPID) == pID  ) ++nSames;

#define neis_insert_neiPID_m_(j_m,k_m) \
  neiPID = &(vxls(i-1,j_m,k_m));  \
  if (*(  neiPID) != pID  )    ++(neis.insert(std::pair<T,short>(*neiPID,0)).first->second); \
  if (*(++neiPID) != pID  )    ++(neis.insert(std::pair<T,short>(*neiPID,0)).first->second); \
  if (*(++neiPID) != pID  )    ++(neis.insert(std::pair<T,short>(*neiPID,0)).first->second);

template<typename T>
void mode26(voxelImageT<T>& vImg, short minDif)  {
  const short minDifp2=minDif/2;
  voxelImageT<T> vxls=vImg;
    forAllkji_1_(vxls)
  {
    T* vp = &(vxls(i,j,k));
    const T pID = *vp;

    short nSames(0);
    T* neiPID;


    { neis_countE_m_(j-1,k-1) }
    { neis_countE_m_(j  ,k-1) }
    { neis_countE_m_(j+1,k-1) }
    { neis_countE_m_(j-1,k  ) }
    { neis_countE_m_(j  ,k  ) }
    { neis_countE_m_(j+1,k  ) }
    { neis_countE_m_(j-1,k+1) }
    { neis_countE_m_(j  ,k+1) }
    { neis_countE_m_(j+1,k+1) }


    if(nSames+minDifp2<14)
    {
      std::map<T,short> neis;///.  ID-counter

      { neis_insert_neiPID_m_(j-1,k-1) }
      { neis_insert_neiPID_m_(j  ,k-1) }
      { neis_insert_neiPID_m_(j+1,k-1) }
      { neis_insert_neiPID_m_(j-1,k  ) }
      { neis_insert_neiPID_m_(j  ,k  ) }
      { neis_insert_neiPID_m_(j+1,k  ) }
      { neis_insert_neiPID_m_(j-1,k+1) }
      { neis_insert_neiPID_m_(j  ,k+1) }
      { neis_insert_neiPID_m_(j+1,k+1) }

       auto neitr = std::max_element(neis.begin(), neis.end(), mapComparer<T>());
       if (neitr->second > nSames+minDif)
       {
        vImg(i,j,k) = neitr->first;
       }
     }
  }
}


template<typename T>
void mode26(voxelImageT<T>& vImg, T minv,T maxv, short minDif)  {
  voxelImageT<T> vxls=vImg;
  forAllkji_1_(vxls)  {
   T* vp = &(vxls(i,j,k));
   const T pID = *vp;
   if(minv<=pID && pID<=maxv)  {
    short nSames(0);
    T* neiPID;

    { neis_countE_m_(j-1,k-1) }
    { neis_countE_m_(j  ,k-1) }
    { neis_countE_m_(j+1,k-1) }
    { neis_countE_m_(j-1,k  ) }
    { neis_countE_m_(j  ,k  ) }
    { neis_countE_m_(j+1,k  ) }
    { neis_countE_m_(j-1,k+1) }
    { neis_countE_m_(j  ,k+1) }
    { neis_countE_m_(j+1,k+1) }


    if(nSames+minDif/2<14)  {
      std::map<T,short> neis;///.  ID-counter

      { neis_insert_neiPID_m_(j-1,k-1) }
      { neis_insert_neiPID_m_(j  ,k-1) }
      { neis_insert_neiPID_m_(j+1,k-1) }
      { neis_insert_neiPID_m_(j-1,k  ) }
      { neis_insert_neiPID_m_(j  ,k  ) }
      { neis_insert_neiPID_m_(j+1,k  ) }
      { neis_insert_neiPID_m_(j-1,k+1) }
      { neis_insert_neiPID_m_(j  ,k+1) }
      { neis_insert_neiPID_m_(j+1,k+1) }

       auto neitr = std::max_element(neis.begin(), neis.end(), mapComparer<T>());
       if (neitr->second >= nSames+minDif)
        (vImg)(i,j,k) = neitr->first;
     }
   }
  }
}




template<typename T>
void curtail(voxelImageT<T>& vImg, T mint, T minv, T maxt, T maxv) {
  forAllkji_(vImg)
  {
    if (vImg(i,j,k)<mint) vImg(i,j,k) = minv;
    else
    if (vImg(i,j,k)>maxt) vImg(i,j,k) = maxv;
  }
}



template<typename T>
void growingThreshold(  voxelImageT<T>& vImg,
  T StartThreshold1,
  T StartThreshold2,
  T finalThreshold1,
  T finalThreshold2,
  const int nIter  ) {
  voxelImageT<T> tmpFinal=vImg;
  vImg.threshold101(StartThreshold1, StartThreshold2);
  vImg.fillHoles(2);
  voxelImageT<T> tmpStart=vImg;

  tmpFinal.threshold101(finalThreshold1, finalThreshold2);
  tmpFinal.FaceMedian06(1,5);
  tmpFinal.FaceMedian06(1,5);
  tmpFinal.FaceMedian06(1,5);
  for (int i=0; i<nIter; ++i) {
    vImg.growPore();  vImg.OR(tmpFinal); }

  vImg.AND(tmpStart);
}


template<typename T>
void replaceRangeByImage(voxelImageT<T>& vImg, T minv,T  maxv, const voxelImageT<T>& vxls) {
  ensure(vImg.size3()==vxls.size3());
  forAllkji_(vImg)  if (T vv=vImg(i,j,k);  minv<=vv && vv<=maxv)  vImg(i,j,k)=vxls(i,j,k);
}

template<typename T>
void replaceByImageRange(voxelImageT<T>& vImg, T minv,T  maxv, const voxelImageT<T>& vxls) {
  ensure(vImg.size3()==vxls.size3());
  forAllkji_(vImg)  if (T vv=vxls(i,j,k);  minv<=vv && vv<=maxv)  vImg(i,j,k)=vv;
}





template<typename T>
voxelImageT<T> magGradient(const voxelImageT<T>& vImg, double diffuseLSqr) {
  cout<<" magGrad ";
  voxelImageT<T> grad=vImg;

  voxelImageT<T> vxls=vImg;
  vxls.growBox(1);

  forAllkji_1_(vxls)  {
    unsigned int gcmp = vxls(i+1,j,k)-vxls(i-1,j,k);
    unsigned int neiSum = gcmp*gcmp;
    gcmp = vxls(i,j+1,k)-vxls(i,j-1,k);
    neiSum += gcmp*gcmp;
    gcmp = vxls(i,j,k+1)-vxls(i,j,k-1);
    neiSum += gcmp*gcmp;

    grad(i-1,j-1,k-1)=std::min(T(sqrt(neiSum*diffuseLSqr)+0.5),maxT(T));
  }

    return grad;
}



template<typename T>
void bilateralX(voxelImageT<T>& vImg, int kernRad, int Xstp,
                float sigmaSqr=16, float sharpFact=0.05, float sigmaGaussSqr=2.,
                float capmin=0, float capmax=fmaxT(T)) {
    (std::cout<<" bilateralX  ").flush();


  float p1rSigma=1./sigmaSqr;
  float p1rGSigma=1./sigmaGaussSqr;
  voxelImageT<T> vxls=vImg;

  OMPFor()
  for (int k=kernRad; k<vxls.nz()-kernRad; ++k)
  {
    if (k%(vxls.nz()/20) == 0) (std::cout<<".").flush();
    for (int j=kernRad; j<vxls.ny()-kernRad; ++j)
    {
      for (int i=kernRad; i<vxls.nx()-kernRad; ++i)
      {
        float  vv=vxls(i,j,k);
        if(capmin<=vv && vv<=capmax)
        {
          float  neidiffSum(0.0f);
          float  neiSumWei(1e-15);
          float  neidiffGSum(0.0f);// G->Gauss (no bilateral)
          float  neiSumGWei(1e-15);
          float  neidiffMSum(0.0f);// M->Max
          float  neiSumMWei(-0.9);
          forAllNei_m_(Xstp,-kernRad,kernRad)
          {
            float dist2Nei=distSqrNei();
            float  nv=_nei(vxls,i,j,k)-vv;
            float  weit=1.0f/(1.0f+nv*nv*p1rSigma+dist2Nei*p1rGSigma);
            neidiffSum+=weit*nv;
            neiSumWei+=weit;
            weit*=(nv>=-1e-32);
            neidiffMSum+=weit*nv;
            neiSumMWei+=weit;
            weit=std::min(nv*nv,sigmaSqr)/(0.01f+dist2Nei*p1rGSigma);
            neidiffGSum+=weit*nv;
            neiSumGWei+=weit;
          }
          float neu= (vv+neidiffSum/(neiSumWei)*(1.0f+sharpFact)-sharpFact*neidiffGSum/(neiSumGWei)+0.49f);
          neu = std::max(capmin,std::min(capmax,neu));
          neu = std::max(vv+(neidiffSum-neidiffMSum)/(1+neiSumWei-neiSumMWei),std::min(vv+neidiffMSum/neiSumMWei,neu));

          vImg(i,j,k)=neu;
        }
      }
    }
  }
}

template<typename T>  bool _bilateralX(voxelImageT<T>& vImg, int nItrs, int kernRad, int Xstp, double sigmavv, double sharpFact, double sigmadd) {
  cout<<"\n bilateralX    nIterations  kernRad, Xstp/2   sigmavv,   sharpFact,  sigmadd  \n";
  cout<<"\n bilateralX    "<<nItrs <<"  "<<kernRad<<" "<<Xstp<<"  "<<sigmavv <<" "<<sharpFact<<" "<<sigmadd<<"\n";
  vImg.growBox(kernRad*Xstp+4);
  for (int i=nItrs-1; i>=0; --i)
  {
    const int stpi=(Xstp+1)/2+(i%(Xstp+1)); //! Xstp =2-> 1,2,3,   =3-> 2,3,4(,5)
    cout<<" ks"<<kernRad*stpi<<"/s"<<stpi<<" ";
    bilateralX(vImg, kernRad*stpi,stpi, sigmavv, sharpFact, sigmadd);
  }
  vImg.shrinkBox(kernRad*Xstp+4);
  cout<<endl;
  return true;
}



template<typename T>
void bilateralGauss(voxelImageT<T>& vImg, int kernRad, float sigmaSqr=16, float sharpFact=0.05,
                    float sigmaGaussSqr=2., float capmin=0, float capmax=fmaxT(T))
{
    (std::cout<<" bilateralGauss  ").flush();


  float p1rSigma=1./sigmaSqr;
  float p1rGSigma=1./sigmaGaussSqr;
  voxelImageT<T> vxls=vImg;

  OMPFor()
  for (int k=kernRad; k<vxls.nz()-kernRad; ++k)
  {
    if (k%(vxls.nz()/20) == 0) (std::cout<<".").flush();
    for (int j=kernRad; j<vxls.ny()-kernRad; ++j)
    {
      for (int i=kernRad; i<vxls.nx()-kernRad; ++i)
      {
        float  vv=vxls(i,j,k);
        if(capmin<=vv && vv<=capmax)
        {
          float  neidiffSum(0.);
          float  neiSumWei(1e-18);
          float  neidiffGSum(0.);
          float  neiSumGWei(1e-18);
          forAllNei(-kernRad,kernRad)
          {
            float dist2Nei=distSqrNei();
            float  nv=_nei(vxls,i,j,k)-vv;
            float  weit=1./(1.+nv*nv*p1rSigma+dist2Nei*p1rGSigma);
            neidiffSum+=weit*nv;
            neiSumWei+=weit;
            weit=std::min(nv*nv,sigmaSqr)*dist2Nei;
            neidiffGSum+=weit*nv;
            neiSumGWei+=weit;
          }

          vImg(i  ,j,k)=std::max(capmin,std::min(capmax,(vv+neidiffSum/(neiSumWei)*(1.0f+sharpFact)-sharpFact*neidiffGSum/(neiSumGWei)+0.5f)));
        }
      }
    }
  }
}

template<typename T>  bool _bilateralGauss(voxelImageT<T>& vImg, int nItrs, int kernRad, double sigmavv, double sharpFact, double sigmadd) {
  cout<<"\nsmoothing:   nIterations:"<<nItrs <<", kernRad:"<<kernRad<<"  sigmavv:"<<sigmavv <<", sharpFact:"<<sharpFact<<", sigmadd:"<<sigmadd<<"\n";
  vImg.growBox(kernRad+2);
  for (int i=0; i<nItrs; ++i)  {
    cout<<"  ";
    bilateralGauss(vImg, kernRad, sigmavv, sharpFact, sigmadd);
  }
  vImg.shrinkBox(kernRad+2);

  cout<<endl;
  return true;
}


template<typename T>
void biLateral(voxelImageT<T>& vImg, int kernRad, double sigmad, double sharpFact) {
    (std::cout<<"  bilat ").flush();

  voxelImageT<T> vxls=vImg;
  double  sigma = sigmad;
  for (int k=kernRad; k<vxls.nz()-kernRad; ++k)
  {
    if (k%(vxls.nz()/20) == 0) (std::cout<<".").flush();
    for (int j=kernRad; j<vxls.ny()-kernRad; ++j)
    {
      for (int i=kernRad; i<vxls.nx()-kernRad; ++i)
      {
        double  vv=vxls(i,j,k);
        double  neidiffSum(0.);
        double  neiSumWei(0.);
        forAllNei(-kernRad,kernRad)
        {
            double  nv=_nei(vxls,i,j,k)-vv;
            double  weit=1./(1.+nv*nv/sigma);
            neidiffSum+=weit*nv;
            neiSumWei+=weit;
        }

        neidiffSum = std::max(-vv,std::min(dmaxT(T)-vv,(neidiffSum/(neiSumWei-1.+1e-12)*(1.+sharpFact) )));

        int deltap6 = neidiffSum/6.+0.5;
        vImg(i  ,j,k)=std::max(0,std::min(imaxT(T),int(vImg(i  ,j,k))+deltap6*6));
        vImg(i-1,j,k)=std::max(0,std::min(imaxT(T),int(vImg(i-1,j,k))-deltap6));
        vImg(i+1,j,k)=std::max(0,std::min(imaxT(T),int(vImg(i+1,j,k))-deltap6));
        vImg(i,j-1,k)=std::max(0,std::min(imaxT(T),int(vImg(i,j-1,k))-deltap6));
        vImg(i,j+1,k)=std::max(0,std::min(imaxT(T),int(vImg(i,j+1,k))-deltap6));
        vImg(i,j,k-1)=std::max(0,std::min(imaxT(T),int(vImg(i,j,k-1))-deltap6));
        vImg(i,j,k+1)=std::max(0,std::min(imaxT(T),int(vImg(i,j,k+1))-deltap6));

      }
    }
  }
}

template<typename T>
void biLateralIterative(voxelImageT<T>& vImg, const int kernRad, const double sigmad, const double sharpFact) {
    (std::cout<<"  bilat ").flush();

  const voxelImageT<T> vxlsO=vImg;
  const double  sigma = sigmad;
  for(int iKer=0; iKer<kernRad; ++iKer)
  {
   (std::cout<<":").flush();
   const voxelImageT<T> vxls=vImg;
   for (int k=1; k<vxls.nz()-1; ++k)
    {
    if (k%(vxls.nz()/10) == 0) (std::cout<<".").flush();
        OMPFor()
        for (int j=1; j<vxls.ny()-1; ++j)
         for (int i=1; i<vxls.nx()-1; ++i)
         {
          const double  vvO=vxlsO(i,j,k);
          const T*  vp=&vxls(i,j,k);
          const double  vv=*vp;
          double  neidiffSum(0.);
          double  neiSumWei(1e-12);
          //double  sumWDn(1e-12);
          //double  sumWUp(1e-12);

          double  nv;        double  weit;
          nv=vxls.v_i(-1,vp)-vvO;  weit=1./(1.+nv*nv/sigma); nv+=vvO-vv;    neidiffSum+=weit*nv;  neiSumWei+=weit; //sumWUp+=max(nv,0.); sumWDn-=min(nv,0.);
          nv=vxls.v_i( 1,vp)-vvO;  weit=1./(1.+nv*nv/sigma); nv+=vvO-vv;    neidiffSum+=weit*nv;  neiSumWei+=weit; //sumWUp+=max(nv,0.); sumWDn-=min(nv,0.);
          nv=vxls.v_j(-1,vp)-vvO;  weit=1./(1.+nv*nv/sigma); nv+=vvO-vv;    neidiffSum+=weit*nv;  neiSumWei+=weit; //sumWUp+=max(nv,0.); sumWDn-=min(nv,0.);
          nv=vxls.v_j( 1,vp)-vvO;  weit=1./(1.+nv*nv/sigma); nv+=vvO-vv;    neidiffSum+=weit*nv;  neiSumWei+=weit; //sumWUp+=max(nv,0.); sumWDn-=min(nv,0.);
          nv=vxls.v_k(-1,vp)-vvO;  weit=1./(1.+nv*nv/sigma); nv+=vvO-vv;    neidiffSum+=weit*nv;  neiSumWei+=weit; //sumWUp+=max(nv,0.); sumWDn-=min(nv,0.);
          nv=vxls.v_k( 1,vp)-vvO;  weit=1./(1.+nv*nv/sigma); nv+=vvO-vv;    neidiffSum+=weit*nv;  neiSumWei+=weit; //sumWUp+=max(nv,0.); sumWDn-=min(nv,0.);

          //neidiffSum = max(-vv,min(dmaxT(T)-vv,(neidiffSum/(neiSumWei)*(1.+sharpFact) )));
          //int deltap6 = neidiffSum/6.+0.5;
          //vImg(i  ,j,k)=max(0,min(255,vImg(i  ,j,k)+deltap6*6));
          //vImg(i-1,j,k)=max(0,min(255,vImg(i-1,j,k)-deltap6));
          //vImg(i+1,j,k)=max(0,min(255,vImg(i+1,j,k)-deltap6));
          //vImg(i,j-1,k)=max(0,min(255,vImg(i,j-1,k)-deltap6));
          //vImg(i,j+1,k)=max(0,min(255,vImg(i,j+1,k)-deltap6));
          //vImg(i,j,k-1)=max(0,min(255,vImg(i,j,k-1)-deltap6));
          //vImg(i,j,k+1)=max(0,min(255,vImg(i,j,k+1)-deltap6));

          neidiffSum =(1.+sharpFact)*neidiffSum/neiSumWei;
          vImg(i,j,k)=std::max(0,std::min(imaxT(T),int(vImg(i,j,k))+int(neidiffSum+0.5)));

          //if(neidiffSum<-1.5)
          //{
            //sumWDn=neidiffSum/sumWDn*0.95;
            //vImg(i-1,j,k)=max(0,min(255,int(vImg(i-1,j,k))+min(int((vxls(i-1,j,k)-vv)*sumWDn),0)));
            //vImg(i+1,j,k)=max(0,min(255,int(vImg(i+1,j,k))+min(int((vxls(i+1,j,k)-vv)*sumWDn),0)));
            //vImg(i,j-1,k)=max(0,min(255,int(vImg(i,j-1,k))+min(int((vxls(i,j-1,k)-vv)*sumWDn),0)));
            //vImg(i,j+1,k)=max(0,min(255,int(vImg(i,j+1,k))+min(int((vxls(i,j+1,k)-vv)*sumWDn),0)));
            //vImg(i,j,k-1)=max(0,min(255,int(vImg(i,j,k-1))+min(int((vxls(i,j,k-1)-vv)*sumWDn),0)));
            //vImg(i,j,k+1)=max(0,min(255,int(vImg(i,j,k+1))+min(int((vxls(i,j,k+1)-vv)*sumWDn),0)));
           //}else if(neidiffSum>1.5){
            //sumWUp=neidiffSum/sumWUp*0.95;
            //vImg(i-1,j,k)=max(0,min(255,int(vImg(i-1,j,k))+max(int((vxls(i-1,j,k)-vv)*sumWUp),0)));
            //vImg(i+1,j,k)=max(0,min(255,int(vImg(i+1,j,k))+max(int((vxls(i+1,j,k)-vv)*sumWUp),0)));
            //vImg(i,j-1,k)=max(0,min(255,int(vImg(i,j-1,k))+max(int((vxls(i,j-1,k)-vv)*sumWUp),0)));
            //vImg(i,j+1,k)=max(0,min(255,int(vImg(i,j+1,k))+max(int((vxls(i,j+1,k)-vv)*sumWUp),0)));
            //vImg(i,j,k-1)=max(0,min(255,int(vImg(i,j,k-1))+max(int((vxls(i,j,k-1)-vv)*sumWUp),0)));
            //vImg(i,j,k+1)=max(0,min(255,int(vImg(i,j,k+1))+max(int((vxls(i,j,k+1)-vv)*sumWUp),0)));
          //}
      }
    }
  }
  (std::cout<<"\n").flush();
}



/*//void biLateralWeighted(voxelImageT<T>& vImg, voxelImageT<T>& grads, double sigmaVG, double sigmaVV, double sharpFact)
//{
//
//    (std::cout<<"  bilatw ").flush();
//  const voxelImageT<T> vxls = vImg;
//
//    for (int k=1; k<vxls.nz()-1; ++k)
//    {
//    if (k%20==10) (std::cout<<".").flush();
//        for (int j=1; j<vxls.ny()-1; ++j)
//        {
//            for (int i=1; i<vxls.nx()-1; ++i)
//            {
//        double  vv=vxls(i,j,k);
//                double neidiffSum(0.);
//                double neiSumWei(0.);
//                forAllNei(-1,1)
//                {
//            double  nv=_nei(vxls,i,j,k)-vv;//distSqrNei()
//            double  ng=_nei(grads,i,j,k);//distSqrNei()
//            double weit=1./(1.+nv*nv*ng*ng/sigmaVV/sigmaVG);
//                    neidiffSum+=weit*nv;
//                    neiSumWei+=weit;
//                }
//                //neidiffSum= neidiffSum/(neiSumWei-1.+1e-12) +vv;
//                //vImg(i,j,k)=max(0,min(255,int(neidiffSum+sharpFact*(neidiffSum-vv)+0.5)));
//
//          neidiffSum = std::max(-vv,std::min(dmaxT(T)-vv,(neidiffSum/(neiSumWei-1.+1e-12)*(1.+sharpFact) )));
//          int deltap6 = neidiffSum/6.+0.5;
//          vImg(i  ,j,k)=std::max(0,std::min(255,vImg(i  ,j,k)+deltap6*6));
//          vImg(i-1,j,k)=std::max(0,std::min(255,vImg(i-1,j,k)-deltap6));
//          vImg(i+1,j,k)=std::max(0,std::min(255,vImg(i+1,j,k)-deltap6));
//          vImg(i,j-1,k)=std::max(0,std::min(255,vImg(i,j-1,k)-deltap6));
//          vImg(i,j+1,k)=std::max(0,std::min(255,vImg(i,j+1,k)-deltap6));
//          vImg(i,j,k-1)=std::max(0,std::min(255,vImg(i,j,k-1)-deltap6));
//          vImg(i,j,k+1)=std::max(0,std::min(255,vImg(i,j,k+1)-deltap6));
//
//            }
//        }
//    }
//    (std::cout<<" ").flush();
//
//}*/


template<typename T>
void shrink(voxelImageT<T>& seged, T min,  T maks, const voxelImageT<T>& vImg) {

    (std::cout<<"  shrink:"<<int(min)<<"-"<<int(maks)<<" ").flush();
    const voxelImageT<T> vxls=seged;


  OMPragma("omp parallel for")
  forkji_be (1,0,0, vImg.nx(),vImg.ny(),vImg.nz()) {
    if (vxls(i,j,k) != vxls(i-1,j,k)) {
      if(seged(i-1,j,k)<=maks && seged(i-1,j,k)>=min ) seged(i-1,j,k)=vImg(i-1,j,k);
      if(seged(i,j,k)<=maks && seged(i,j,k)>=min ) seged(i,j,k)=vImg(i,j,k);
    }
  }

  OMPragma("omp parallel for")
  forkji_be (0,1,0, vImg.nx(),vImg.ny(),vImg.nz()) {
        if (vxls(i,j,k) != vxls(i,j-1,k)) {
      if(seged(i,j-1,k)<=maks && seged(i,j-1,k)>=min ) seged(i,j-1,k)=vImg(i,j-1,k);
      if(seged(i,j,k)<=maks && seged(i,j,k)>=min ) seged(i,j,k)=vImg(i,j,k);
    }
  }


   OMPragma("omp parallel for")
  forkji_be (0,0,1, vImg.nx(),vImg.ny(),vImg.nz()) {
    if (vxls(i,j,k) != vxls(i,j,k-1)) {
      if(seged(i,j,k-1)<=maks && seged(i,j,k-1)>=min ) seged(i,j,k-1)=vImg(i,j,k-1);
      if(seged(i,j,k)<=maks && seged(i,j,k)>=min ) seged(i,j,k)=vImg(i,j,k);
    }
  }
    (std::cout<<" ").flush();
}



template<typename T>
void grow(voxelImageT<T>& vImg, T midv, T minv,  T maxv,  voxelImageT<T>& later) {
  (std::cout<<int(minv)<<":"<<int(maxv)<<"<-"<<int(midv)<<"  | ").flush();

  voxelImageT<T> vxls = vImg;
  forAllkji_1_(vxls) {
    T vv = vxls(i,j,k);
    if (vv!=midv && minv<=vv && vv<=maxv && !later(i,j,k)) {
      forAllNei(-1,1) {
        if( midv==_nei(vxls,i,j,k) && _nei(later,i,j,k)) {
          vImg(i,j,k)=midv;
        }
      }
    }
  }
}

template<typename T>
void grow(voxelImageT<T>& vImg, T midv, T minv,  T maxv) {
  (std::cout<<int(minv)<<":"<<int(maxv)<<"<-"<<int(midv)<<"   ").flush();

  voxelImageT<T> vxls = vImg;
  forAllkji_1_(vxls)
  {
  T vv = vxls(i,j,k);
  if (vv!=midv && minv<=vv && vv<=maxv)
  {
    forAllNei(-1,1)
    {
      if( midv==_nei(vxls,i,j,k))
      {
        vImg(i,j,k)=midv;
      }
    }
  }
  }
}

template<typename T>
void mode(voxelImageT<T>& vImg, T min,  T max,  short nNeist=2,  T midv=maxT(T)) {
  const voxelImageT<T> vxls = vImg;
  //long long nChanges = 0;
  forAllkji_1_(vxls) {
    const T* vp = &vxls(i,j,k);
    T pID = *vp;

    if (pID == midv) {

    std::map<T,short> neis;///.  ID-counter

    T
    neiPID = vxls.v_i(-1,vp);
    if (neiPID != pID && min <= neiPID && max>= neiPID )    ++(neis.insert({neiPID,0}).first->second);
    neiPID = vxls.v_i( 1,vp);
    if (neiPID != pID && min <= neiPID && max>= neiPID )    ++(neis.insert({neiPID,0}).first->second);
    neiPID = vxls.v_j(-1,vp);
    if (neiPID != pID && min <= neiPID && max>= neiPID )    ++(neis.insert({neiPID,0}).first->second);
    neiPID = vxls.v_j( 1,vp);
    if (neiPID != pID && min <= neiPID && max>= neiPID )    ++(neis.insert({neiPID,0}).first->second);
    neiPID = vxls.v_k(-1,vp);
    if (neiPID != pID && min <= neiPID && max>= neiPID )    ++(neis.insert({neiPID,0}).first->second);
    neiPID = vxls.v_k( 1,vp);
    if (neiPID != pID && min <= neiPID && max>= neiPID )    ++(neis.insert({neiPID,0}).first->second);

    auto neitr = std::max_element(neis.begin(), neis.end(), mapComparer<T>());
    if (neitr->second >= nNeist) {
      //++nChanges;
      vImg(i,j,k) = neitr->first;
     }
    }
  }

  //cout<<"  nGrowMedian: "<< left<<nChanges<<"  ";

}

template<typename T>
void medGrow(voxelImageT<T>& vImg, T min,  T max, int dist=1,  short nNeist=2) { // grow values  in range [min max]
  (std::cout<<"  medGrow ").flush();
  const voxelImageT<T> vxls = vImg;
  //long long nChanges = 0;
  forAllkji_m_(dist,vxls) {
    const T* vp = &vxls(i,j,k);
    T pID = *vp;

    if (min > pID || max<pID) {

     std::vector<T> neis; neis.reserve(6);///.  ID-counter

     T
     neiPID = vxls.v_i(-dist,vp);
     if (min <= neiPID && max>= neiPID )   neis.push_back(neiPID);
     neiPID = vxls.v_i( dist,vp);
     if (min <= neiPID && max>= neiPID )   neis.push_back(neiPID);
     neiPID = vxls.v_j(-dist,vp);
     if (min <= neiPID && max>= neiPID )   neis.push_back(neiPID);
     neiPID = vxls.v_j( dist,vp);
     if (min <= neiPID && max>= neiPID )   neis.push_back(neiPID);
     neiPID = vxls.v_k(-dist,vp);
     if (min <= neiPID && max>= neiPID )   neis.push_back(neiPID);
     neiPID = vxls.v_k( dist,vp);
     if (min <= neiPID && max>= neiPID )   neis.push_back(neiPID);

     if (int(neis.size())>nNeist)
     {
      std::nth_element(neis.begin(), neis.begin() + neis.size()/2, neis.end());
      //++nChanges;
      vImg(i,j,k) = neis[neis.size()/2];
     }
    }
  }

  //cout<<"  nGrowMedian: "<< left<<nChanges<<"  ";

}

template<typename T>
voxelImageT<T> mean(const voxelImageT<T>& vImg) {
  (std::cout<<"  mean ").flush();
  voxelImageT<T>  smooth=vImg;
  forAllkji_1_(vImg)
  {
    int neiSum=0;
    forAllNei(-1,1)
    {
      neiSum+=_nei(vImg,i,j,k);
    }
    smooth(i,j,k)=(0.5+neiSum/27.);
  }
  return smooth;
}



template<typename T>
voxelImageT<T> maxWide(const voxelImageT<T>& vImg, int nW, T lowerB, T upperB) {
  (std::cout<<"  maxWide ").flush();
  voxelImageT<T> vxls=vImg;
  forAllkji_m_(nW,vImg) {
    if(const T* vp=&vImg(i,j,k); *vp<lowerB)
    vxls(i,j,k) =
          min(max(max(vImg.v_i(-nW,vp), vImg.v_i( nW,vp)),
                  max(max(vImg.v_j(-nW,vp), vImg.v_j( nW,vp)),
                      max(vImg.v_k(-nW,vp), vImg.v_k( nW,vp)))), upperB);
  }

  return vxls;
}

template<typename T>
voxelImageT<T> meanWide(const voxelImageT<T>& vImg, int nW, T lowerB, T upperB, voxelImageT<T>& grad, T gradthresh) {
  (std::cout<<"  meanWide ").flush();
  voxelImageT<T> vxls=vImg;
  forAllkji_m_(nW, vImg) {
    const T* vp=&vImg(i,j,k);
    if( (*vp<lowerB || upperB<*vp) && grad(i,j,k)<=gradthresh)
    {
      int nNeis=0; Tint sumNeis=0;
      getnNeissumNeis(nNeis,sumNeis, vImg,vp,nW, lowerB, upperB)
      getnNeissumNeis(nNeis,sumNeis, vImg,vp,nW/2, lowerB, upperB)
      if(nNeis>5)
        vxls(i,j,k) = (sumNeis+nNeis/2)/nNeis;
    }
  }
  return vxls;
}

/*//template<typename T> voxelImageT<T> meanWide(const voxelImageT<T>& vImg, int nW, T lowerB, T upperB)
//{
  //(std::cout<<"  meanWide ").flush();
  //voxelImageT<T> vxls=vImg;
  //forAllkji_m_(nW,vImg)
  //{  const T* vp=&vImg(i,j,k);
    //if( *vp<lowerB || upperB<*vp)
    //{
      //std::array<T,7> vvs={{
                //vImg.v_i(-nW,vp), vImg.v_i( nW,vp),
                //vImg.v_j(-nW,vp), vImg.v_j( nW,vp),
                //vImg.v_k(-nW,vp), vImg.v_k( nW,vp)
                 //}};
      //std::nth_element(vvs.begin(),vvs.begin()+3,vvs.end());
      //vxls(i,j,k) = vvs[3];
    //}
  //}
  //return vxls;
//}

//template<typename T>
//voxelImageT<T> mean6Wide(const voxelImageT<T>& vImg, T lowerB, T upperB)
//{
  //(std::cout<<"  mean6Wide ").flush();
  //voxelImageT<T> vxls=vImg;
  //forAllkji_1_(vImg)
  //{  const T* vp=&vImg(i,j,k);
    //if( *vp<lowerB || upperB<*vp)
      //vxls(i,j,k) = (sum6Nei(vImg,vp)+2)/6;
  //}
  //return vxls;
//}
*/

template <typename T>
dbls convertToDbls(piece<T> hist) {
  int nHist = len(hist);
  dbls histd(nHist);
  for (int i = 0; i<nHist; i++) histd[i]=hist[i];
  return histd;
}


template<typename OtT>
inline pair<OtT, array<double,5>> otsu_threshold(const piece<double>& hist, int  ibgn, int  iend, OtT shift, OtT scale) {
  /* binarization by Otsu's method based on maximization of inter-class variance */
  double sumw(0), sumiw(0);
  for (int i = ibgn; i <= iend; ++i) { sumw+=hist[i];  sumiw+=hist[i]*i; }

  double sumB = 0;
  double wB = 0;
  double max_sigma = 0;
  int ilevel=1;
  double wL=1;
  double sunL=1;
  for (int ii = ibgn; ii <= iend; ii++) {
    wB += hist[ii];
    if (wB == 0 || wB == sumw)        continue;
    sumB +=  ii * hist[ii];
    //double dif = sumB*sumw  -  sumiw*wB   ; ///. <= sumB * (sumw-wB)  -  (sumiw-sumB) *wB;
    //double  sigma =  dif * dif / (wB * (sumw-wB) ); ///. division is to componsate for above line
    double dif = sumB*sumw  -  sumiw*wB   ; ///. <= sumB /wB  -  (sumiw-sumB) /(sumw-wB);
    double  sigma =  dif * dif / (wB * (sumw-wB) ); ///. division is to componsate for above line
    if ( sigma > max_sigma) {
      ilevel = ii;
      max_sigma = sigma;
      sunL=sumB;
      wL=wB;
    }
  }

  OtT level = shift + ilevel*scale;
  OtT minv = shift + ibgn*scale;
  OtT maxv = shift + iend*scale;

  double  avgs[2] = {sunL/wL, (sumB-sunL)/(wB-wL)};
  cout<<"   Otsu: ** "<< level<<" **  N: "<< sumw<<",  cdf_"<<level<<": "<< std::setprecision(3)<<wL<<"  cdf_"<<maxv<<": "<< std::setprecision(3)<<wB;
  cout<<"   segment 2   "<<minv<<"  "<< int(avgs[0]) <<"  "<<level<<"  "<<int(avgs[1])<<"  "<<maxv<<endl;


  array<double,5> levels{{0.}};
  levels[0]=minv;   levels[1]=avgs[0];   levels[2]=level;   levels[3]=avgs[1];   levels[4]=maxv;
  return {level, levels};

  /*
  const int MaxVV = 256 ; // No. of gray levels
  double prob[MaxVV]; // prob of graylevels
  double myu[MaxVV], cdf[MaxVV];   // mean value for separation, omega is CDF
  double max_sigma, sigma[MaxVV]; // inter-class variance
  long long total(0);


  // Histogram generation
  for(int hi : hist)  { total+=hi; }

  // calculation of probability density
  for (int i = minv; i < maxv; i ++ )  prob[i] = (double)hist[i] / (total);

  // cdf & myu generation
  cdf[minv] = prob[minv];
  myu[minv] = minv*prob[minv];       // 0. times prob[0] equals zero
  for (int i = minv+1; i < maxv; ++i) {   cdf[i] = cdf[i-1] + prob[i];   myu[i] = myu[i-1] + i*prob[i];  }

  // sigma maximization sigma stands for inter-class variance and determines optimal levelold value
  int level = 0; // levelold for binarization
  max_sigma = 0.;
  for (int i = minv; i < maxv-1; ++i) {
    if (cdf[i]!=0. && cdf[i]!=1.)  {   sigma[i] = pow(myu[maxv-1]*cdf[i]-myu[i], 2) /   (cdf[i]*(1.-cdf[i])); }
    else      sigma[i] = 0.;
    if (sigma[i] > max_sigma) {      max_sigma = sigma[i];      level = i;    }
  }
  //for(int i=minv; i<maxv; ++i)cout<<sigma[i]<<" ";cout<<endl;

  double avgs[2] = {myu[level]/cdf[level], (myu[maxv-1]-myu[level])/(cdf[maxv-1]-cdf[level])};
  cout<<"   Otsu levelold: ** "<< level<<" **  N: "<< total<<",  cdf_"<<level<<": "<< std::setprecision(3)<<cdf[level]<<"  cdf_"<<maxv-1<<": "<< std::setprecision(3)<<cdf[maxv-1];
  cout<<"   segment 2   "<<minv<<"  "<< int(avgs[0]) <<"  "<<level<<"  "<<int(avgs[1])<<"  "<<maxv<<endl;


  array<double,5> levels{{0.}};
  levels[0]=minv;   levels[1]=avgs[0];   levels[2]=level;   levels[3]=avgs[1];   levels[4]=maxv;
  return levels;
  //forAllkji(vImg)    vImg(i,j,k) = vImg(i,j,k)>level;
*/
}


template<typename T>
array<double,5> otsu_th(const voxelImageT<T>& vImg, Tint  minvi, Tint  maxvi, double skipFrac=0.)
{//! Warning doesn't work for negatives
  int3 nnn1=skipFrac*vImg.size3();
  int3 nnn2=(1.000000000001-skipFrac)*vImg.size3();
  T  minv=minvi, maxv=maxvi;

  const int nHist = std::min(maxvi-minvi, Tint(65536/10));
  Tint delta = std::max((maxvi-minvi)/ nHist, Tint(T(1)));

  vars<long long> hist(nHist+1, 0l);

  for (int k=nnn1[2]; k<nnn2[2]; ++k)
    for (int j=nnn1[1]; j<nnn2[1]; ++j)
      for (int i=nnn1[0]; i<nnn2[0]; ++i) {
        T vv = vImg(i,j,k);
        if (minv<=vv && vv<maxv)  ++hist[(vv-minv)/delta];
      }
  dbls histd = convertToDbls(hist);
  return otsu_threshold(histd, 0, nHist, minvi, delta).second;
}

template<typename T>
voxelImageT<uint8_t> otsu_th01(const voxelImageT<T>& vImg, int  minvi=0, int  maxvi=maxT(T), double skipFrac=0.)
{//! Warning doesn't work for negatives
  T otst2 = otsu_th(vImg,minvi,maxvi,skipFrac)[2];
  voxelImageT<uint8_t> msk(vImg.size3(), 1, vImg.dx(),vImg.X0());
  forAlliii_(vImg)  if(vImg(iii)<otst2) msk(iii)=0;
  return msk;
}


template<typename T>
int average2p(const voxelImageT<T>& vm, T minV=2) {
  long long sum(128);
  long long nsum(1);
  forAllvv_seq(vm)  if(vv>=minV)  {    sum+=vv;    ++nsum;  }
  return sum/nsum;
}

template<typename T>
void deringImg(voxelImageT<T>& vImg, int nr,int nth,int nz,  T minV,T maxV,
  int X0,int Y0,int X1,int Y1, int nGrowBox, double scaleDifV=1, double sharpenRings=0.05, bool writeDumps=false) {// deringImg TODO to be tested

  cout<<"\n  dering:  { range:"<<minV <<"-"<<maxV<<"  XY0: "<<X0 <<" "<<Y0 <<"   XY1: "<<X1 <<" "<<Y1
      <<"  nr:"<<nr<<"  ntheta:"<<nth<<"  nz:"<<nz <<"  nGrowBox:"<<nGrowBox <<endl;

  const voxelImageT<T> vxls0 = vImg;
  if (nGrowBox) vImg.growBox(nGrowBox);
  X0+= nGrowBox; Y0+= nGrowBox; X1+= nGrowBox; Y1+= nGrowBox;
  nz += nGrowBox*(nz/vImg.nz());
  nr+= nGrowBox;

  int n3=vImg.nz(),n2=vImg.ny(),n1=vImg.nx();
  const int  nrCrs = 1.01 + (n2+n1)*0.25/nr;
  const int  nzCrs = n3/nz;
  nz = n3/nzCrs+1;
  nr = n1/nrCrs+1;


  const voxelImageT<T> vxls = vImg;

  if (maxV==maxT(T)) maxV -= 1;

  const T invalidT = std::min(T(maxV+1),maxT(T));


  cout<<"capping to "<<maxV<<endl;
  int noisev=0.1*(maxV-minV);
  bilateralX(vImg, 3, 2, noisev*noisev,0.05,12);
  // bilateralX(vImg, 4, 4, noisev*noisev,0.05,12);
  // bilateralX(vImg, 6, 4, noisev*noisev,0.05,12);
  if (writeDumps) vImg.write("dumpFiltDering.tif");
  vImg=median(vImg);

  replaceRange(vImg, maxV, invalidT, maxV);
  replaceRange(vImg,maxV,maxT(T),invalidT);
  replaceRange(vImg,T(0),minV,invalidT);
  grow(vImg,invalidT, T(0),maxV); //. shrink used voxels
  grow(vImg,invalidT, T(0),maxV); ///. to set to origils

  constexpr T minVr = 1;

  if (writeDumps) vImg.write("dumpDringMeGrow0.tif");
  if (0) {
  // vImg=median(vImg);//, 0, 254, 1, 255
  // vImg=median(vImg);//, 0, 254, 1, 255
  // vImg=median(vImg);//, 0, 254, 1, 255
  // vImg=median(vImg);//, 0, 254, 1, 255
  // vImg=median(vImg);//, 0, 254, 1, 255
  // medGrow(vImg,minV,maxV,1,3);
  // medGrow(vImg,minV,maxV,1,3);
  // medGrow(vImg,minV,maxV,2,3);
  // medGrow(vImg,minV,maxV,2,3);
  // medGrow(vImg,minV,maxV,3,3);
  // medGrow(vImg,minV,maxV,3,3);
  // for(int ii=0; ii<10; ++ii) medGrow(vImg,minV,maxV,3,1);
  // vImg=median(vImg);//, 0, 254, 1, 255
  // for(int ii=0; ii<10; ++ii) medGrow(vImg,minV,maxV,5,0);
  // medGrow(vImg,minV,maxV,10,0);
  // medGrow(vImg,minV,maxV,20,0);
  // for(int ii=0; ii<10; ++ii) medGrow(vImg,minV,maxV,40,0);
  // medGrow(vImg,minV,maxV,20,0);
  }
  vImg=median(vImg);//, 0, 254, 1, 255
  T avg = average2p(vImg, minVr);
  cout<<"avg:"<<avg<<endl;
  // replaceRange(vImg,invalidT,invalidT,avg);
  //avg = 2*average2p(vImg)-avg;
  //replaceRange(vImg,invalidT,invalidT,avg);
  cout<<"avg:"<<avg<<endl;
  if (writeDumps) vImg.write("dumpDringMeGrow1.tif");


  voxelImageT<T> radimag({nr, nth+6, nz}, 0);
  for (int z=0; z<int(radimag.nz()) ; z++)
  {
    int k = z*nzCrs;
    for (int p=-2; p<int(radimag.ny())-2; p++)
    {
      float cosp(cos(_2pi*p/nth)), sinp(sin(_2pi*p/nth));
      for (int r=0; r<int(radimag.nx()) ; r++)
      {
      float xo=float((n3-k)*X0+k*X1)/n3+0.5;
      float yo=float((n3-k)*Y0+k*Y1)/n3+0.5;
      float rf = r*nrCrs;

      int i = xo+rf*cosp;
      int j = yo+rf*sinp;

      int  npCrs=sqrt(_2pi*sqrt((i-xo)*(i-xo)+(j-yo)*(j-yo))/nth)+1;
      float pf=p*npCrs;

      double neiSum=0;
      int nNeis=0;

      for (short j_nei_m=-npCrs-5/(r+1); j_nei_m<=npCrs+5/(r+1); ++j_nei_m)
       for (short i_nei_m=-nrCrs-5/(r+1); i_nei_m<=nrCrs+5/(r+1); ++i_nei_m) {
        int jj=yo+(rf+i_nei_m)*sin(_2pi*(pf+j_nei_m)/nth/npCrs);
        int ii=xo+(rf+i_nei_m)*cos(_2pi*(pf+j_nei_m)/nth/npCrs);
        if (ii>=0 && ii<n1 && jj>=0 && jj<n2)
        {
        for (short kk=max(nzCrs*int(z)-nzCrs-10./(r+1)+0.5,0.1); kk<=min(z*nzCrs+nzCrs+10./(r+1)+0.5,n3-0.9); ++kk)
          if (T vv = vImg(ii,jj,kk); vv!=invalidT) {
            neiSum+=vxls(ii,jj,kk); ++nNeis;
          }
        }
       }
      radimag(r,p+2,z)=min(T(0.5+neiSum/(nNeis+0.001)),maxT(T));
      }
    }

  }




  //vImg=radimag; return;
  medGrow(radimag,minVr,maxT(T),1,1);
  medGrow(radimag,minVr,maxT(T),1,1);
  medGrow(radimag,minVr,maxT(T),1,1);
  medGrow(radimag,minVr,maxT(T),2,1);
  medGrow(radimag,minVr,maxT(T),2,1);
  medGrow(radimag,minVr,maxT(T),3,1);
  medGrow(radimag,minVr,maxT(T),2,1);
  for(int ii=0; ii<10; ++ii) medGrow(radimag,minVr,maxT(T),3,1);
  radimag = medianz(radimag);
  radimag = mediany(radimag); // theta
  radimag = medianz(radimag);
  radimag = mediany(radimag); // theta
  if (writeDumps) radimag.write("dumpDringRad.tif");
  avg = average2p(radimag, minVr);
  replaceRange(radimag, T(0), T(minVr-1), T(avg));
  voxelImageT<T> smoothRad = mean(radimag);//GaussHalf

  //biLateral(smoothRad, 2., 1., 0.5);
  avg = average2p(smoothRad, minVr);
  replaceRange(smoothRad, T(0), T(minVr-1), T(avg));

  if (writeDumps) smoothRad.write("dumpDringSmoothRad.tif");

  cout << ": avg "<<avg<<endl;


  ensure((vImg.nz()-1)/nzCrs<smoothRad.nz(), _s(smoothRad.size3())+" "+_s(vImg.nz())+" "+_s(nzCrs));

  long long nSkipped = 0;
  bool warned = false;
  forAllkji_(vImg) {

    float xo=float((n3-k)*X0+k*X1)/n3+0.5;
    float yo=float((n3-k)*Y0+k*Y1)/n3+0.5;
    int r = 0.5+sqrt((i-xo)*(i-xo)+(j-yo)*(j-yo))/nrCrs;

    int p = (atan2(yo-j,xo-i)/_2pi+0.5)*nth;

    int radV = avg;
    if (!(r<smoothRad.nx() && p+2<smoothRad.ny() && k/nzCrs<smoothRad.nz())) {
      ++nSkipped;
      if (!warned) {
        ensure( k/nzCrs<smoothRad.nz(), "k: "+_s(k)+" k/nzCrs: "+_s(k/nzCrs)+" smoothRad.nz(): "+_s(smoothRad.nz()));
        warned = true;
      }
    }
    else if (T vr = smoothRad(r,p+2,k/nzCrs); vr >= minVr)
      radV = vr;

    vImg(i,j,k) = radV;
  }

  ensure(nSkipped==0, "nSkipped: "+_s(nSkipped)+" %"+_s(100.*nSkipped/(vImg.nz()*vImg.ny()*vImg.nx())));
  if (nGrowBox) vImg.shrinkBox(nGrowBox);
  ensure(vImg.size3()==vxls0.size3());

  if (writeDumps) vImg.write("dumpDringBackground.tif");
  vImg = median(vImg);
  bilateralX(vImg, 3, 2, noisev*noisev,sharpenRings,12);
  forAlliii_(vImg) vImg(iii) = min(max(int(vxls0(iii)) - int((vImg(iii)-avg)*scaleDifV), 0), imaxT(T));


  cout<<" dering; }/ "<<endl;
}



////////////////////


template<typename T>
void  multiSegment(voxelImageT<T>& vImg, vector<int> trshlds, vector<int> minSizs, double resolutionSqr, double noisVSqr, bool writeDumps, std::string smoot) {


  const int nSegs=minSizs.size();
  const int capmin =trshlds[0];
  const int capmax =trshlds[nSegs];

  (std::cout<<" "<<capmin<<" "<<capmax<<" ").flush();
  if(writeDumps)   vImg.write("dumpImage2.tif");

  bilateralGauss(vImg, 4, noisVSqr, 0.15, (resolutionSqr+1.), capmin,capmax);
  if(writeDumps)   vImg.write("dumpImage3_bilateral.tif");
  vector<int> meds(nSegs,-2);
  {
    voxelImageT<T> vxls = median(median(vImg));
    array<double,256> histi{{0}};
    forAllvv_seq(vxls)  { if (1<=vv && vv<maxT(T)) { histi[vv]+=1.;} }
    double myu[256], cdf[256];   // mean value for separation, omega is CDF
    int minv=trshlds[0], maxv=trshlds[nSegs];
    cdf[minv] = histi[minv];
    myu[minv] = minv*histi[minv];       // 0. times prob[0] equals zero
    for (int i = max(minv,1); i <= maxv; ++i) {   cdf[i] = cdf[i-1] + histi[i];   myu[i] = myu[i-1] + i*histi[i];  }
    for (int i=0; i<nSegs; ++i) meds[i] = (myu[trshlds[i+1]]-myu[trshlds[i]])/max(cdf[trshlds[i+1]]-cdf[trshlds[i]],1.);
  }
  cout<<"-> mid+ranges: "; for (int i=0; i<nSegs; ++i) { (std::cout<<"  "<<int(trshlds[i])<<" "<<int(meds[i])).flush(); }   cout<<"  "<<int(trshlds[nSegs])<<endl;


  for (size_t tt=0;tt<meds.size(); ++tt)
    replaceRange(vImg, T(meds[tt]),T(meds[tt]), T(meds[tt]-1)); ///! -1 is to make the voxels anasyned
  cout<<endl;

  voxelImageT<T> origimg = vImg;

  if(writeDumps)   origimg.write("dumpImage4_replaceRange.tif");

  voxelImageT<T> grad;

  if (hasExt(smoot,".mhd")) {
    vImg.readFromHeader(smoot);
    int ndif = origimg.nx()-vImg.nx();
    if(ndif) {
      if(ndif<20 && ndif%2==0)
        vImg.growBox(ndif/2);
      else cout<<" Error wrong image size "<<smoot<<endl;}

  }
  else {
    for (int i=0; i<5; ++i)
    {
      (cout).flush();
      bilateralGauss(vImg, 3, noisVSqr, 0.1, (resolutionSqr+1.), capmin,capmax);
    }
    //if(writeDumps)
      vImg.write("dumpImage5_bilateralSmooth.mhd");
  }

  grad=magGradient(vImg, resolutionSqr*(dmaxT(T)/(meds[meds.size()-1]-meds[0])));
  bilateralGauss(grad, 2, noisVSqr, 0.00, 1);
  forAllvp_(grad)  { if (*vp<1) { *vp=1;} }
  if(writeDumps)     grad.write("dumpGrad5.tif");

  //cout<<"capping to 254"<<endl;
  //replaceRange(vImg, maxT(T), maxT(T), 254);
  //*trshlds.rbegin()=min(*trshlds.rbegin(),254);
  //*meds.rbegin()=min(*meds.rbegin(),254);






  // voxelImageT<T> segmented=vImg;
  for (size_t tt=0;tt<meds.size(); ++tt) {
    cout<<tt<<": m"<<meds[tt]<<"  t"<<trshlds[tt]<<"     m"<<meds[tt]<<"  t"<<trshlds[tt+1]<<"    ";
    cout<< "    "<<0.5*(1.*meds[tt]+1.*trshlds[tt])+0.5<<"  "<<0.5*(meds[tt]+1.*trshlds[tt+1])-0.5<<"  "<<meds[tt]<<endl;
    replaceRange(vImg, T(0.5*(1.*meds[tt]+1.*trshlds[tt])+0.5), T(0.5*(meds[tt]+1.*trshlds[tt+1])-0.5), T(meds[tt]));
  }
  if(writeDumps) vImg.write("dumpseg0_replacethresholds.tif");


  //for (size_t tt=0;tt<meds.size(); ++tt)
   //for (int i=0; i<minSizs[tt]; ++i)
    //shrink(vImg, meds[tt],meds[tt], origimg);; ///. set to origs


  if(writeDumps)   vImg.write("dumpseg1.tif");




  //T avgMagGrad=average2p(grad);
  int noisv=sqrt(noisVSqr+1)+1;
  for (int iter=0; iter<8; ++iter) {
    if(writeDumps) vImg.write("dumpseg"+to_string(2+iter)+".tif");

    (std::cout<<" , ").flush();

    mode26(vImg,3);
    if(iter<3)  {
      for (size_t tt=0;tt<meds.size(); ++tt)
       for (int i=1; i<minSizs[tt]; ++i)
        mode26(vImg,T(meds[tt]),T(meds[tt]),5);
    }

    forAllkji_m_(2,origimg)  {
      //const int vv = vImg(i,j,k);
      const int ov = origimg(i,j,k);
      const auto gv = grad(i,j,k);

      if(gv && capmin<=ov && ov<=capmax)  {
        array<double,256> coVar;///. not efficient
        array<double,256> coVarSum;///. not efficient
        coVar[0]=0.;  coVarSum[0]=1.;
        for (int mid: meds) {coVar[mid]=0.; coVarSum[mid]=0.;}

      //if(nearMid)
      //{
        //for (const T* neip : {&vImg.v_i(-1,vp), &vImg.v_i(1,vp), &vImg.v_j(-1,vp), &vImg.v_j(1,vp), &vImg.v_k(-1,vp), &vImg.v_k(1,vp)})
        forAllNei(-2,2) {
          for (int mid: meds)
            if (mid==_nei(vImg,i,j,k)) {
              const int nei=_nei(origimg,i,j,k);
              const int neig=max(_nei(grad,i,j,k),_nei1(grad,i,j,k));
              const double weit=10000000./(std::max(neig,2)*(distSqrNei()+2)+noisv);
              coVar[mid]+=weit/(abs(nei-mid)*0+abs(nei-ov)+2*noisv);
              coVarSum[mid]+=weit;
            }
        }
        int newmid = 0;
        for (int mid: meds)
          if ( coVar[   mid]/((coVarSum[   mid]+2000000.)*(abs(ov-   mid)+8*noisv))>
              coVar[newmid]/((coVarSum[newmid]+2000000.)*(abs(ov-newmid)+8*noisv)))
            newmid=mid;

        if(newmid && coVar[newmid]>1e-12) vImg(i,j,k) = T(newmid);
      //}
      }
    }


    ////! 2018-07-04
    //forAllkji_m_(2,origimg)
    //{
      //const int ov = origimg(i,j,k);
      //const auto gv = grad(i,j,k);
      //if(gv && capmin<=ov && ov<=capmax)
      //{
        //array<double,256> coVar;///. not efficient
        //array<double,256> coVarSum;///. not efficient
        //for (int mid: meds) {coVar[mid]=0.; coVarSum[mid]=0.;}
        //forAllNei(-2,2)
        //{
          //for (int mid: meds)
            //if (mid==_nei(vImg,i,j,k))
            //{
              //const int neid=_nei(origimg,i,j,k)-mid;
              //const int neig=_nei(grad,i,j,k);
              //int weit=10000000/(max(neig,2)*(distSqrNei()+1)+noisv);
              //weit/=abs((ov-mid)-neid)+noisv;
              //coVar[mid]+=weit/(abs(neid)+noisv);
              //coVarSum[mid]+=weit;
            //}
        //}
        //T newmid = 0;
        //for (int mid: meds)  if ((1./(abs(ov-mid)+8*noisv))*coVar[mid]/(coVarSum[mid]+100000.)>(1./(abs(ov-newmid)+8*noisv))*coVar[newmid]/(coVarSum[newmid]+100000.)) { newmid=mid; }
        //if(newmid && coVar[newmid]>1e-12) vImg(i,j,k)=newmid;
      //}
    //}



    //int noisestd=sqrt(noisVSqr)/2.+0.5;
    /*forAllkji_1_(vImg)
    {
      const int gv = grad(i,j,k);
      const T* vp = &vImg(i,j,k);
      const T vv = *vp;
      int nearMid= 513;
      //for (int mid: meds)  if (mid==vv) { nearMid=0; break; }
      int ov = origimg(i,j,k);
      //if(nearMid)
      {
        //for (const T* neip : {&vImg.v_i(-1,vp), &vImg.v_i(1,vp), &vImg.v_j(-1,vp), &vImg.v_j(1,vp), &vImg.v_k(-1,vp), &vImg.v_k(1,vp)})
        forAllNei(-1,1)
        {
          const T neiv=_nei(vImg,i,j,k);
          const int neig=_nei(grad,i,j,k);
          //const T neiv=*neip;
          //const int neig=*(&grad.data_[0]+(neip-&vImg.data_[0]));
          for (int mid: meds)  if (mid==neiv && ((mid-vv)*(mid-vv)+noisVSqr)*(neig*neig+noisVSqr)<((nearMid-vv)*(nearMid-vv)+noisVSqr)*(gv*gv+noisVSqr)) nearMid=neiv;
        }
        if(nearMid<maxT(T)) vImg(i,j,k)=nearMid;
      }

    }*/


    /*
    //grad.growPore();
    //T gradLimit=min(avgMagGrad*(0.5+iter)/8+(1.*iter*iter)*dmaxT(T)/meds.size()/(resolutionSqr+2.),dmaxT(T));
    //cout<<"\n  gradLimit:"<<int(gradLimit)<<"  ";
    //replaceRange(grad,1,gradLimit,0);


    //for (size_t tt=0;tt<meds.size(); ++tt)
      //grow(vImg,meds[tt],(trshlds[tt]), (trshlds[tt+1]-1) , grad);
    //for (size_t tt=0;tt<meds.size(); ++tt)
      //grow(vImg,meds[tt],0.5*(midse[tt]+trshlds[tt])+0.5, 0.5*(midse[tt+2]+trshlds[tt+1])-0.5 , grad);
    //for (size_t tt=0;tt<meds.size(); ++tt)
      //grow(vImg,meds[tt],(midse[tt]+1), (midse[tt+2]-1) , grad);

    //for (size_t tt=0;tt<meds.size(); ++tt)
      //grow(vImg,meds[tt],0, 255 , grad);
    //cout<<"\n";
    //if(writeDumps) vImg.write("dumpseg"+to_string(2+iter)+".mhd");
    //if(writeDumps) grad.write("dumpundecid"+to_string(2+iter)+".mhd");
    */
  }

  cout<<"."<<endl;;
}



template<typename T>
void  multiSegment2(voxelImageT<T>& vImg, vars<Tint> trshlds, vars<int> minSizs, double resolutionSqr, double noisv, double localF, int krnl, double flatnes, double gradFactor, int nItrs, bool writeDumps)
{


  const int nSegs=minSizs.size();
  const Tint capmin =trshlds[0];
  const Tint capmax =trshlds[nSegs];

  (std::cout<<"\n  "<< __FUNCTION__<< " resolutionSqr:"<<resolutionSqr<<" noisv:"<<noisv<<" localF:"<<localF<<" krnl:"<<krnl<<" flatnes:"<<flatnes<<" gradFactor:"<<gradFactor<<"  ").flush();
  (std::cout<<"  analysing range ["<<capmin<<" "<<capmax<<"], ").flush();
  if(writeDumps)
     vImg.write("dumpImage2.tif");

  vars<T> meds(nSegs,-2);
  {
    voxelImageT<T> vxls = median(median(vImg));
    array<double,maxT(T)> histi{{0}};
    forAllvv_seq(vxls)  { if (1<=vv && vv<maxT(T)) { histi[vv]+=1.;} }
    double myu[imaxT(T)+1], cdf[imaxT(T)+1];   // mean value for separation, omega is CDF
    Tint minv=trshlds[0], maxv=trshlds[nSegs];
    cdf[minv] = histi[minv];
    myu[minv] = minv*histi[minv];       // 0. times prob[0] equals zero
    for (int i = max(minv,1); i <= maxv; ++i) {   cdf[i] = cdf[i-1] + histi[i];   myu[i] = myu[i-1] + i*histi[i];  }
    for (int i=0; i<nSegs; ++i) meds[i] = T((myu[trshlds[i+1]]-myu[trshlds[i]])/max(cdf[trshlds[i+1]]-cdf[trshlds[i]],1.));
  }
  cout<<"  -> mid+ranges: "; for_i_(0,nSegs) { (std::cout<<"  t"<<int(trshlds[i])<<" m"<<int(meds[i])).flush(); }   cout<<"  "<<int(trshlds[nSegs])<<endl;
  ensure(meds.back()>0,"meds:"+_s(meds),-1);


  voxelImageT<T> origimg = vImg;


  ints midIs(size_t(maxT(T))+1, -1);
  for (int ii=0; ii<nSegs; ++ii) { midIs[meds[ii]]=ii;  }
  //cout<<piece<int>(&midIs[0],&midIs.back())<<endl;

  vImg = median(vImg);


  voxelImageT<T> grad;
  grad=magGradient(origimg, resolutionSqr*(dmaxT(T)/(meds.back()-meds[0])));

  bilateralX(grad, 1,1, noisv*noisv, 0.05, 12);
  { double avgGrad=average2p(grad)*0.2+0.01;
  forAllvp_(grad)  { if (*vp<avgGrad) { *vp=10;} else  *vp=min(dmaxT(T),*vp*10./avgGrad);} }
  if(writeDumps)
    grad.write("dumpGrad5.tif");
  double avgGradInv=1./average2p(grad);

  cout<<endl;
  for (size_t tt=0;tt<meds.size(); ++tt)
  {
    replaceRange(origimg,  T(meds[tt]),  T(meds[tt]), T(meds[tt]-1));// to avoid conflict with seg values in shrink
    cout<<"  "<<tt<<"{ m"<<int(meds[tt])<<" <== t["<<trshlds[tt]<<','<<trshlds[tt+1]<<']'<<"}  ";
    T bgnh=0.5*(1.5*meds[tt]+0.5*trshlds[tt])+0.5;
    T endh=0.5*(1.5*meds[tt]+0.5*trshlds[tt+1])-0.5;
    replaceRange(vImg, bgnh, endh, meds[tt]); //achive region growing / water shed segmentation
    if(tt==0)             replaceRange(vImg, T(trshlds[tt]), bgnh, meds[tt]);
    else                  replaceRange(vImg, T(trshlds[tt]), bgnh, T(meds[tt]-1));
    if(tt+1==meds.size()) replaceRange(vImg, endh, T(trshlds[tt+1]), meds[tt]);
    else                  replaceRange(vImg, endh, T(trshlds[tt+1]), T(meds[tt]+1));
    cout<<endl;
  }

    //replaceRange(vImg, 0.5*(1.*meds[tt]+1.*trshlds[tt])+0.5, 0.5*(meds[tt]+1.*trshlds[tt+1])-0.5, meds[tt]);

  mode26(vImg,1);
  mode26(vImg,1);

  for (size_t tt=0;tt<meds.size(); ++tt)
   for (int i=0; i<minSizs[tt]; ++i)
    shrink(vImg, meds[tt],meds[tt], origimg);


  if(writeDumps)   vImg.write("dumpseg1.tif");
  //voxelImageT<T> meanimg = origimg;

  //const int noisv=sqrt(noisVSqr+1)+1;


  double resolSqr_1=resolutionSqr-0.99,   krnlCub=2*(krnl+0.5)*(krnl+0.5)*(krnl+0.5);
  for (int iter=0; iter<nItrs; ++iter)
  {
    std::cout<<endl;
      //array<double,maxT(T)> avgLbl={0.} ,  sumLbl={0.};
      //forAllkji_m_(2,origimg)
      //{ size_t iii = origimg.index(i,j,k); avgLbl[vImg(iii)]+=origimg(iii)-vImg(iii);  sumLbl[vImg(iii)]+=1; }
      //cout<<" ***"; for (int mid: meds) {cout<<" "<<avgLbl[mid]/(sumLbl[mid]+0.01)<<" "; } cout<<"*** ";

    if(writeDumps) vImg.write("dumpseg"+to_string(2+iter)+".tif");


    //if(iter<6)
    for (int i=0; i<minSizs[0]; ++i)
      mode26(vImg, 1);

    //if(iter<3) for(size_t tt=0;tt<meds.size(); ++tt) for(int i=1; i<minSizs[tt]; ++i) mode26(vImg,meds[tt],meds[tt],5);
    (std::cout<<"  ").flush();

    vImg.zeroGrad(2);
    //if(writeDumps)  meanimg.write("dumpmean"+to_string(2+iter)+".tif");

    voxelImageT<T> vxls = vImg;

    (std::cout<<"-").flush();
    size_t nChange=0,  nAll=0;
    double meanCV=0,  meanCVS=0;//,  meanTst=0;

    OMPFor(reduction(+:nChange) reduction(+:nAll) reduction(+:meanCV) reduction(+:meanCVS))
    forAllkji_m_seq(krnl,origimg)  {
      size_t iii = origimg.index(i,j,k);
      const int ov = origimg(iii);
      if(capmin<=ov && ov<=capmax)
      {


      //const int mv = meanimg(iii);
      double respGradFact=grad(i,j,k); respGradFact=resolSqr_1+gradFactor*respGradFact*respGradFact;
      vector<double> meanSum  (nSegs,0);
      vector<double> sumW (nSegs,1e-15);

      if(const int tI=midIs[vxls(i,j,k)];  tI>=0)  {
        const double weit=0.999;// *respGradFact
        meanSum[tI]-=weit*origimg(i,j,k);
        sumW[tI]-=weit;
      }
      forAllNei(-krnl, krnl) {
        const int tn=_nei(vxls,i,j,k);
        const int tI=midIs[tn];
        if(tI>=0) {
          double gj=avgGradInv*_nei(grad,i,j,k);
          const double weit=respGradFact/( distSqrNei()+resolSqr_1+ gradFactor*gj*gj );
          meanSum[tI]+=weit*_nei(origimg,i,j,k);//*(abs(mn-tn)+1000);*(abs(mn-on)+1)
          sumW[tI]+=weit;
          //meanCoVarSum2[tI]+=weit/(abs(ov-mn)+noisv);
        }
      }

      T vid = 0;
      double vidProb=10e64;
      for (int ii=0; ii<nSegs; ++ii) { //meanSum is watershed cmponent and risky, noisv refelects far-range variations, noisF refelects short range variance
        if ( sumW[ii]>1e-9) {
          double iiProb=(abs(meanSum[ii]/(sumW[ii]) -ov)*localF+(1.-localF)*abs(meds[ii]-ov)+noisv)/min(1.+flatnes*sumW[ii],krnlCub);
          if(iiProb<vidProb)   { vid=ii; vidProb=iiProb;}
        }
        ++nAll;
        meanCV+=meanSum[ii];
        meanCVS+=sumW[ii];
        //meanTst+=abs((meanSum[ii]+1000000.0f)/(sumW[ii]) - ov);
      }
      if(sumW[vid]>0.01 && meds[vid]!=vImg(iii)) {  ++nChange;  vImg(iii)=meds[vid];  }
      }
    }
    cout<<"  nChang:"<<nChange<<" CV:"<<meanCV/nAll<<"/"<<meanCVS/nAll<<" = "<<meanCV/meanCVS<<" nAll:"<<nAll;


  }

  cout<<"."<<endl;;
}


template<typename T>  bool segment2(voxelImageT<T>& vImg, int nSegs, vars<Tint> th, vars<int> minSizs,
double noisev, double localF, double flatnes, double resolution, double gradFactor, int krnl, int nItrs, int writedumps
) {
  cout<<"{ \n  segmenting";
  ensure( flatnes<0.9, "too large flatnes, set to ~0.1",2);  ensure( gradFactor<10.1, "too large gradFactor, set to ~0.1",2);
  ensure( localF<0.1, "localF should be < 0.1");          ensure( nItrs>5, "nItrs should be > 5 ");
  ensure( krnl,"wrong kernel  in segment2",-1);
  resolution=max(resolution,1.);
  if (th.empty()) th = vars<Tint>(nSegs + 1, -1);
  if (minSizs.empty()) { minSizs = vars<int>(nSegs, 2); minSizs[0] = 1; }

  (cout<<", nSegs: "<<nSegs).flush();
  cout<<", ranges: "<<th <<",  minSizs (nshrink): "<<minSizs<<endl;
  cout<<"  Noise(x): "<<noisev<<"  "<<localF<<",  krnl: "<<krnl<<",  flatnes: "<<flatnes<<",  diffuseL: "<<resolution<<",  gradFactor: "<<gradFactor<<":"<<endl;
  if (writedumps) cout<<"\n  **** writingdumps **** \n"<<endl;

  constexpr Tint i254 = maxT(T)-1;
  constexpr Tint i1=minT(T)+1;
  if(th[0]<i1)        { th[0]=i1;       forAllvp_(vImg)  if(*vp<i1)  *vp=i1;  }
  ensure( th[nSegs]<=maxT(T), "incompatible threshold value and image type", -1);
  if(th[nSegs]>i254)  { th[nSegs]=i254; forAllvp_(vImg)  if (*vp>i254) *vp=i254;  }


  if(th[0]<0)
  {
    cout<<" trying to figure out threshold (outdated) "<<endl;

    constexpr int nHist = std::min(Tint(maxT(T)), Tint(2<<12)-1);
    constexpr Tint delta = maxT(T) / nHist;
    static_assert(delta>=1);
    dbls hist(nHist+1, 0.);
    {
      cout<<"  calculating histogram: ";
      voxelImageT<T> voxls = vImg;
      //bilateralGauss(voxls, 2, noisev, 0.00, (resolution+1.), 1,254);
      voxls = median(median(median(median(median(vImg)))));
      //voxls.write("dumpmedian.mhd");
      voxelImageT<T> grad=magGradient(voxls, resolution);
      //grad.write("dumpGrad5.mhd");
      array<double,5> otst = otsu_th(grad,0,maxT(T)-1);

      forAlliii_seq(vImg)
      {  int vv = vImg(iii);
        if (0<vv && vv<maxT(T))  hist[vv]+=1./(max(grad(iii)-otst[1],0.)+0.1*otst[2]);
      }
      forAllvv_seq(voxls)  { if (1<=vv && vv<maxT(T)) { hist[vv/delta]+=1.; } }
    }

    if(nSegs>2)
    {  th[0]=1; th[nSegs]=maxT(T)-1;
      for_i_(1,nSegs)  if(th[i]<=0)  th[i] = th[i-1]+(i254-th[i-1])/(nSegs-i);
      cout<<"  Ges ranges: " <<th<<endl;
      for_i_(0,10)
      {
        for_i_(1,nSegs)  th[i] = otsu_threshold(hist, th[i-1]/delta, th[i+1]/delta, 0, delta).first;
        cout<<"  New ranges: "  <<th<<endl;
      }
    }
    else
    {  th[0]=1; th[nSegs]=i254;
      th[1] = otsu_threshold(hist, th[0]/delta, th[2]/delta, 0, delta).first;
      cout<<"  New ranges: " <<th<<endl;
    }
  }

  vImg.growBox(8);
  multiSegment2(vImg, th, minSizs, sqr(resolution), noisev, localF, krnl, flatnes, gradFactor, nItrs, writedumps);
  vImg.shrinkBox(8);

  cout<<"}"<<endl;
  return true;
}

    /*{//     //voxelImageT<T> avgimg = meanimg;
     //forAllkji_m_(1,origimg)
     //{
      //size_t iii = origimg.index(i,j,k);
      //const int ov = origimg(iii);//avgimg for recursive growth
      //const int tv = vImg(iii);      //const int wv = probImg(iii),  mv = meanimg(iii);
      //if(capmin<=ov && ov<=capmax)
      //{
        //float muk=0;
        //float sumRo=0;        //int sumP=0;
        //forAllNei(-1,1)
        //{
          //const int tn=_nei(vImg,i,j,k);
          //const int on=_nei(origimg,i,j,k);//avgimg
          //const int dl=on-ov;
          //const int rhoI = imaxT(T)*(tn==tv)/(abs(dl)+noisv);
          //sumRo+=rhoI;          //sumP+=(256*256+5)*(2*(tn==tv)-1)/(dl*dl+100);;
          //muk+=rhoI*on;
        //}
        //meanimg(iii)=min(muk/sumRo,fmaxT(T));        //probImg(iii)=min(max(sumP/(27*30)+5,1),maxT(T));
      //}
     //}
    }*/
    //meanimg.zeroGrad(2);    //probImg.zeroGrad(2);
        //vImg.zeroGrad(2);



      /*{//vector<float> meanCoVarSum2(nSegs,0);
      //const int mv = meanimg(iii);
      //forAllNei(-2,2)
      //{ const int tn=_nei(vImg,i,j,k);
        //const int tI=midIs[tn];
        //if(tI>=0)
        //{ const float wn=50;//ei(probImg,i,j,k);
          //const int mn=_nei(meanimg,i,j,k); //const int on=_nei(origimg,i,j,k);
          //const float weit=wn/( (distSqrNei()+resolutionSqr) );
          //meanSum[tI]+=weit*mn;// *(abs(mn-tn)+1000);*(abs(mn-on)+1)
          //sumW[tI]+=weit;
          //meanCoVarSum2[tI]+=weit/((mv-mn)*(mv-mn)+noisVSqr);
        //}
      //}
      //T vid = 0;
      //for (int ii=0; ii<nSegs; ++ii)
      //{//! the two +1s can increasee and decrease, respectively, to increase spherisity
        //if ( (abs((meanSum[ii ])/(sumW[ii]) - ov)+noisVSqr)/(meanCoVarSum2[ ii]+0.000001*noisVSqr)  <  // /sumW[ii]
           //(abs((meanSum[vid])/(sumW[vid]) - ov)+noisVSqr)/(meanCoVarSum2[vid]+0.0000101*noisVSqr) // /sumW[ii]
          //) { vid=ii; }
        //++nAll;
        //meanCV+=meanSum[ii];
        //meanCVS+=sumW[ii];
        //meanTst+=abs((meanSum[ii]+1000000.0f)/(sumW[ii]) - ov);
      //}
      //if(meanSum[vid]>1e-16 && meds[vid]!=vImg(iii)) {  ++nChange;  vImg(iii)=meds[vid];  }
      }*/
