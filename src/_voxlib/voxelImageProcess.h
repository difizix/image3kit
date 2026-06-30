#pragma once
/*-------------------------------------------------------------------------*\
Developed by:
  - Ali Qaseminejad Raeini (2015-2021), Email: a.q.raeini@gmail.com
\*-------------------------------------------------------------------------*/

#include "typses.h"
#include "voxelEndian.h"
#include "voxelImage.h"


#include "voxelImageML.h"
#include "voxelImageCutOutside.h"
#include "voxelRegions.h"
#include "voxelPng_stbi.h"
#include <algorithm>
#include <limits>

#ifdef SVG
#include "svplot.hpp"
#endif

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <cassert>

namespace MCTProcessing {


template<typename T>  bool averageWith(voxelImageT<T>& vImg, const std::vector<std::string>& imgNames) {
  int nImgs=1;
  voxelField<double> sumImgs(vImg.size3(),0.);
  forAlliii_(sumImgs) sumImgs(iii)=vImg(iii);
  cout<<" Averaging  images, ";
  for(const auto& img2Nam : imgNames) {
    if(img2Nam.size()>4)
    {
      cout<<"  with image  "<<img2Nam<<":  "<<endl;
      if (hasExt(img2Nam,".tif")) {
        vImg.reset(sumImgs.size3(),0);
        vImg.readBin(img2Nam);
      }
      else vImg.readFromHeader(img2Nam);
      ensure(vImg.nx()==sumImgs.nx(),"bad size", -1);
      forAlliii_(sumImgs) sumImgs(iii)+=vImg(iii);
      ++nImgs;
    }
    else if(img2Nam.size()) cout<<"\n  not with  "<<img2Nam<<":  "<<endl;
  }

  vImg.reset(sumImgs.size3());
  forAlliii_(sumImgs) vImg(iii)=double(sumImgs(iii))/nImgs+0.5;

  return true;
}

template<typename T>  bool averageWith( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("img2Nam img2Nam2...");
  std::vector<std::string> imgNames;
  while(ins)  {
    string img2Nam;  ins>>img2Nam;
    if(img2Nam.size()>4) imgNames.push_back(img2Nam);
    else { if(img2Nam.size()) cout<<"\n  not with  "<<img2Nam<<":  "<<endl; break; }
  }
  return averageWith(vImg, imgNames);
}

template<typename T>  bool averageWith_mBE(voxelImageT<T>& vImg, const std::vector<std::string>& img2Nams) {
  if (img2Nams.size()<2) { cout<<"\n\n  Error: need more than 2 images "<<endl;  return false; }

  const int nImg2s=img2Nams.size();
  cout<<"  averaging with "<<nImg2s<<" other images"<<endl;

  vector<voxelImageT<T>> image2s(nImg2s);
  for(int i=0; i<nImg2s; ++i)
  {
    voxelImageT<T>& image2 = image2s[i];
    image2 = voxelImageT<T>(img2Nams[i], readOpt::procOnly);
    if(image2.nx()!=vImg.nx()) cout<<"\n\n  Error bad size\n"<<endl;

  }

  forAlliii_(vImg)
  {
    int minvv=vImg(iii);   int  maxvv=minvv,    sumvvs=minvv;
    for(int i=0; i<nImg2s; ++i) { int vv = image2s[i](iii); sumvvs+=vv; minvv=min(minvv,int(vv)); maxvv=max(maxvv,int(vv)); }
    vImg(iii)=(((sumvvs-minvv)-maxvv) + (nImg2s-1)/2 )/(nImg2s-1);
  }

  return true;
}

template<typename T>  bool averageWith_mBE( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("img2Nam img2Nam2..., except begin and end vals");
  vector<string> img2Nams;
  cout<<" averaging mBE, images : "<<endl;
  while(ins)
  {
    string img2Nam;
    ins>>img2Nam;
    if(img2Nam.size()>4)
    {
      cout<<"  with image  "<<img2Nam<<":  "<<endl;
      img2Nams.push_back(img2Nam);
    }
    else { if(img2Nam.size()) cout<<"\n  not with  "<<img2Nam<<":  "<<endl; break; }
  }
  return averageWith_mBE(vImg, img2Nams);
}

template<typename T> bool adjustBrightnessWith(voxelImageT<T>& vImg, std::string img2Nam) {
  cout<<"   adjasting contrast,  ";
  double meanvimg = otsu_th(vImg, Tint(1), TImax(T)-1, 0.2)[3];

  _dbgetOrReadImg(T,image2,img2Nam);

  double meantovxls = otsu_th(image2, Tint(1), TImax(T)-1, 0.2)[3];

  Tint delC = meantovxls - meanvimg;
  cout<<"  adjust contrast by:  "<<int(meantovxls)<<" - "<<int(meanvimg)<<" = "<<delC<<",  "; cout.flush();
  forAllvp_(vImg)
    if(0<(*vp) && (*vp)<maxT(T))
      *vp = max(Tint(0), min(Tint(*vp)+delC, TImax(T)-1));;

  return true;
}

template<typename T>  bool adjustBrightnessWith( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("img2Nam ");
  string img2Nam;  ins>>img2Nam;
  return adjustBrightnessWith(vImg, img2Nam);
}


template<typename T> bool adjustSliceBrightness(voxelImageT<T>& vImg, voxelImage& mskRegA, voxelImage& mskRegB, voxelImageT<T>& img2, int nSmoothItr, int nSmoothKrnl)
{
    dbls mu1sRegA(vImg.nz(), 0.); //mean at region RegA of img1
    dbls mu2sRegA(vImg.nz(), 0.);
    dbls mu1sRegB(vImg.nz(), 0.);
    dbls mu2sRegB(vImg.nz(), 0.);
    //dbls cov11s(vImg.nz(), 0.); // variance img1
    //dbls cov12s(vImg.nz(), 0.);// covariance img1-2
    //https://en.wikipedia.org/wiki/Simple_linear_regression
    mskRegA.printInfo();
    mskRegB.printInfo();
    long long avgv=0;
    forAllcp(mskRegA) avgv += (*cp!=0);
    forAllcp(mskRegB) avgv += (*cp!=0);
    ensure(avgv < 1.1*mskRegA.nxy()*mskRegA.nz(), "Too much mask overlap", -1);

    OMPFor()
    for (int k=0; k<mskRegA.nz(); ++k)
    {
      double sum1A=0., sum2A=0., sum1B=0., sum2B=0.;
      long long countRegA=0, countRegB=0;
      forAlliii_k(mskRegA) {
       if (mskRegA(iii)){  sum1A+=vImg(iii); sum2A+=img2(iii); ++countRegA;  }
       if (mskRegB(iii)){  sum1B+=vImg(iii); sum2B+=img2(iii); ++countRegB;  }
      }

      mu1sRegA[k]=sum1A/countRegA;   mu2sRegA[k]=sum2A/countRegA;
      mu1sRegB[k]=sum1B/countRegB;  mu2sRegB[k]=sum2B/countRegB;
    }
    for (int k=1; k<mskRegA.nz()-1; ++k) if(mu1sRegA[k]==0&&mu1sRegB[k]==0) // add single missing slices
    {
      forAlliii_k(mskRegA)  {  vImg(iii)=0.5*(vImg(iii-vImg.nxy())+vImg(iii+vImg.nxy()));  }
      mu1sRegA[k]=0.5*(mu1sRegA[k-1]+mu1sRegA[k+1]);  mu2sRegA[k]=0.5*(mu2sRegA[k-1]+mu2sRegA[k+1]);
      mu1sRegB[k]=0.5*(mu1sRegB[k-1]+mu1sRegB[k+1]);  mu2sRegB[k]=0.5*(mu2sRegB[k-1]+mu2sRegB[k+1]);
    }

    cout<<"\n  pre biMovingAvg{\n"<<
    Table<double,piece>().apnd(mu1sRegA,"mu1sRegA").apnd(mu2sRegA,"mu2sRegA").apnd(mu1sRegB,"mu1sRegB").apnd(mu2sRegB,"mu2sRegB")
    <<"\n}\n"<<endl;
    NaNsToMean(mu1sRegA); NaNsToMean(mu2sRegA);  NaNsToMean(mu1sRegB); NaNsToMean(mu2sRegB);
    for(int itr=0;itr<nSmoothItr;++itr)
    {
      mu1sRegA= biMovingAvg(mu1sRegA,nSmoothKrnl,nSmoothKrnl);
      mu2sRegA= biMovingAvg(mu2sRegA,nSmoothKrnl,nSmoothKrnl);
      mu1sRegB= biMovingAvg(mu1sRegB,nSmoothKrnl,nSmoothKrnl);
      mu2sRegB= biMovingAvg(mu2sRegB,nSmoothKrnl,nSmoothKrnl);

      mu1sRegA= biMovingAvg(mu1sRegA,nSmoothKrnl,nSmoothKrnl);
      mu2sRegA= biMovingAvg(mu2sRegA,nSmoothKrnl,nSmoothKrnl);
      mu1sRegB= biMovingAvg(mu1sRegB,nSmoothKrnl,nSmoothKrnl);
      mu2sRegB= biMovingAvg(mu2sRegB,nSmoothKrnl,nSmoothKrnl);


      mu1sRegA= median(mu1sRegA,nSmoothKrnl,nSmoothKrnl);
      mu2sRegA= median(mu2sRegA,nSmoothKrnl,nSmoothKrnl);
      mu1sRegB= median(mu1sRegB,nSmoothKrnl,nSmoothKrnl);
      mu2sRegB= median(mu2sRegB,nSmoothKrnl,nSmoothKrnl);
    }

    dbls delta1 = (mu1sRegB-mu1sRegA);
    for(auto& dl:delta1) if(abs(dl)<1e-16) dl=(2*signbit(dl)-1)*1e-16;
    dbls slops = (mu2sRegB-mu2sRegA)/delta1;
    dbls shifs = mu2sRegA-slops*mu1sRegA;

    for(int itr=0;itr<nSmoothItr;++itr)
    {
      slops= biMovingAvg(slops,nSmoothKrnl,nSmoothKrnl);
      shifs= biMovingAvg(shifs,nSmoothKrnl,nSmoothKrnl);

      slops= biMovingAvg(slops,nSmoothKrnl,nSmoothKrnl);
      shifs= biMovingAvg(shifs,nSmoothKrnl,nSmoothKrnl);

      slops= median(slops,nSmoothKrnl,nSmoothKrnl);
      shifs= median(shifs,nSmoothKrnl,nSmoothKrnl);
    }


    cout<<"\n{\n"<<
    Table<double,piece>().apnd(mu1sRegA,"mu1sRegA").apnd(mu2sRegA,"mu2sRegA").apnd(mu1sRegB,"mu1sRegB").apnd(mu2sRegB,"mu2sRegB")
    .apnd(slops,"slops").apnd(shifs,"shifs")
    <<"\n}\n"<<endl;


    OMPFor()
    for (int k=0; k<vImg.nz(); ++k)
    {
      forAlliii_k(vImg) vImg(iii)=std::max(dminT(T),std::min(vImg(iii)*slops[k]+shifs[k],dmaxT(T)));
    }
  return true;
}

template<typename T>  bool adjustSliceBrightness( stringstream& ins, voxelImageT<T>& )  {
  KeyHint("....");
  cout<<"  adjusting Slice RegBrightness  ";
  int nSmoothItr(3), nSmoothKrnl(20);
  string img2Nam, mskNamRegA, mskNamRegB;  ins>>img2Nam>>mskNamRegA>>mskNamRegB>>nSmoothItr>>nSmoothKrnl;
  cout<<"  with "+img2Nam+", using masks "+mskNamRegA+" and"+mskNamRegB+" nSmoothItr:"; cout<<nSmoothItr<<"  nSmoothKrnl:"<<nSmoothKrnl<<"   ";

  #ifdef _STOR_PUB
    _dbgetOrReadImg(unsigned char, mskRegA, mskNamRegA);
    _dbgetOrReadImg(unsigned char, mskRegB, mskNamRegB);
    _dbgetOrReadImg(T,             img2   , img2Nam   );
    adjustSliceBrightness(vImg, mskRegA, mskRegB, img2, nSmoothItr, nSmoothKrnl);
  #else
    ensure(0,"adjustSliceBrightness not supported",-1);
  #endif
  return true;
}


template<typename T>  bool growingThreshold( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("(Outdated)....");
  //thresholdImage=true;
  cout<<"\n  converting to binary: 0 (pore) and 1 (rock):"<<endl
      <<" start selecting pore with values between:";
  int  StartThreshold1=0,StartThreshold2=0, finalThreshold1=0,finalThreshold2=0;
  int nIter=4;

  ins>>StartThreshold1;
  ins>>StartThreshold2;
  ins>>finalThreshold1;
  ins>>finalThreshold2;
  ins>>nIter;
  cout<<"   "<<(int)StartThreshold1<<"  and "<<(int)StartThreshold2<<"  inclusive."<<endl;
  cout<<"   growing to voxels  with values between:";
  cout<<"   "<<(int)finalThreshold1<<"  and "<<(int)finalThreshold2<<", in "<<nIter<<" iterations."<<endl;

  growingThreshold(vImg, T(StartThreshold1), T(StartThreshold2), T(finalThreshold1), T(finalThreshold2), nIter);

  return true;
}


template<typename T>  bool bilateralX( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("nItrs   kernRad    Xstp   sigmavv   sharpFact   sigmadd");
  double sigmavv(16.*maxT(T)/256.), sigmadd(2.), sharpFact(0.1); int nItrs(1), Xstp(2), kernRad(1);
  ins >>nItrs >> kernRad >>  Xstp >> sigmavv >> sharpFact >> sigmadd;
  return _bilateralX(vImg, nItrs, kernRad, Xstp, sigmavv, sharpFact, sigmadd);
}


template<typename T>  bool bilateralGauss( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("nItrs   kernRad   sigmavv   sharpFact   sigmadd");
   int nItrs(1), kernRad(1); double sigmavv(16.), sigmadd(2.), sharpFact(0.1);
  ins >> nItrs >> kernRad >> sigmavv >> sharpFact >> sigmadd;
  return _bilateralGauss(vImg, nItrs, kernRad, sigmavv, sharpFact, sigmadd);
}

template<typename T>  bool smooth(voxelImageT<T>& vImg, int nItrs, int kernRad, double sigmavv, double sharpFact) {
  cout<<"\nsmoothing:   nIterations:"<<nItrs <<", kernRad:"<<kernRad<<"  sigmavv:"<<sigmavv <<", sharpFact:"<<sharpFact<<"\n";
  vImg.growBox(kernRad+2);
  for (int i=0; i<nItrs; ++i)
  {
    cout<<"  ";
    //biLateral(vImg, kernRad, sigmavv, sharpFact);
    biLateralIterative(vImg, kernRad, sigmavv, sharpFact);
  }
  vImg.shrinkBox(kernRad+2);

  cout<<endl;
  return true;
}

template<typename T>  bool smooth( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("nItrs  kernRad  sigmavv  sharpFact");
  double sigmavv(16.), sharpFact(0.1); int nItrs(1), kernRad(1);
  ins >> nItrs >> kernRad >> sigmavv >> sharpFact;
  return smooth(vImg, nItrs, kernRad, sigmavv, sharpFact);
}


template<typename T>  bool medianTime( stringstream& ins, voxelImageT<T>& )  {
  KeyHint("Not implemented");
  //int nMedImgs=5;  ins >> nMedImgs;
  //cout<<"\n  nMedImgs:"<<nMedImgs;
  //vector<string> imgNams(nMedImgs);

  //vector<voxelImageT<T> > imgs(nMedImgs);
  //for(int ii=0; ii<nMedImgs; ++ii)
  //{
    //ins>>imgNams[ii];
    //imgs[ii]=voxelImageT<T>(imgNams[ii]);
  //}
  //int imgI=0;
  //int imgJ=0;
  //while (imgNams[imgI].size())
  //{
    //cout<<"  ";
    //auto& imgMid = imgs[nMedImgs/2]
    //forAlliii_(vImg)
    //{
      //intSum
      //for(const auto& img:imgs)
      //{
        //std::array<T,7> vvs={{ *vp,
                    //vImage.v_i(-1,vp), vImage.v_i( 1,vp),
                    //vImage.v_j(-1,vp), vImage.v_j( 1,vp),
                    //vImage.v_k(-1,vp), vImage.v_k( 1,vp)
                    //}};

        //std::nth_element(vvs.begin(),vvs.begin()+3,vvs.end());
        //////////nChanged+=voxls(i,j,k) != vvs[3];
        //voxls(i,j,k) = vvs[3];

      //}
      //vImg
    //}
    //vImg.mode(2);
    //vImg.write(imgNams[imgI]);
  //}
  //vImg.shrinkBox(kernRad+2);

  cout<<endl;
  return true;
}



template<typename T>  bool mode26(voxelImageT<T>& vImg, int nItrs, int nMinD) {
  cout<<" mode26 "<<nItrs<<" itrs,  minDif:"<<nMinD<<"  ";
  vImg.growBox(1);
  for (int i=0; i<nItrs; ++i)
    mode26(vImg,nMinD);
  vImg.shrinkBox(1);
  return true;
}

template<typename T>  bool mode26( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("nItrs nMinD");
  int nItrs=1, nMinD=2;
  ins >> nItrs>> nMinD;
  return mode26(vImg, nItrs, nMinD);
}


/*//template<typename T>  bool modeFilter( stringstream& ins, voxelImageT<T>& vImg)
//{
  //if(ins.peek()=='?') { ins.str("nItrs(1) minNdif(1)"); return true; }
  //int nItrs(1); int minNdif(2);
  //ins >> nItrs >> minNdif;
  //if (minNdif<0 || minNdif>6) cout<<"Error:  !(minNdif<1 || minNdif>3): in modeFilter, bad threshold: "<<minNdif<<""<<endl;;
  //minNdif=min(max(minNdif,0),6);
  //(cout<<"  mode Filter, nIterations: "<<nItrs<<",  minNdif: "<<minNdif<<":").flush();
  //for (int i=0; i<nItrs; ++i)
  //{
    //vImg.mode(minNdif);
    //(cout<<" - ").flush();
  //}
  //(cout<<".").flush();
  //return true;
//}*/


template<typename T>  bool flipEndian( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("fix ending, use to convert big-endian to small-ending");
  ::flipEndian(vImg);
  (cout<<"flipEndian.").flush();
  return true;
}

#ifdef SVG

template<typename T> int svgZProfile(const voxelImageT<T>& vImg, const string& fnam, T minv, T maxv)  {
  svg::svgraphic my_svg;
  svg::svplot& my_plot = my_svg.subplot<svg::svplot>(0);
  vars<dbls> datas(3,dbls(vImg.nz(),0.));
  OMPFor()
  for (int k=0; k<vImg.nz(); ++k)  {
    double sum=0.; size_t cnt=0;
    forAlliii_k(vImg) {  T vv=vImg(iii); if(minv<=vv && vv<=maxv)  { sum+=vv; ++cnt; } }
    datas[0][k]=k;
    datas[1][k]=sum/std::max(cnt, size_t(1));
    datas[2][k]=cnt;
  }
  my_plot.plot(datas[0], datas[1],"").line_on(true).shape(svg::no_point);
  my_plot.x_label("z (voxels)").y_label("average intensity");
  my_svg.description("RawData:\n"+_s(tableIO(datas, {"z(voxels)","average","count"})));
  cout<<"\nWriting "<<fnam<<" \n";  // #WViz
  my_svg.write(fnam);
  return 0;
}

template<typename T> int svgHistogram(const voxelImageT<T>& vImg, const string& fnam, int nBins, double minV, double maxV)  {
  ensure(maxV<-1e38 || maxV>minV,"Wrong  range", -1);
  svg::svgraphic my_svg;
  svg::svplot& my_plot = my_svg.subplot<svg::svplot>(0);
  vars<dbls> datas = vxlDist(vImg,nBins,minV,maxV);
  my_plot.plot(datas[0], datas[1],"").line_on(true).shape(svg::no_point);
  my_plot.x_label("voxel value").y_label("frequency");
  my_svg.description("RawData:\n"+_s(tableIO(datas, {"voxel value","frequency"})));
  cout<<"\n""Writing "<<fnam<<" \n";  // #WViz

  my_svg.write(fnam);
  return 0;
}

template<typename T>  bool svgHistogram( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("fileName nBins minV  maxV // write PDF of voxel intensities");
  int nBins(128); double minV(3e38), maxV(-3e38);
  string fnam("aa.svg");
  ins >> fnam >> nBins>> minV>> maxV ;
  if(!hasExt(fnam,".svg"))  fnam+=".svg";
  if (fnam.find('/')==std::string::npos) { mkdirs("fig"); fnam="fig/"+fnam; }

  cout<<" >> "<<fnam<<", nBins: "<<nBins<<", range: "<<minV<<" "<<maxV<<", ";
  svgHistogram(vImg,fnam,nBins,minV,maxV);
  return 0;
}

template<typename T>  bool svgZProfile( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("fileName.svg minV  maxV  // write slice averaged intensities");
  string fnam("aa.svg");  Tint minV(0), maxV(maxT(T));
  ins >> fnam >> minV>> maxV ;
  if(!hasExt(fnam,".svg"))  fnam+=".svg";
  if (fnam.find('/')==std::string::npos) { mkdirs("fig"); fnam="fig/"+fnam; }
  cout<<" >> "<<fnam<<", range:"<<minV<<" "<<maxV<<"  ";
  svgZProfile(vImg,fnam,T(minV),T(maxV));
  return 0;
}


template<typename T>  bool plotAll(voxelImageT<T>& vImg,  int minv=0, int maxv=-1000001, int iSlice_=-1000000, int nBins=128,
  string normalAxis="xyz", string fnam_="pltAll", int colrGreyHistZprofile=15,
  voxelImageT<T>* img2Ptr=nullptr, int mina=0, int maxa=-1000001 // transparency
)  {
  if(hasExt(fnam_,".png")) fnam_=fnam_.substr(0,fnam_.size()-4);
  if(maxv==-1000001)  { minv = accumulate(vImg,std::min<T>, std::numeric_limits<T>::max());   maxv = accumulate(vImg,std::max<T>, std::numeric_limits<T>::min());  }
  (cout<<"slice2: "<<fnam_<<"*, minmaxv: "<<minv<<" "<<maxv<<" ... "<<nBins).flush();
  ensure (maxv>minv, "error maxv shall be bigger than minv, hint: the image may be empty, or has a single value");

    if (fnam_.find('/')==std::string::npos) { mkdirs("fig"); fnam_="fig/"+fnam_; }

  if(colrGreyHistZprofile&4 && vImg.nz()>1) // FIXME: nz==1 breakes svgplot
    svgHistogram(vImg,fnam_+"Dist.svg",nBins,(minv),(maxv));
  if(colrGreyHistZprofile&8) svgZProfile( vImg,fnam_+"ZProfile.svg"  ,T(minv),T(maxv));
  (cout<<".... ").flush();

  #ifdef LPNG
  for(char Ax:normalAxis)  {
    int iSlice = iSlice_;
    int iDir=Ax-'x'; iDir%=3;
    if(iSlice<-vImg.size3()[iDir]) iSlice=vImg.size3()[iDir]/2;
    string fnam=fnam_;
    if(fnam[0]=='_')  fnam=_s(Ax)+_s(iSlice)+fnam;
    else     fnam=fnam+"_"+_s(Ax)+_s(iSlice);


    (cout<<" "<<Ax<<""<<iSlice<<" ").flush();
    if(img2Ptr) {
      slice2RGBAPng(vImg,Ax,fnam+"_rgba.png", iSlice,T(minv),T(maxv),*img2Ptr,T(mina),T(maxa));
    } //else
    if(colrGreyHistZprofile&2) slice2RGBPng    (vImg,Ax,fnam+"_rgb.png", iSlice,T(minv),T(maxv));
    if(colrGreyHistZprofile&1) slice2GrayPng   (vImg,Ax,fnam+"_grey.png", iSlice,T(minv),T(maxv)); // grey_ is at begin for image slide-show order
    (cout<<" .").flush();
  }

  #endif

  return true;
}

template<typename T>  bool plotAll( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("fileName  minv maxv nBins normalAxes iSlice_ alphaImg // histogram and intensity-vs-z profile");
  int minv(0); int maxv(-1000001); int iSlice_(-1000000), nBins(128);
  string normalAxis="xyz";
  int colrGrey=15;
  string fnam_("pltAll"),  alphaImg;
  int mina(0); int maxa(-1000001);
  ins >> fnam_ >> minv >> maxv >> nBins >> colrGrey >> normalAxis >> iSlice_ >> alphaImg >> mina >> maxa;
  voxelImageT<T>* img2Ptr=nullptr;
  if(len(alphaImg)) {
    #ifdef _STOR_PUB
      img2Ptr=vxlCast<T>(dbget(_STOR,alphaImg));
      if(maxa==-1000001)  { mina = accumulate(*img2Ptr,(std::min<T>));   maxa = accumulate(*img2Ptr,(std::max<T>));  }
      (cout<<" alpha: "<<alphaImg<<"  minmaxa: "<<mina<<" "<<maxa).flush();
    #else
      (cout<<" alpha image not supported "<<maxa).flush();
    #endif
  }
  return plotAll(vImg,  minv, maxv, iSlice_, nBins, normalAxis, fnam_, colrGrey, img2Ptr, mina, maxa);
}
#endif

template<typename T>  bool end_here( stringstream& ins, voxelImageT<T>& )  {
  KeyHint("//End of VxlPro commands");
  cout<<"end_here"<<endl;
  //DONOT throw xception("end_here");  // end_here is used as synonym with end which is also used as the last data in InputFile commands
  return 0;
}

template<typename T>  bool print_info( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("// print image informations");


  vImg.printInfo();

  (cout<<".").flush();
  return true;
}

template<typename T>  bool print_otsu( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("minv maxv // not reliable");
  int minv(0),maxv(256);
  //char dir='z';
  ins >> minv >> maxv ;

  ( cout<<"  otsu_th:"   ).flush();

  otsu_th(vImg,minv,maxv);

  (cout<<".").flush();
  return true;
}


template<typename T>  bool dering( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("X0  Y0  X1 Y1 minV maxV nr ntheta nz // Fix ring effects");

  int X0,Y0,X1,Y1, nr=0.2*(vImg.ny()+vImg.nx()), ntheta=18, nz=vImg.nz();
  ins >> X0>> Y0;
  X1=X0;Y1=Y0;
  ins >> X1>> Y1;
  int minV(0),maxV(maxT(T));
  ins >> minV>> maxV;
  ins >> nr>> ntheta>> nz;

  deringImg(vImg,nr,ntheta,nz,T(minV),T(maxV), X0,Y0,X1,Y1, 10, 1, 0.05, true);
  return true;
}

template<typename T>  bool segment(voxelImageT<T>& vImg, int nSegs, std::vector<int> trshlds, std::vector<int> minSizs, std::string smoot, double noisev, double resolutionSqr, int writedumps) {
  cout<<"\n  segmenting, ";
  cerr<<"\n  Outdated, use segment2 instead";

  resolutionSqr=max(resolutionSqr,1.);
  cout<<",  noiseAmp: "<<noisev<<",  diffuseL: "<<resolutionSqr;
  if (writedumps) cout<<"\n  **** writingdumps **** \n"<<endl;
  if (hasExt(smoot,".mhd"))  cout<<"\n  **** reading smooth file:"<<smoot<<" **** \n"<<endl;
  cout<<":"<<endl;


  ensure(trshlds.size() > size_t(nSegs), "threshold array less than nSegs, <" + _s(nSegs), -1);
  ensure(nSegs>1, "nSegs  shall be at least 2", -1);
  constexpr int nHist = std::min(Tint(maxT(T)), Tint(2<<12)-1);
  constexpr Tint delta = maxT(T)/nHist;
  static_assert(delta>=1, "wrong algorithm");

  if(trshlds[0]<0) {
    dbls hist;
    {
      cout<<"  calculating histogram: ";
      voxelImageT<T> voxls = vImg;
      //bilateralGauss(voxls, 2, noisev, 0.00, (resolutionSqr+1.), 1,254);
      array<long long, nHist+1> histi{{0}};
      voxls = median(median(median(median(median(vImg)))));
      //voxls.write("dumpmedian.mhd");
      // voxelImageT<T> grad = magGradient(voxls, resolutionSqr);
      //grad.write("dumpGrad5.mhd");
      // array<double,5> otst = otsu_th(grad, 0, maxT(T)-1);

      // forAlliii_seq(vImg) {
      //   int vv = vImg(iii);
      //   if (1<=vv && vv<maxT(T))  hist[vv/delta] += 1./(max(double(grad(iii))-otst[1],0.)+0.1*otst[2]);
      // }
      forAllvv_seq(voxls)  { if (1<=vv && vv<maxT(T)) { ++histi[vv/delta]; } }
      hist = convertToDbls(allof(histi));
    }



    if(nSegs>2)  {
      trshlds[0]=1; trshlds[nSegs]=254;

      for (int iSg=1; iSg<nSegs; ++iSg)  if(trshlds[iSg]<=0)  trshlds[iSg]=trshlds[iSg-1]+(254-trshlds[iSg-1])/(nSegs-iSg);
        cout<<"  Ges ranges: ";    for (int i=0; i<=nSegs; ++i) {(cout<<" "<<int(trshlds[i])<<" ").flush(); }  cout<<" "<<endl;


      for (int itr=0; itr<10; ++itr)
      {

        for (int iSg=1; iSg<nSegs; ++iSg)
          trshlds[iSg] = otsu_threshold(hist, trshlds[iSg-1], trshlds[iSg+1], 0, delta).first;


        cout<<"  New ranges: ";    for (int i=0; i<=nSegs; ++i) {(cout<<" "<<int(trshlds[i])<<" ").flush(); }  cout<<" "<<endl;
      }
    }
    else  {
      trshlds[0]=1; trshlds[nSegs]=254;
      trshlds[1] = otsu_threshold(hist, trshlds[0], trshlds[2], 0, delta).first;

      cout<<"  New ranges: ";    for (int i=0; i<=nSegs; ++i) {(cout<<" "<<int(trshlds[i])<<" ").flush(); }  cout<<" "<<endl;

    }
  }




  vImg.growBox(8);
  multiSegment(vImg, trshlds, minSizs, resolutionSqr, noisev, writedumps, smoot);
  vImg.shrinkBox(8);

  cout<<endl;
  return true;
}

template<typename T>  bool segment( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint(" nSegs(2) t_i(1 128 254) minSizes_i(1 1)  smoothimg(\"\") \\\n\
           noisev(16) resolutionSqr(2) writedumps(0)\n\t example:\n\t segment 2 1 60 255\n\t  (OUTDATED)");

  double noisev(16.), resolutionSqr(2);
  int nSegs(2), writedumps(0);
  ins >> nSegs ;
  string smoot;

  //segment nSeg  t0 med t1 med  t2   minsiz0(2)  minsiz1(2)  noisev(16.)  resolutionSqr(2)  readdumps

  vector<int> trshlds(nSegs+1,-1);


   for (int i=0; i<=nSegs; ++i)   ins>>trshlds[i];
  cout<<"  Ranges: ";    for (int i=0; i<=nSegs; ++i) {(cout<<" "<<int(trshlds[i])<<" ").flush(); }  cout<<" "<<endl;


  vector<int> minSizs(nSegs, 1); *minSizs.rbegin()=1;   *minSizs.begin()=1;
  cout<<"  minSizs: ";
  for (int i=0; i<nSegs; ++i)  {  ins>> minSizs[i];  (cout<<" "<<int(minSizs[i])).flush(); }

   ins >> smoot;
  ins >> noisev >> resolutionSqr >> writedumps;
  return segment(vImg, nSegs, trshlds, minSizs, smoot, noisev, resolutionSqr, writedumps);
}


template<typename T>  bool segment2( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("nSegs(3) t_i(1 60 128 254) minSizs(1 2 2)  "
            " noisev(2) localF(0.05) flatnes(0.01)  resolution(2) nItrs(13) writedumps(0)");

  int nSegs(2);  ins >> nSegs ;

  vars<Tint> th;  ins >> th;//- thresholds.  NB! using vars<T> does not read ascii as number
  vars<int> minSizs;  ins >> minSizs;   //NOT USED

  double noisev(2.),localF(0.05), flatnes(0.1), resolution(2), gradFactor(0); int krnl(2), nItrs(13), writedumps(0);
  ins >> noisev >>localF >> krnl >> flatnes >> resolution >> gradFactor>> nItrs >> writedumps;

  return segment2(vImg, nSegs, th, minSizs, noisev, localF, flatnes, resolution, gradFactor, krnl, nItrs, writedumps);
}

template<typename T>  bool vxlProCutOutside( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("dir(z) nExtraOut(0) threshold outVal(-1) nShiftX(0) nShiftY(0) cutHighs(0)\n// automatically remove voxels on the outside of cylindrical samples");

  //circular cut
  char dir='z';
  int nExtraOut(0), threshold(-1), outVal(maxT(T)), nShiftX(0), nShiftY(0), cuthighs(0);
  ins >> dir>>nExtraOut>>threshold>>outVal>>nShiftX>>nShiftY>>cuthighs;
  (std::cout<<"  cutOutside:  dir:"<<dir<<",  nExtraOut:"<<nExtraOut<<",  threshold:"<<threshold<<",  cuthighs:"<<cuthighs<<",  nShiftX:"<<nShiftX<<",  nShiftY:"<<nShiftY ).flush();

  VoxLib::cutOutside(vImg,dir,nExtraOut,threshold,cuthighs,nShiftX,nShiftY, T(outVal));

  (std::cout<<".").flush();
  return true;
}


template<typename T>  bool replaceByImageRange( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("imagefilename.mhd thresholdMin thresholdMax");
  string fnam;   ins >> fnam;
  unsigned int  thresholdMin(1),thresholdMax(0);
  ins >> thresholdMin >> thresholdMax;
  cout<<" Replacing  with "<<fnam<<" range [ "<<thresholdMin<<"  "<<thresholdMax<<"];   ";
  ensure(thresholdMin<=thresholdMax,"wrong thresholds",-1);
  if(fnam.size())
    replaceByImageRange(vImg, T(thresholdMin), T(thresholdMax), voxelImageT<T>(fnam, readOpt::procOnly));
  else  alert("no image name provided",-1);
  (cout<<".").flush();
  return true;
}


template<typename T>  bool replaceRangeByImage( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("thresholdMin  thresholdMax filenam");

  unsigned int  thresholdMin(0),thresholdMax(0);
  ins >> thresholdMin >> thresholdMax;

  string fnam;   ins >> fnam;
  cout<<" Replacing range  ["<<thresholdMin<<"  "<<thresholdMax<<"] with "<<fnam<<";   ";
  if(fnam.size()) replaceRangeByImage(vImg, T(thresholdMin), T(thresholdMax), voxelImageT<T>(fnam, readOpt::procOnly));
  else              alert("no image name provided");
  (cout<<".").flush();
  return true;
}


template<typename T> bool replaceOutSideValue(voxelImageT<T>& vImg, int vo, int vnew, int nHoleSize) {
  //forAllvp_(vImg) if(*vp == vo) *vp=vnew;  //replaceRange(vo,vo,vnew);
  int3 nn = vImg.size3();
  voxelImageT<T> vxls = vImg;
  //OMPragma("omp parallel for")
  for (int k=0; k<nn.z; ++k)
    for (int j=0; j<nn.y; ++j) {
      int i=0;   while (i<nn.x && vxls(i,j,k)==vo) { vImg(i,j,k)=vnew; ++i; }
      i=nn.x-1;   while (i>=0   && vxls(i,j,k)==vo) { vImg(i,j,k)=vnew; --i; }
    }
  //OMPragma("omp parallel for")
  for (int k=0; k<nn.z; ++k)
    for (int i=0; i<nn.x; ++i) {
      int j=0;   while (j<nn.y && vxls(i,j,k)==vo) { vImg(i,j,k)=vnew; ++j; }
      j=nn.y-1;  while (j>=0   && vxls(i,j,k)==vo) { vImg(i,j,k)=vnew; --j; }
    }

  //OMPragma("omp parallel for")
  for (int j=0; j<nn.y; ++j)
    for (int i=0; i<nn.x; ++i) {
      int k=0;   while (k<nn.z && vxls(i,j,k)==vo) { vImg(i,j,k)=vnew; ++k; }
      k=nn.z-1;   while (k>=0   && vxls(i,j,k)==vo) { vImg(i,j,k)=vnew; --k; }
    }
  for(int i=0; i<nHoleSize; ++i)  grow(vImg,T(vo),T(vnew),T(vnew));
  for(int i=0; i<nHoleSize; ++i)  grow(vImg,T(vnew),T(vo),T(vo));
  return true;
}

template<typename T>  bool replaceOutSideValue( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("vv_old vv_new nHoleSize ");
  int vo = 0, vnew = 2, nHoleSize=5;
  ins >> vo>> vnew>> nHoleSize;
  return replaceOutSideValue(vImg, vo, vnew, nHoleSize);
}

template<typename T>  bool meanWide(voxelImageT<T>& vImg, int nW, int noisev, int avg, int delta, int nItrs, std::string smoothImg) {
  std::cout<<"  meanWide:  nW:"<<nW<<"  noisev:"<<noisev<<"  avg:"<<avg <<"  delta:"<<delta <<"  nIterations:"<<nItrs<<"  smoothImg:"<<smoothImg<<std::endl;

  if(avg==0)
  {
    array<double,5> thresholdsOtsu = otsu_th(vImg,0,254,0.2);
    avg=thresholdsOtsu[3];
    std::cout<<"  avg:"<<avg<<"  nW:"<<nW <<std::endl;
  }

  if(!nW) return true;
  vImg.growBox(nW+5);
  int3 n = vImg.size3();
  for (int i:{1,2,3})
  {
    vImg.replaceyLayer(n[1]-1-i,      4);
    vImg.replaceyLayer(i,        n[1]-5);
    vImg.replacexLayer(n[0]-1-i,      4);
    vImg.replacexLayer(i,        n[0]-5);
    vImg.setLayer     (n[2]-1-i, &vImg(0,0,4));
    vImg.setLayer     (i,        &vImg(0,0,(n[2]-5)));
  }
  Tint minvv = max(avg-delta,1);
  Tint maxvv = min(avg+delta,imaxT(T)-2);
  T minvnrw = max(avg-delta/2,1);
  T maxvnrw = min(avg+delta/2,imaxT(T)-2);
  voxelImageT<T> orig = vImg;
  if(smoothImg.empty()) bilateralX(vImg, 6,3, noisev*noisev, 0.05, 12);
  voxelImageT<T> grad = smoothImg.size() ? magGradient(voxelImageT<T>(smoothImg, readOpt::procOnly), 16) : magGradient(median(mean(median(vImg))), 16);
  grad=median(grad);
  grad.write("dumpGrad5.raw");
  //threshGrad = otsu_th(grad,0,254)[1]*0.6+0.5;
  double threshGrad = 2.5*noisev;
  (std::cout<<"  threshGrad:"<<threshGrad<<",  ").flush();
  vImg.write("dumpImg4.raw");
  keepLargest(vImg,grad,T(2),T(maxT(T)-2),T(0),T(threshGrad));
  vImg.write("dumpImg5.raw");
  {
    replaceRange(vImg,T(0),minvnrw,T(minvv-1));
    replaceRange(vImg,maxvnrw,maxT(T),T(maxvv+1));
    vImg = median(vImg);
    //shrink(vImg,minvv-1,minvv-1,orig);
    //shrink(vImg,maxvv+1,maxvv+1,orig);
    vImg = median(vImg);

    T tresh=min(1.50*threshGrad,dmaxT(T));
    forAlliii_(grad)  {  if(grad(iii)>tresh) vImg(iii) = maxvv+1;  }
  }
  grow(vImg,T(maxvv+1),T(avg),maxT(T));
  grow(vImg,T(minvv-1),T(0),T(avg));
  //grow(vImg,maxvv+1,avg,255);
  //grow(vImg,minvv-1,0,avg);


  for (int i=0; i<nItrs; ++i)
  {
    threshGrad +=1+0.5*noisev;
    threshGrad *=1+1.5*i/(1.+nItrs);
    minvnrw = max(minvnrw-delta/10-2,minvv);
    maxvnrw = min(maxvnrw+delta/10+2,maxvv);
    cout<<" threshGrad:"<<min(0.2*threshGrad,dmaxT(T))  <<" minvnrw:"<<int(minvnrw)  <<" maxvnrw:"<<int(maxvnrw)  <<" "<<  endl;
    vImg=meanWide(vImg,i%nW+1,minvnrw,maxvnrw,grad,T(min(0.2*threshGrad,dmaxT(T))));
    //vImg=median(vImg);
    //vImg=maxWide(vImg,20-5*((i+0)%4),T(max(mean-20,0)),T(max(mean-0,0)));
    //vImg=maxWide(vImg,20-5*((i+1)%4),T(max(mean-20,0)),T(max(mean-0,0)));
    //vImg=maxWide(vImg,20-5*((i+2)%4),T(max(mean-20,0)),T(max(mean-0,0)));
    //vImg=maxWide(vImg,20-5*((i+3)%4),T(max(mean-20,0)),T(max(mean-0,0)));
    //vImg=meanWide(vImg,5-3+(1*i+1)%7,T(max(mean-10+nItrs/2,0)),T(min(mean+10-nItrs/2,255)));
    //vImg=mean6Wide(vImg,T(max(mean-10,0)),T(min(mean+10,255)));
    //vImg=meanWide(vImg,2,T(max(mean-10,0)),T(min(mean+10,255)));
    //vImg=mean6Wide(vImg,T(max(mean+1,0)),T(min(mean-1,255)));
  }

  vImg.write("dumpImg6.raw");
  vImg.shrinkBox(nW+5);

  (std::cout<<".").flush();
  return true;
}

template<typename T>  bool meanWide( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("nW  noisev  avg  delta  nItrs  smoothImg");

  int nW=0, noisev=4, avg=0, delta=20; //noisev is a scale factor
  int nItrs = 15;
  string smoothImg;
  ins >>nW >>noisev>>avg>>delta>>nItrs>>smoothImg;
  return meanWide(vImg, nW, noisev, avg, delta, nItrs, smoothImg);
}

template<typename T>  bool labelImage( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("minvv  maxvv  outnam");

  int minvv(0),maxvv(10);
  string outnam;
  ins >>minvv>>maxvv>>outnam;
  (std::cout<<"  labelImage: min/max vv:["<<minvv <<","<<maxvv <<")  output:"<<outnam  ).flush();

  auto lbls = labelImage(vImg,T(minvv),T(maxvv));
  compressLabelImage(lbls);
  lbls.write(outnam);
  (std::cout<<".").flush();
  return true;
}


template<typename T> bool readFromFloat(voxelImageT<T>& vImg, std::string header, float aa, float bb) {
  cout<<" Reading data from header "<<header<<" converting to T (short),  d = "<<aa<<"*x+"<<bb<<endl;
  voxelImageT<float> vImgf(header, readOpt::procOnly); // readFromHeaderT
  vImg=voxelImageT<T>(vImgf.size3(), 0, vImgf.dx(), vImgf.X0());
  forAlliii_(vImg)  vImg(iii) = max(minT(T),T(min(fmaxT(T), aa*vImgf(iii)+bb)));

  (std::cout<<".").flush();
  return true;
}

template<typename T>  bool readFromFloat( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("imageName scale shift");
  vImg.reset(0,0,0,0);
  string header;  float aa=1, bb=0;
  ins>>header >>aa>>bb; //!readFromFloat
  return readFromFloat(vImg, header, aa, bb);
}


} // namespace MCTProcessing
