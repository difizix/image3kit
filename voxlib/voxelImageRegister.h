/*-------------------------------------------------------------------------*\
Image registration
\*-------------------------------------------------------------------------*/


#include "Vctr.h"
#include "Tnsr.h"
#include "typses.h"
#include "cmaes.h"
// #include "newuoa.h"
#include "voxelImage.h"
#include "voxelImageI.h"
#include <mutex>

using namespace std;


template<typename T>  int3 toInt3(const Vctr<3,T>& d){   int3 n;   for(size_t i=0; i<3; ++i) n[i]=d[i];   return n;   }


#define CrsFctr  3
#define SCALE_TRANS 1000.
template<typename T>
class sumAbsDif {
 public:
  sumAbsDif(voxelImageT<T>& vImageGrad, const voxelImageT<T>& toImageGrad, const voxelImageT<T>& wImage, Vctr3d nMin, Vctr3d nMax)
  :  vImage_(vImageGrad), toImage_(toImageGrad), wImage_(wImage), nMin_(toInt3(nMin)), nMax_(toInt3(nMax)), calls_count(0)
  {
    //maxTo=accumulate(toImage_, std::max<T>, minT(T));
    // adjust brightness outside, before masking
    //double sumI=0., sum2=0.;
    //long long countI=0., count2=0;
    //OMPFor(reduction(+:sumI) reduction(+:countI))
    //forAllcp(vImage_)
      //if((*cp)>0) { sumI+=(*cp); ++countI;}

    //OMPFor(reduction(+:sum2) reduction(+:count2))
    //forAllcp(toImage_)
      //if((*cp)>0) { sum2+=(*cp); ++count2;}

    //difContrast_ = sum2/(count2+1)-sumI/(countI+1);
    //(cout<<"  difContrast: "<<difContrast_<<"  ").flush();

    //forAllvp_(vImageGrad) *vp = min(max(0,int(*vp)+difContrast_),imaxT(T));  // vImageGrad being changed

  };

  double operator()(long , const double *x) const
  {
    double res(1000);
    Vctr3d   trans = x; trans*=SCALE_TRANS; // assuming the init guess is
    Tnsr3d   rotat; rotat = x+3;
    Vctr3d offset(vImage_.X0()/vImage_.dx()-1e-12);

    int3 nnn=vImage_.size3();//-int3(1,1,1)
    double sumw = 0.1;
    forkjid_seq(nMin_,nMax_,1)  //set to 3 to spead up
    {
      const int v1=toImage_(i,j,k);
      if(v1) // mask
      {
        const int w= 1;//wImage_(i,j,k);//   1.;
        sumw+=w;
        dbl3 ijk(rotat*Vctr3d(i,j,k)+trans-offset);
        double dv = (0<=ijk.x&&   0<=ijk.y&&   0<=ijk.z && ijk.x<nnn.x   && ijk.y<nnn.y   && ijk.z<nnn.z) ?
                vImage_.vv_mp5(ijk.x,ijk.y,ijk.z)-v1 :
                v1+(maxT(T)>>4);
        res+=w*dv*dv;
      }
    }
    res/=sumw;
    if (calls_count%100==0)
     (cout<<_s(rotat)+" "+_s(trans)+" "+_s(sqrt(res))+" \n").flush();//
    mut.lock();
    ++calls_count;
    mut.unlock();

    return res+0.1*(abs(rotat(0,2))+abs(rotat(2,0))+abs(rotat(1,2))+abs(rotat(2,1))+abs(rotat(2,2)-1.));
  };
 public:
  const voxelImageT<T>& vImage_;
  const voxelImageT<T>& toImage_;
  const voxelImageT<T>& wImage_;
  const int3 nMin_;
  const int3 nMax_;
  //T maxTo;
  //int difContrast_;
  mutable std::mutex mut;
  mutable int calls_count;
};



template<typename T>
class sumAbsDif7DOF {
 public:
  sumAbsDif7DOF(voxelImageT<T>& vImageGrad, const voxelImageT<T>& toImageGrad, const voxelImageT<T>& wImage, Vctr3d nMin, Vctr3d nMax)
  :  vImage_(vImageGrad), toImage_(toImageGrad), wImage_(wImage), nMin_(toInt3(nMin)), nMax_(toInt3(nMax)), calls_count(0)
  {  };

  double operator()(long, const double *x) const
  {
    double res(1000);
    dbl3   trans(x); trans*=SCALE_TRANS; // assuming the init guess is
    double angl= x[3];
    dbl3   axis(x+4);  axis.z=1.; axis/=mag(axis);;
    double scal= x[6];
    Vctr3d offset(vImage_.X0()/vImage_.dx()-1e-12);

    int3 nnn=vImage_.size3();//-int3(1,1,1)
    double sumw = 0.1;
    forkjid_seq(nMin_,nMax_,1)  //set to 3 to spead up
    {
      const int v1=toImage_(i,j,k);
      if(v1) // mask
      {
        const int w= 1;//wImage_(i,j,k);//   1.;
        sumw+=w;
        dbl3 ijk(rotateAroundVec(dbl3(i,j,k),angl,axis)*scal+trans-offset);
        double dv = (0<=ijk.x&&   0<=ijk.y&&   0<=ijk.z && ijk.x<nnn.x   && ijk.y<nnn.y   && ijk.z<nnn.z) ?
                vImage_.vv_mp5(ijk.x,ijk.y,ijk.z)-v1 :
                v1+(maxT(T)>>4);
        res+=w*dv*dv;
      }
    }
    res/=sumw;
    if (calls_count%100==0)
     (cout<<_s(trans)+" "+_s(angl)+"  "+_s(axis)+" *"+_s(scal)+": "+_s(sqrt(res))+" \n").flush();//
    mut.lock();
    ++calls_count;
    mut.unlock();

    return res;
  };
 public:
  const voxelImageT<T>& vImage_;
  const voxelImageT<T>& toImage_;
  const voxelImageT<T>& wImage_;
  const int3 nMin_;
  const int3 nMax_;
  //T maxTo;
  //int difContrast_;
  mutable std::mutex mut;
  mutable int calls_count;
};



template<typename T>
inline std::array<double,7>  registerToImageEMS7DOF(voxelImageT<T>& origImage, voxelImageT<T>& vxls, voxelImageT<T>& origImage2, const voxelImageT<T>& toVxls, const voxelImageT<T>& wImage, dbl3 X1=dbl3(0.25,0.25,0.25), dbl3 X2=dbl3(0.75,0.75,0.75), dbl3 dX0=dbl3(0.,0.,0.), double thetaZ=0., int defaultV=-1)
{

  constexpr int nx = 7;
  double rotZ = thetaZ*(PI/180.);
  double xx[] = {dX0.x/SCALE_TRANS, dX0.y/SCALE_TRANS, dX0.z/SCALE_TRANS,   rotZ,   0., 0., 1.};

  dbl3 minReg(vxls.size3()); minReg*=X1;
  dbl3 maxReg(vxls.size3()); maxReg*=X2;
  sumAbsDif7DOF<T> function(vxls,toVxls,wImage,minReg,maxReg);

  CMAES<double>  evo;
  double *const* pop;
  double stddev[nx]= { 0.01, 0.01, 0.01,  0.01,   0.01, 0.01,  0.00001};
  Parameters<double> parameters;
  parameters.init(nx, xx, stddev);
  double *ovs = evo.init(parameters); // obj func return values
  std::cout << "{ "<<evo.sayHello() << std::endl;

  while(!evo.testForTermination()&& function.calls_count<2000*nx)
  {
    pop = evo.samplePopulation(); // Generate lambda new search points, sample population.  Do not change content of pop
    //Here you may resample each solution point pop[i] until it  becomes feasible, e.g. for box constraints (variable boundaries). function is_feasible(...) needs to be   user-defined.
    //Assumptions: the feasible domain is convex, the optimum is  not on (or very close to) the domain boundary, initialX is  feasible and initialStandardDeviations are sufficiently small  to prevent quasi-infinite looping.
    // for (i = 0; i<evo.get(CMAES<double>::PopSize); ++i)         while (!is_feasible(pop[i]))           evo.reSampleSingle(i);
    int nPop =evo.get(CMAES<double>::Lambda)+0.5;
    OMPFor()
    for (int i=0; i<nPop; ++i)
      ovs[i] = function(nx, pop[i]);

    evo.updateDistribution(ovs);
  }
  //std::cout << "Stop:" << std::endl << evo.getStopMessage();
  //evo.writeToFile(CMAES<double>::WKResume, "resumeevo1.dat"); // write resumable state of CMA-ES
  // std::cout<<nEvals<<" "<<evo.get(CMAES<double>::Lambda)<<std::endl;

  const double *  xxBest = evo.getPtr(CMAES<double>::XBestEver); // "XBestEver" or "XMean" might be used

  dbl3   trans(xxBest); trans*=SCALE_TRANS*CrsFctr; // assuming the init guess is
  double angl= xxBest[3];
  dbl3   axis(xxBest+4);  axis.z=1.; axis/=mag(axis);;
  double scal= xxBest[6];
  dbl3 offset(origImage.X0()/origImage.dx());
  cout<<"} rotat:  "<<angl<<" around "<<axis<<"\n"<<" trans: "<<trans<<", scale "<<scal<<"\n"<<" offset: "<<offset<<endl;
  cout<<"function_calls_count: "<< function.calls_count<<endl;
  // cout<<"result: "<< result<<endl;



  const int3 nnn=origImage.size3();
  double dif=0.; size_t count=1;
  int diffcontrast=0;//function.difContrast_;
  OMPragma("omp parallel for reduction(+:dif) reduction(+:count)")
  forAllkji(origImage2) {
    dbl3 ijk(rotateAroundVec(dbl3(i,j,k),angl,axis)*scal+trans-offset);
        //dbl3 ijk((imgCntr+rotateAroundVec(dbl3(i,j,k)-imgCntr,angl,axis))*scal+trans-offset); once implemented in func, no effect
    if(-0.499<ijk[0] && ijk[0]<nnn[0]-1.  &&   -0.499<ijk[1] && ijk[1]<nnn[1]-1.  &&   -0.499<ijk[2] && ijk[2]<nnn[2]-1.) {
      dif+=abs(double(origImage2(i,j,k))-double(origImage(ijk[0],ijk[1],ijk[2])));  ++count;
      origImage2(i,j,k) = min(max(0, int(origImage.vv_mp5(ijk[0],ijk[1],ijk[2]))),imaxT(T)) ;
    }
    else if (defaultV==-1)
      origImage2(i,j,k) = min(max(0, int(origImage2(i,j,k))-diffcontrast),imaxT(T));
  }
  origImage = origImage2;

  cout<<"dif: "<< dif/count-diffcontrast<<endl;

  return std::array<double,7>{xxBest[0],xxBest[1],xxBest[2],xxBest[3],xxBest[4],xxBest[5],xxBest[6]};
}






/*//inline voxelImage  registerToImageNewuoa(const voxelImage& vImage, const string& toImgName)
//{
//  voxelImage toVxlsOrig(toImgName);
//  voxelImage toVxls = (((mean(toVxlsOrig))));
//  voxelImage vxls = (((mean(vImage))));
//
//  const size_t nx = 12;
//  double xx[] = {0., 0., 0.,   1.,0., 0.,   0., 1., 0.,   0., 0., 1.};
//
//  Vctr3d minReg=toVctr3d(vxls.size3()); minReg*=0.3;
//  Vctr3d maxReg=toVctr3d(vxls.size3()); maxReg*=0.7;
//  sumAbsDif function(vxls,toVxls,minReg,maxReg);
//
//  {
//    //Vctr3d   trans = xx; trans*=SCALE_TRANS;
//    //Tnsr3d   rotat; rotat = xx+3;
//    //cout<<" rotat: \n"<<rotat<<endl;   cout<<" trans: "<<trans<<endl;
//  }
//
//  const long n_interpolation_conditions = (nx + 1)*(nx + 2)/2;
//  const double init_trust_region_radius = 0.02;
//  const double final_trust_region_radius = 0.002;
//  const long max_function_calls_count = 1000;
//  const size_t working_space_size = NEWUOA_WORKING_SPACE_SIZE(nx, n_interpolation_conditions);
//  double working_space[working_space_size];
//  //double result =
//  newuoa(function,
//       nx,  n_interpolation_conditions,  xx,
//       init_trust_region_radius,
//       final_trust_region_radius,
//       max_function_calls_count,
//       working_space);
//
//
//  Vctr3d   trans = xx; trans*=SCALE_TRANS;
//  Tnsr3d   rotat; rotat = xx+3;
//  Vctr3d offset(vImage.X0()/vImage.dx());
//  cout<<" rotat: \n"<<rotat<<endl;   cout<<" trans: "<<trans<<endl;
//  cout<<"function_calls_count: "<< function.calls_count<<endl;
//  // cout<<"result: "<< result<<endl;
//
//  int3 nnn=vImage.size3();
//  double dif=0.; int count=1;
//  OMPragma("omp parallel for reduction(+:dif) reduction(+:count)")
//  for (int k=0; k<(toVxlsOrig).nz(); ++k)
//  for (int j=0; j<(toVxlsOrig).ny(); ++j)
//  for (int i=0; i<(toVxlsOrig).nx(); ++i)
//  {
//    Vctr3d ijk = rotat*Vctr3d(i,j,k)+trans-offset;
//    if(-0.5<=ijk[0] && ijk[0]<nnn[0]-0.5  &&   -0.5<=ijk[1] && ijk[1]<nnn[1]-0.5  &&   -0.5<ijk[2] && ijk[2]<nnn[2]-0.5)
//    {
//      dif+=abs(double(toVxlsOrig(i,j,k))-double(vImage(ijk[0],ijk[1],ijk[2])));  ++count;
//      toVxlsOrig(i,j,k) = vImage.vv_mp5(ijk[0],ijk[1],ijk[2]);
//    }
//  }
//  cout<<"dif: "<< dif/count<<endl;
//  return toVxlsOrig;
//}
*/




template<typename T>
inline std::pair<Tnsr3d,Vctr3d>  registerToImageEMS(voxelImageT<T>& origImage, voxelImageT<T>& vxls, voxelImageT<T>& origImage2, const voxelImageT<T>& toVxls, const voxelImageT<T>& wImage, dbl3 X1=dbl3(0.25,0.25,0.25), dbl3 X2=dbl3(0.75,0.75,0.75), dbl3 dX0=dbl3(0.,0.,0.), double thetaZ=0.)
{

  const int nx = 12;
  double rotZ = thetaZ*(PI/180.);
  double x0[] = {dX0[0]/SCALE_TRANS, dX0[1]/SCALE_TRANS, dX0[2]/SCALE_TRANS,   cos(rotZ),-sin(rotZ), 0.,   sin(rotZ),cos(rotZ), 0.,   0., 0., 1.};
  double xx[] = {dX0[0]/SCALE_TRANS, dX0[1]/SCALE_TRANS, dX0[2]/SCALE_TRANS,   cos(rotZ),-sin(rotZ), 0.,   sin(rotZ),cos(rotZ), 0.,   0., 0., 1.};

  dbl3 minReg(vxls.size3()); minReg*=X1;
  dbl3 maxReg(vxls.size3()); maxReg*=X2;
  sumAbsDif<T> function(vxls,toVxls,wImage,minReg,maxReg);

  CMAES<double>  evo;
  double *const* pop;
  double stddev[nx]= { 0.01, 0.01, 0.01,  0.01, 0.01, 0.001,   0.01, 0.01, 0.001,   0.001, 0.001, 0.001 };
  Parameters<double> parameters;
  parameters.init(nx, xx, stddev);
  double *ovs = evo.init(parameters); // obj func return values
  std::cout << "{ "<<evo.sayHello() << std::endl;

  while(!evo.testForTermination()&& function.calls_count<2000*nx)
  {
    pop = evo.samplePopulation(); // Generate lambda new search points, sample population.  Do not change content of pop
    if(function.calls_count == 0)
      for_i_(0,nx) pop[0][i]=x0[i];
    //Here you may resample each solution point pop[i] until it  becomes feasible, e.g. for box constraints (variable boundaries). function is_feasible(...) needs to be   user-defined.
    //Assumptions: the feasible domain is convex, the optimum is  not on (or very close to) the domain boundary, initialX is  feasible and initialStandardDeviations are sufficiently small  to prevent quasi-infinite looping.
    // for (i = 0; i<evo.get(CMAES<double>::PopSize); ++i)         while (!is_feasible(pop[i]))           evo.reSampleSingle(i);
    int nPop =evo.get(CMAES<double>::Lambda)+0.5;
    OMPFor()
    for (int i=0; i<nPop; ++i)
      ovs[i] = function(nx, pop[i]);

    evo.updateDistribution(ovs);
  }
  //std::cout << "Stop:" << std::endl << evo.getStopMessage();
  //evo.writeToFile(CMAES<double>::WKResume, "resumeevo1.dat"); // write resumable state of CMA-ES
  // std::cout<<nEvals<<" "<<evo.get(CMAES<double>::Lambda)<<std::endl;

  const double *  xxBest = evo.getPtr(CMAES<double>::XBestEver); // "XBestEver" or "XMean" might be used


  Vctr3d   trans = xxBest; trans*=SCALE_TRANS*CrsFctr;
  Tnsr3d   rotat; rotat = xxBest+3;
  Vctr3d offset(origImage.X0()/origImage.dx());
  cout<<"} rotat: \n"<<rotat<<"\n"<<" trans: "<<trans<<"\n"<<" offset: "<<offset<<endl;
  cout<<"function_calls_count: "<< function.calls_count<<endl;
  // cout<<"result: "<< result<<endl;



  const int3 nnn=origImage.size3();
  double dif=0.; size_t count=1;
  int diffcontrast=0;//function.difContrast_;
  OMPragma("omp parallel for reduction(+:dif) reduction(+:count)")
  forAllkji(origImage2)
  {
    Vctr3d ijk = rotat*Vctr3d(i,j,k)+trans-offset;
    if(-0.499<ijk[0] && ijk[0]<nnn[0]-1.  &&   -0.499<ijk[1] && ijk[1]<nnn[1]-1.  &&   -0.499<ijk[2] && ijk[2]<nnn[2]-1.)
    {
      dif+=abs(double(origImage2(i,j,k))-double(origImage(ijk[0],ijk[1],ijk[2])));  ++count;
      origImage2(i,j,k) = min(max(0, int(origImage.vv_mp5(ijk[0],ijk[1],ijk[2]))),imaxT(T)) ;
    }
    else
      origImage2(i,j,k) = min(max(0, int(origImage2(i,j,k))-diffcontrast),imaxT(T));
  }
  origImage = origImage2;


  cout<<"dif: "<< dif/count-diffcontrast<<endl;

  return std::pair<Tnsr3d,Vctr3d>(rotat,trans);

}





namespace MCTProcessing {

template<typename T>  bool registerToImage(stringstream& ins, voxelImageT<T> & vxlImage)  {
  KeyHint("toImgName maskNam  smooth   (bilateralX:nIters  kernRad , Xstp   sigmavv  sigmadd \\\n\t  sharpFact  smotOut toSmOut) bgnReg endReg dX0 thetaz nSkip defaultV");

  cout<<"registerToImage toImgName maskNam  smooth   (bilateralX:nIters  kernRad , Xstp   sigmavv  sigmadd sharpFact  smotOut toSmOut) bgnReg endReg X0 thetaz nSkip defaultV"<<endl;
  string toImgName,maskNam("otsu"),smooth("default"),smotOut("skip"),toSmOut("skip");
  ins >> toImgName >> maskNam >> smooth ;
                (cout<<"registerToImage "<<toImgName<<"  mask: "<<maskNam<<"  smooth: "<<smooth <<"  ").flush();
  int nIters(2), kernRad(2), Xstp(2);   double sigmavv(81.), sigmadd(36.), sharpFact(-0.05); // bilateralXParams

  ensure(smooth=="bilateralX"||smooth=="default","only bilateralX or default is accepted!",2);
  if(smooth=="bilateralX") {
    ins >> nIters >> kernRad >>  Xstp >> sigmavv >> sigmadd >> sharpFact >>smotOut >>toSmOut;
                (cout<<",  bilateralX:  "<<nIters<<" iters of "<<kernRad<<"x"<<Xstp<<"  s_vv:"<<sigmavv<<"  s_dd:"<<sigmadd <<" sh:"<<sharpFact <<"  >>"<< smotOut<<"  >>"<< toSmOut<<" ").flush();
                ensure(sharpFact<0.5,"sharpFact should be less than 2, note order of bilateralX data is changed here",2);
  }


  dbl3 bgnReg(0.2,0.2,0.2), endReg(0.8,0.8,0.8), dX0(0.,0.,0.);   int thetaz(0), defaultV(-1); // nSkipZ
  ins >>bgnReg               >>endReg                >>dX0                >>thetaz     >>defaultV;
                cout<<"  "<<bgnReg<<"  "<<endReg <<"  "<<            dX0<< "  "<<     thetaz<<"  "<<   defaultV<<endl;
                ensure(bgnReg[0]<0.5,"check registerToImage data",2);
                ensure(Xstp<5, "too large s_Xstp size");
                ensure(bgnReg[0]<=0.99 && bgnReg[0]>=0., "bad bgnReg[0]");
                ensure(endReg[0]>=0.01 && endReg[0]<=1., "bad endReg[0]");


  cout<<"registerToImage:  "<<toImgName<<"  "<<maskNam<<"  bilateralX  "<<nIters<<"  "<<kernRad<<"  "<<Xstp<<"  "<<sigmavv<<"  "<<sigmadd<<"  "<<sharpFact<<"  "<<smotOut<<"  "<<toSmOut<<"  "<<bgnReg<<"  "<<endReg<<"  "<<dX0<<"  "<<thetaz<<"  "<<defaultV<<endl;

  voxelImageT<T> vxlsmoot = resampleMean(vxlImage, CrsFctr);
  if(nIters) {
    cout<<"\n from bilateralX:   nIterations:"<<nIters <<", kernel:"<<kernRad<<"  sigmavv:"<<sigmavv <<", sharpFact:"<<sharpFact<<", sigmadd:"<<sigmadd<<" -> "<<smooth<<endl;
    for (int i=0; i<nIters; ++i) {
      bilateralX(vxlsmoot, kernRad*Xstp+kernRad-1,Xstp+(i%Xstp), sigmavv, sharpFact, sigmadd);  cout<<"  ";    }
    if(smotOut.size()>4) vxlsmoot.write(smotOut);
  }

  voxelImageT<T> vxlImage2=copyOrReadImgT<T>(toImgName);


  ensure(vxlImage2.nx(), "in register To Image: can not read image "+toImgName,-1);
  voxelImageT<T> vxlsmot2 = resampleMean(vxlImage2, CrsFctr);
  if(nIters) {
    cout<<"\n  to bilateralX:   nIterations:"<<nIters <<", kernel:"<<kernRad<<"  sigmavv:"<<sigmavv <<", sharpFact:"<<sharpFact<<", sigmadd:"<<sigmadd<<" -> "<<toSmOut<<endl;
    for (int i=0; i<nIters; ++i) {
      bilateralX(vxlsmot2,kernRad*Xstp+kernRad-1,Xstp+(i%Xstp), sigmavv, sharpFact, sigmadd);  cout<<"  ";   }
    if(toSmOut.size()>4) vxlsmot2.write(toSmOut);
  }

  voxelImageT<T> wImage=vxlsmot2; // used as weight
  voxelImage maskImg;

  if (maskNam!="otsu") {
    maskImg= resampleMode(copyOrReadImgT<unsigned char>(maskNam),CrsFctr);
    ensure(maskImg.size3()==vxlsmot2.size3(),"",3);
  }
  else
    maskImg = otsu_th01(vxlsmot2,0,maxT(T),0.3);



  ensure(maskImg.size3()==vxlsmot2.size3());
  vxlsmoot = mean(median(magGradient((vxlsmoot),4)));
  vxlsmot2 = mean(median(magGradient((vxlsmot2),4)));

  forAllvp_(vxlsmoot)   if(*vp<2) *vp=2;
  forAllvp_(vxlsmot2)  if(*vp<2) *vp=2;

  //! mask to rock phase +2 voxels
  maskImg.FaceMedian06(2,4);
  maskImg.growPore();
  maskImg.FaceMedian06(2,4);
  maskImg.write("maskImg.tif");
  forAlliii_(maskImg)  if(maskImg(iii)==0) vxlsmot2(iii)=0;//! 0 is exclusion mask
  maskImg.reset(int3(0,0,0));

  if(smotOut.size()>4)  vxlsmoot.write("mgrad_"+smotOut);
  if(toSmOut.size()>4)  vxlsmot2.write("mgrad_"+toSmOut);

  (cout<<" \n ... ").flush();
  //std::pair<Tnsr3d,Vctr3d> trnsf =
  registerToImageEMS7DOF(vxlImage,vxlsmoot,vxlImage2,vxlsmot2, wImage, bgnReg, endReg, dX0, thetaz, defaultV);


  (cout<<".").flush();
  return true;
}



template<typename T>  void resetTransform(voxelImageT<T>& vImg, const Vctr3d& trans, const Tnsr3d& rotat, const voxelImageT<T>& image2)
{
  int3 nnn=image2.size3();
  (cout<<"nnn: "<<nnn<<" ").flush();
  (cout<<"vImg.size: "<<vImg.size3()<<" ").flush();

  Tnsr3d rotatInv; rotat.inverse(rotatInv);
  size_t nChanged=0;
  forAllkji_(vImg)
  {
    Vctr3d ijk_ =rotatInv*Vctr3d(i,j,k)-trans+0.5;
    int3 ijk{int(ijk_[0]),int(ijk_[1]),int(ijk_[2])};
    if(0<=ijk[0] && ijk[0]<nnn[0]  &&   0<=ijk[1] && ijk[1]<nnn[1]  &&   0<=ijk[2] && ijk[2]<nnn[2]) {
      vImg(i,j,k) = image2(ijk[0],ijk[1],ijk[2]);
      ++nChanged;
    }
  }
  (cout<<"nChanged: "<<nChanged<<" ").flush();
}


template<typename T>  bool readTransFrom( stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("img2Nam translate3(0,0,0) rotate3x3(Unity)");
  string img2Nam;
  ins>>img2Nam;
  cout<<"{   mapping from  image "<<img2Nam;
  Vctr3d  trans=Vctr3d::ZERO;
  Tnsr3d  rotat=Tnsr3d::IDENTITY();
  ins>>trans>>rotat;
  if(img2Nam.size()>4)
  {
    cout<<" trans: "<<trans<<",  rotat: "<< rotat<<endl;
    voxelImageT<T> image2(img2Nam, readOpt::procOnly);
    cout<<" OrigInfo: "<<endl;  vImg.printInfo();
    cout<<" Img2Info: "<<endl;  image2.printInfo();
    resetTransform(vImg,trans,rotat,image2);
    cout<<" NewImfo: "<<endl;  vImg.printInfo();
  }
  std::cout<<"}"<<std::endl;
  return true;
}

}  // namespace MCTProcessing
