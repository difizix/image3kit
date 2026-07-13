#include "globals.h"
#include "typses.h"
#include "voxelImage.h"
#include <filesystem>
#include <set>
namespace fs = std::filesystem;
using namespace std;


template<typename T>  bool voxelTimeMedian( std::stringstream & ins, voxelImageT<T>& vImage)
{
  string inDir;     ins>>inDir;
  string sfx;       ins>>sfx;
  string ouDir;     ins>>ouDir;
  int   kernelN;    ins>>kernelN;

  set<string> imgNams;
  auto lx=sfx.size();
  cout<<"files:"<<endl;
  for(auto& p: fs::directory_iterator(inDir))
  {
    string pStr = p.path().string();
    auto lp=pStr.size();
    if(lp>lx&& pStr.compare(lp-lx,lx,sfx) == 0)
    {
      imgNams.insert(pStr);
      cout<<pStr<<endl;
    }
    else cout<<pStr<<", skipped"<<endl;
  }
  cout<<"running voxelTimeMedian on "<<imgNams.size()<<" images, kernelN: "<<kernelN<<endl;
  bool created = fs::create_directory(ouDir);
  ensure(created, "can not create new directory "+ouDir);

  ensure(kernelN==3);
  size_t mid = kernelN/2;
  vector<voxelImageT<T> > imgs(kernelN);
  imgs[0]=vImage;
  imgs[0].readBin(*imgNams.begin());
  for(int i=1; i<imgs.size(); ++i) imgs[i]=imgs[0];
  for(auto imgnam=std::next(imgNams.begin()); imgnam!=std::prev(imgNams.end()); ++imgnam)
  {  int i=0;
    for(; i<imgs.size()-1; ++i) imgs[i]=std::move(imgs[i+1]);
    imgs[i+1].readBin(inDir+"/"+*imgnam);
    forAlliii_(vImage)
    {
      std::vector<T> vvs(imgs.size(),0);
      for_(imgs, j) vvs[j]=imgs[j](iii);
      std::nth_element(vvs.begin(),vvs.begin()+mid,vvs.end());
      vImage(iii) = vvs[mid];
    }
    vImage.writeBin(ouDir+"/"+*imgnam);
  }

  return false;
}
