
#include "globals.h"
#include "voxelImage.h"

namespace VoxLib {
using namespace std;

inline void zerogradVecBC(vector<double>& vs) {vs[0]=vs[1];  vs[vs.size()-1]=vs[vs.size()-2]; }
constexpr double _PI = 3.141592653589793;

template<typename T>
struct sumAbsDif_XYR {
  sumAbsDif_XYR(const voxelImageT<T>& vImage,T treshold, bool cuthighs=false)    {
    int3 nnn=vImage.size3();
    voxelImageT<T> voxels = median(vImage);

    xAvg.resize(nnn[2],0.);
    yAvg.resize(nnn[2],0.);
    Rad.resize(nnn[2],0.);
    OMPFor()
    for (int k=0; k<nnn[2]; ++k)  {
      vector<int> imins(nnn[1],nnn[0]);
      vector<int> imaxs(nnn[1],0);
      for (int j=0; j<nnn[1]; ++j)
        if(cuthighs)  {
          for (int i=0; i<nnn[0]; ++i)
          if(voxels(i,j,k)<treshold)  {
            if(i>imaxs[j]) imaxs[j]=i;
            if(i<imins[j]) imins[j]=i;
          }
        }
        else  {
          for (int i=0; i<nnn[0]; ++i)
          if(voxels(i,j,k)>treshold)  {
            if(i>imaxs[j]) imaxs[j]=i;
            if(i<imins[j]) imins[j]=i;
          }
        }

      size_t Area=0,centrei=0,centrej=0;
      for (int j=0; j<nnn[1]; ++j)  {
        int Dx=max(0,imaxs[j]-imins[j]);
        Area+=Dx;
        centrei+=(Dx*(Dx+1+2*imins[j]))/2;
        centrej+=Dx*(j);
      }
      xAvg[k]=centrei/(Area+1e-300);
      yAvg[k]=centrej/(Area+1e-300);
      Rad[k]=sqrt(Area/_PI);
    }

    //- smooth the arrays

    for (int i=1; i<2; ++i)  {
      vector<double>  oxAvg=xAvg,  oyAvg=yAvg,   oRAvg=Rad;
      for (int k=1; k<nnn[2]-1; ++k)  {
        xAvg[k]=0.25*(oxAvg[k-1]+2.*oxAvg[k]+oxAvg[k+1]);
        yAvg[k]=0.25*(oyAvg[k-1]+2.*oyAvg[k]+oyAvg[k+1]);
        Rad[k]=max( max(oRAvg[k-1],oRAvg[k]), oRAvg[k+1]);
      }
      zerogradVecBC(xAvg); zerogradVecBC(yAvg); zerogradVecBC(Rad);
    }
    for (int i=1; i<2; ++i)  {
      vector<double> oxAvg=xAvg,  oyAvg=yAvg,   oRAvg=Rad ;
      for (int k=1; k<nnn[2]-1; ++k)  {
        xAvg[k]=0.25*(oxAvg[k-1]+2.*oxAvg[k]+oxAvg[k+1]);
        yAvg[k]=0.25*(oyAvg[k-1]+2.*oyAvg[k]+oyAvg[k+1]);
        Rad[k] =0.25*(oRAvg[k-1]+2.*oRAvg[k]+oRAvg[k+1]);
      }
      zerogradVecBC(xAvg); zerogradVecBC(yAvg); zerogradVecBC(Rad);
    }
  };


  // function return values!
  vector<double> xAvg;/// inside average x
  vector<double> yAvg;/// inside average y
  vector<double> Rad; /// inside radius
};


template<typename T>
void cutOutside(voxelImageT<T>& vImage, char dir='z', int nExtraOut=1, int threshold=-1, bool cuthighs=false, int shiftX=0, int shiftY=0, T outVal=maxT(T)) { // TODO yet to be tested

  int3 nnn=vImage.size3();
  ensure(dir=='z');

  if(threshold<0)  {
    array<double,5> thresholdsOtsu = otsu_th(vImage, Tint(1), TImax(T)-1, 0.2);
    if (cuthighs) threshold=0.5*thresholdsOtsu[3]+0.5*thresholdsOtsu[3];
    else          threshold=0.5*thresholdsOtsu[3]+0.5*thresholdsOtsu[2];
    cout<<"  threshold:  "<<threshold<<",  "; cout.flush();
  }

  sumAbsDif_XYR<T> function(vImage,threshold);


  double R2Sum = 0.;
  OMPragma("omp parallel for reduction(+:R2Sum)")
  for (int k=0; k<nnn[2]; ++k)
  {
    double X0=    function.xAvg[k]+shiftX;
    double Y0=    function.yAvg[k]+shiftY;
    double Rsqr=  function.Rad[k]-nExtraOut; Rsqr*=Rsqr; R2Sum+=Rsqr;
    for (int j=0; j<nnn[1]; ++j)
      for (int i=0; i<nnn[0]; ++i)
        if((i-X0)*(i-X0)+(j-Y0)*(j-Y0)>Rsqr) vImage(i,j,k)=outVal;
  }

  cout<<" Mean image diameter:  "<<int(2.*sqrt(R2Sum/nnn[2]))<<",  n:"<<nnn[0]<<" "<<nnn[1]<<endl;
}
} // namespace VoxLib
