
#include "VxlStrips.h"
#include "globals.h"

#include <iostream>

using namespace std;


VxlStrips::VxlStrips(const InputFile& inp)
    : nx(0), ny(0), nz(0), vxlSize(1), X0(0.,0.,0.)
{
  (cout<< "InputData: ").flush();
  inp.echoKeywords(std::cout);

  std::srand(1001);

  if (!inp.giv("DefaultImageFormat", imgfrmt)) imgfrmt=".raw.gz";
  if(imgfrmt[0]!='.') imgfrmt="."+imgfrmt;
  imgExt(imgfrmt);
  cout<<" DefaultImageFormat: "<<imgfrmt<<endl;

  nBP6 = inp.getOr("multiDir",true) ? 6 : 2;
  inp.giv("nOpenSides",nBP6);

  cout<<" voxel indices:"<<endl;
  rockTypes_.push_back({0,0});
  inp.giv("VoidRange", rockTypes_.back());
  cout<<"  "<<0<<": void voxels "<<endl;

  istringstream iss;

  if(inp.giv("porousRanges", iss))  { // DAR_0:
    int2 rng{int(rockTypes_.size()),int(rockTypes_.size())};
    while(iss>>rng)  {
      int vi=rockTypes_.size();
      cout<<"  "<<vi<<": porousRange = "<<rng<<endl;
      rockTypes_.push_back(int2(rng.a,rng.b));
    }
    cout<< "  number of rock types: "<<rockTypes_.size()<<endl;
  }  // DAR_0;

  segValues.resize(256, rockTypes_.size());

  for_(rockTypes_,i)  {
    auto& rt=rockTypes_[i];
    for(int j=rt.a; j<=rt.b; ++j)  segValues[j] = i;
    cout<<"  RT "<<i<<" voxel values: ["<<rt<<"]"<<endl;
  }

  cout<<"  Voxel value indices:";
  for(size_t i=0; i<=12; ++i)    cout<<" "<<segValues[i];
  cout<<" ... "<<endl;

}


void VxlStrips::setImageInfo(const InputFile& inp, const voxelImage& VImage) {

  vxlSize = VImage.dx().x;
  ensure(vxlSize>1e-18 || vxlSize<1e18, "bad voxel size",-54754);

  X0=VImage.X0();
  nx=VImage.nx();  ny=VImage.ny();  nz=VImage.nz();
  ensure(nz,"image not read",2);
  VImage.printInfo();

  nInside= (long long)(nx)*ny*nz;
  cout<<" size: "<<VImage.size3()<<", vxlSize: "<<vxlSize<<", X0: "<<X0<<endl;

  int2 outrange;
  if(inp.giv("outside_range", outrange))  {
    forAllcp(VImage) if(outrange.a<=(*cp) && (*cp)<=outrange.b) --nInside;
    cout<<" outside_range: "<<outrange<<"; inside_fraction: "<<nInside/(double(nx)*ny*nz)<<endl;
  }
}

void VxlStrips::createStripsX(const voxelImage& VImage) {

  nVxlVs.resize(rockTypes_.size()+1,0);
  segXs_.resize(nz+2); for(auto& ss:segXs_) ss.resize(ny+2);

  #ifndef _MSC_VER // MSVC OpenMP support is primitive
  #ifdef OpenMP
    #pragma   omp declare reduction(vec_sizet_plus : vector<size_t> : \
          transform(omp_out.begin(), omp_out.end(), omp_in.begin(), omp_out.begin(), plus<size_t>())) \
          initializer(omp_priv = omp_orig)
  #endif

  OMPragma("omp parallel for  reduction(vec_sizet_plus : nVxlVs)")
  #endif
  for (int iz = 0; iz<nz; ++iz)  {
    stvec<strip> segTmp(nx+1);
    for (int iy = 0; iy<ny; ++iy)  {
      int cnt = 0;
      int  currentSegVal = 257;
      for (int ix = 0; ix<nx; ++ix) {
        if (unsigned char vV = VImage(ix,iy,iz); segValues[vV] != currentSegVal) {
          currentSegVal = segValues[vV];
          segTmp[cnt].i0 = ix;
          segTmp[cnt].vv = currentSegVal;
          ++cnt;
        }
      }
      segTmp[cnt].i0 = nx;  segTmp[cnt].vv = 254;

      strips & ss = segXs_[iz+1][iy+1];
      ss.reSize(cnt);

      for (int i = 0; i<cnt; ++i) {
        nVxlVs[segTmp[i].vv]+=segTmp[i+1].i0-segTmp[i].i0;
        ss.sts[i].i0 = segTmp[i].i0;
        ss.sts[i].vv = segTmp[i].vv;
        dAsrt (i==0 || ss.sts[i].vv != ss.sts[i-1].vv) ;
      }
      ss.sts[cnt].i0 = nx+_1ExtraSegX;  ss.sts[cnt].vv = 254; // FIXME // #tagXm1N2
      ss.sts[0  ].i0 = -_1ExtraSegX;
    }
    segXs_[iz+1][ny+1]=segXs_[iz+1][ny];
    segXs_[iz+1][0   ]=segXs_[iz+1][1 ];
  }
  segXs_[nz+1]=segXs_[nz];
  segXs_[0   ]=segXs_[1 ];


  cout<< endl;
  size_t nVInsids=0;
  for_(rockTypes_,i)  {
    nVInsids+=nVxlVs[i];
    cout<<" "<<i<<" totalVol: "<<nVxlVs[i]<<" voxels, "<< nVxlVs[i]/(double(0.01*nx)*ny*nz) << "%"<<endl;
  }
  ensure(nVInsids>double(0.01*nx)*ny*nz, "too few voxels, set 'VoidRange' maybe?", -1);
  cout<< endl;

}


// Testing TODO
voxelImage segToVxlMesh(const VxlStrips & ref)  {/// converts strips back to voxelImage
  voxelImage vxls({ref.nx,ref.ny,ref.nz},255);
  for (int iz = 0; iz<ref.nz; ++iz)
    for (int iy = 0; iy<ref.ny; ++iy)  {
      const strips& sgs = ref.segXs_[iz+1][iy+1];
      for (int ix = 0; ix<sgs.cnt; ++ix)
        std::fill (&vxls(sgs.sts[ix].i0,iy,iz), &vxls(sgs.sts[ix+1].i0,iy,iz), sgs.sts[ix].vv);
    }
  return vxls;
}



DistMap::DistMap(const InputFile& inp, VxlStrips& cfg, size_t ivVal)//, double vmvLimRelF, double crossAreaf
  : inVxVal_(ivVal)
  , segXs_(cfg.segXs_)
{
  _nRSmoothing=1;
  _clipROutx=0.05;
  _clipROutyz=0.98;
  if(cfg.nBP6==6)   _clipROutyz=_clipROutx;

  if(inVxVal_) // DAR_1:
  {
    _clipROutyz = 1.;
  }


  if (std::istringstream iss;
      inp.giv("DistMapSettings"+_s(inVxVal_), iss) || inp.giv("DistMapSettings", iss)
     ) iss >> _clipROutx >> _clipROutyz >> _nRSmoothing;
  cout<<"#!         clipROut.x   .yz    nRSmoothing:"<<endl;
  cout<<"DistMapSettings: "  << _clipROutx<<"  " << _clipROutyz<< "           "<< _nRSmoothing  <<endl;

  invalidSeg.i0=-10000;
  invalidSeg.vv=255;


  nx = cfg.nx;
  ny = cfg.ny;
  nz = cfg.nz;

}

/// dj = nearest solid relative position ...
float DistMap::calc_distmap(int i, int j, int k, int3& dj) {
  if (dj.x >= 0 || dj.y >= 0 || dj.z >= 0) {
    // FIXME: this is to avoid generating random big throats, probably something wrong elsewhere, 
    // and also the logic below is wrong
    dj.x = max(0, min(nx - 1, dj.x));
    dj.y = max(0, min(ny - 1, dj.y));
    dj.z = max(0, min(nz - 1, dj.z));
  }

  int rSqr = sqr(dj.x-i)+sqr(dj.y-j)+sqr(dj.z-k);

  int zmax = min(nz-k-1,int( sqrtf(rSqr)+1.9f));
  int ymax = min(ny-j-1,int( sqrtf(rSqr)+1.9f));

  for (int c = max(-k,int(-sqrtf(rSqr)-1.9f)); c<0 || c*c <=  min(zmax*zmax,rSqr); ++c) {
    for (int b = max(-j,int(-sqrtf(abs(rSqr-c*c))-1.9f)); b<0 || b*b <=  min(ymax*ymax,rSqr-c*c); ++b) {
      const strip& sX = segX0(i, j+b, k+c);
      int b2c2 = b*b+c*c;
      if(sX.vv!=inVxVal_) {
        if(b2c2 < rSqr)  {
          rSqr = b2c2;  dj.x = i;  dj.y = j+b;  dj.z = k+c;
          if(b<0) b=max(b,-int(sqrtf(rSqr+1)));
        }
        continue;
      }

      if(int a = sX.i0-1-i;   a>=-i && a*a+b2c2 < rSqr) {
        rSqr = a*a+b2c2;  dj.x = i+a;  dj.y = j+b;  dj.z = k+c;
        //if(b<0) b=max(b,-int(sqrtf(rSqr+1)));
      }

      if(int a = (&sX+1)->i0-i;  a<nx-i && a*a+b2c2 < rSqr) {
        rSqr = a*a+b2c2;  dj.x = i+a;  dj.y = j+b;  dj.z = k+c;
        //if(b<0) b=max(b,-int(sqrtf(rSqr+1)));
      }
    }
    if(c<0)  c=max(c,-int(sqrtf(rSqr-1)));
  }

  assert(rSqr<40000);

  if ( ! isInside(dj.x,dj.y,dj.z)) {
      dj.x = (i<nx/2) ? -nx/4-1  : nx*5/4+1;
      dj.y = (j<ny/2) ? -ny/4-1  : ny*5/4+1;
      dj.z = (k<nz/2) ? -nz/4-1  : nz*5/4+1;
      return sqrt(sqr(dj.x-i) + sqr(dj.y-j) + sqr(dj.z-k)) - 0.5;
  }
  else {
    int dx=abs(dj.x-i),  dy=abs(dj.y-j),  dz=abs(dj.z-k);

    float limit = sqrt(dx*dx + dy*dy + dz*dz) - 0.5;
    float iSqr;
    if (iSqr=min((j+2),(ny-j+1)); iSqr<limit)
      limit=max((1.-_clipROutyz)*limit+_clipROutyz*iSqr,0.01);
    if (iSqr=min((k+2),(nz-k+1)); iSqr<limit)
      limit=max((1.-_clipROutyz)*limit+_clipROutyz*iSqr,0.01);
    if (iSqr=min((i+2),(nx-i+1)); iSqr<limit)
      limit=max((1.-_clipROutx )*limit+_clipROutx*iSqr,0.1);
    return limit;
  }
}


/// dj = nearest solid relative position ...
float  DistMap::calc_distmapf(dbl3 mb, int3& dj) {
  int i=mb.x, j=mb.y, k=mb.z;

  if(const strip& sX = segX0(i, j, k); sX.vv!=inVxVal_) return 0.0f;

  if (dj.x >= 0 || dj.y >= 0 || dj.z >= 0) {
    dj.x = max(0, min(nx - 1, dj.x));
    dj.y = max(0, min(ny - 1, dj.y));
    dj.z = max(0, min(nz - 1, dj.z));
  }

  int rSqr = sqr(dj.x-mb.x)+sqr(dj.y-mb.y)+sqr(dj.z-mb.z);
  float rrr = sqrtf(rSqr)+1.9f;

  int zmax = min(nz-k-1,int(rrr));
  int ymax = min(ny-j-1,int(rrr));

  for (int c = max(-k,int(-rrr)); c<0 || c*c <=  min(zmax*zmax,rSqr); ++c) {
    for (int b = max(-j,int(-sqrtf(abs(rSqr-c*c))-1.9f)); b<0 || b*b <=  min(ymax*ymax,rSqr-c*c); ++b) {
      const strip& sX = segX0(i, j+b, k+c);
      int b2c2 = b*b+c*c;
      if(sX.vv!=inVxVal_) {
        if(b2c2 < rSqr)  {
          rSqr = b2c2;  dj.x = i;  dj.y = j+b;  dj.z = k+c;
          if(b<0) b=max(b,-int(sqrtf(rSqr+1)));
        }
        continue;
      }

      if(int a = sX.i0-1-i;   a>=-i && a*a+b2c2 < rSqr) {
        rSqr = a*a+b2c2;  dj.x = i+a;  dj.y = j+b;  dj.z = k+c;
        //if(b<0) b=max(b,-int(sqrtf(rSqr+1)));
      }

      if(int a = (&sX+1)->i0-i;  a<nx-i && a*a+b2c2 < rSqr) {
        rSqr = a*a+b2c2;  dj.x = i+a;  dj.y = j+b;  dj.z = k+c;
        //if(b<0) b=max(b,-int(sqrtf(rSqr+1)));
      }
    }
    if(c<0)  c=max(c,-int(sqrtf(rSqr-1)));
  }

  assert(rSqr<40000);

  if ( ! isInside(dj.x,dj.y,dj.z)) {
      dj.x = (i<nx/2) ? -nx/4-1  : nx*5/4+1;
      dj.y = (j<ny/2) ? -ny/4-1  : ny*5/4+1;
      dj.z = (k<nz/2) ? -nz/4-1  : nz*5/4+1;
      return sqrt(sqr(dj.x-mb.x) + sqr(dj.y-mb.y) + sqr(dj.z-mb.z)) - 0.5;
  }
  else {
    int dx=abs(dj.x-mb.x),  dy=abs(dj.y-mb.y),  dz=abs(dj.z-mb.z);

    float limit = sqrt(dx*dx + dy*dy + dz*dz) - 0.5;
    float iSqr;
    if (iSqr=min((j+2),(ny-j+1)); iSqr<limit)
      limit=max((1.-_clipROutyz)*limit+_clipROutyz*iSqr,0.01);
    if (iSqr=min((k+2),(nz-k+1)); iSqr<limit)
      limit=max((1.-_clipROutyz)*limit+_clipROutyz*iSqr,0.01);
    if (iSqr=min((i+2),(nx-i+1)); iSqr<limit)
      limit=max((1.-_clipROutx )*limit+_clipROutx*iSqr,0.1);
    return limit;
  }
}

voxelImage readImageU8(const InputFile& inp) {

  string fnam(inp.fileName());
  if(fnam.empty()) { inp.giv("ElementDataFile", fnam) || inp.giv("read", fnam); }
  cout<<" Image file: "<<fnam<<endl;

  voxelImage VImage(fnam,readOpt::procAndConvert);

  return VImage;
}

voxelImage distMapExtrude(const voxelImage& VImage, const InputFile& inp, double offsetR, double scaleR, double powerR)  {

  VxlStrips cfg(inp);

  int ivVal=0;

  cfg.setImageInfo(inp, VImage);    // read image
  cfg.createStripsX(VImage); // RLE compress image

  DistMap srf(inp, cfg, ivVal);

  const int nx=cfg.nx, ny=cfg.ny; constexpr int nz1=1;
  assert(cfg.nz==1);
  constexpr int iz=0;

  cout<< " nx   "<<nx<< " "<<ny<< " "<<nz1<<endl;
  cout<< " computing distance map for index "<<int(ivVal)<<endl;

   voxelImageT<float> rads(int3(nx,ny,nz1),0.);
  float maxrrr=0;
  OMPragma("omp parallel for reduction(max:maxrrr)")
  for (int iy = 0; iy<ny; ++iy)  {
    int3 neilian{-nx,-ny,-nz1};
    for (int ix = 0; ix<nx; ++ix)  if (VImage(ix,iy,0)==0) {
      float rr = srf.calc_distmap(ix, iy, iz, neilian);
      rr = std::pow(scaleR * rr, powerR);
      maxrrr = max(rr,maxrrr);
      rads(ix,iy,iz) = rr;
    }
  }
   //rads.write("distMapExtrude_radius.mhd");
  cout<< "maxrrr radius = "<<maxrrr<<endl;

  voxelImage vxls(int3(nx,ny,int(maxrrr+0.5)+2), 255, dbl3{cfg.vxlSize},dbl3{0.});

  int offCenter = maxrrr*offsetR + 1.501;
  OMPFor()
  for (int iy = 0; iy<ny; ++iy)
    for (int ix = 0; ix<nx; ++ix) {
      double rr = rads(ix,iy,iz);
      int nr = rr+0.9;
      int offset = offCenter - nr*offsetR;
      for (int ir=0; ir<nr; ++ir)  vxls(ix,iy,ir+offset)=0;
    }

  // vxls.write(fnam+"3D"+cfg.imgfrmt);
  return vxls;
}
