#pragma once

#include <iostream>
#include <string>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <cctype>
#include <type_traits>

#include "stb_image_write.h"
#include "stb_image.h"


#ifdef min // to avoid conflict with windows.h mess
#undef min
#undef max
#endif

#include "IOUtils.h"
#include "globals.h"

#include "typses.h"
#include "voxelImage.h"

#include <svg_color.hpp>

//template <typename T>
//inline void setRGB(png_byte *ptr, T val, T bgnv, T endv)
//{
  //int v = (int(val-bgnv) * 767)/(endv-bgnv);  // = f*767
  //if (v < 0) v = 0;w
  //if (v > 767) v = 767;
  //int offset = v % 256;

  //if (v<256)      { ptr[0] = 0; ptr[1] = 0; ptr[2] = offset; }
  //else if (v<512) { ptr[0] = 0; ptr[1] = offset; ptr[2] = 255-offset; }
  //else            { ptr[0] = offset; ptr[1] = 255-offset; ptr[2] = 0; }
//}



template <typename T>
int slice2RGBAPng(const voxelImageT<T>& imgRGB, char axis, std::string fnam, int iSlice, T bgnv, T endv, const voxelImageT<T>& imgAlf, T bgna, T enda)  {
  std::cout<<"Writing "<<fnam<<" \n";  // #WViz

  int width=0, height=0, CHANNEL_NUM=4;
  auto nnn=imgRGB.size3();
  switch (axis) {
    case 'x':  width=nnn[2]; height=nnn[1]; ensure(iSlice<nnn[0]," i_slice ?<"+_s(nnn[0]),2);  break;
    case 'y':  width=nnn[0]; height=nnn[2]; ensure(iSlice<nnn[1]," i_slice ?<"+_s(nnn[1]),2);  break;
    case 'z':  width=nnn[0]; height=nnn[1]; ensure(iSlice<nnn[2]," i_slice ?<"+_s(nnn[2]),2);  break;
  }
  int code = 0;

  uint8_t* pixels = new uint8_t[width * height * CHANNEL_NUM];

  // Write image data
  T minvv=bgnv, maxvv=endv;   if(minvv>maxvv) std::swap(minvv,maxvv);
  float ZdelV=1./(double(endv)-bgnv);
  T minaa=bgna, maxaa=enda;   if(minaa>maxaa) std::swap(minaa,maxaa);
  float ZdelA=255./(double(enda)-bgna);
  int x, y;
  for (y=0 ; y<height ; y++) {
    auto row = pixels + (y * width) * CHANNEL_NUM;
    for (x=0 ; x<width ; x++) {
      size_t ind=0;
      switch (axis) {
        case 'x': ind=imgRGB.index(iSlice,y,x);  break;
        case 'y': ind=imgRGB.index(x,iSlice,y);  break;
        case 'z': ind=imgRGB.index(x,y,iSlice);  break;
      }
      float v = (float(min(max(minvv,imgRGB(ind)),maxvv))-bgnv)*ZdelV;
      //auto clr = svg::rgbtween(v, svg::svg_color(20,30,0), svg::svg_color(250,220,221)); // good for rock, goes through pink
      //auto clr = svg::rgbtween(v, svg::svg_color(0,0,100), svg::svg_color(255,110,100));
      auto clr = svg::rgbtween(v, svg::svg_color(10,11,150), svg::svg_color(210,190,170));
      row[x*4]=clr.r_; row[x*4+1]=clr.g_; row[x*4+2]=clr.b_; row[x*4+3]=(float(min(max(minaa,imgAlf(ind)),maxaa))-bgna)*ZdelA;
    }
  }

   stbi_write_png(fnam.c_str(), width, height, CHANNEL_NUM, pixels, width * CHANNEL_NUM);
   delete[] pixels;

  return code;
}


template <typename T>
int slice2RGBXPng(const voxelImageT<T>& imgRGB, char axis, std::string fnam, int iSlice, T bgnv, T endv, const voxelImageT<T>& imgAlf, T bgna, T enda, float wt)  {
  std::cout<<"Writing "<<fnam<<" \n";  // #WViz

  int width, height, CHANNEL_NUM=3;
  auto nnn=imgRGB.size3();
  switch (axis) {
    case 'x':  width=nnn[2]; height=nnn[1]; ensure(iSlice<nnn[0]," i_slice ?<"+_s(nnn[0]),2);  break;
    case 'y':  width=nnn[0]; height=nnn[2]; ensure(iSlice<nnn[1]," i_slice ?<"+_s(nnn[1]),2);  break;
    case 'z':  width=nnn[0]; height=nnn[1]; ensure(iSlice<nnn[2]," i_slice ?<"+_s(nnn[2]),2);  break;
  }
  int code = 0;

  uint8_t* pixels = new uint8_t[width * height * CHANNEL_NUM];

  float wv=1.0f-wt;
  // Write image data
  T minvv=bgnv, maxvv=endv;   if(minvv>maxvv) std::swap(minvv,maxvv);
  float ZdelV=1./(double(endv)-bgnv);
  T minaa=bgna, maxaa=enda;   if(minaa>maxaa) std::swap(minaa,maxaa);
  float ZdelA=127./(double(enda)-bgna);
  int x, y;
  for (y=0 ; y<height ; y++) {
    auto row = pixels + (y * width) * CHANNEL_NUM;
    for (x=0 ; x<width ; x++)
    {
      size_t ind=0;
      switch (axis) {
        case 'x': ind=imgRGB.index(iSlice,y,x);  break;
        case 'y': ind=imgRGB.index(x,iSlice,y);  break;
        case 'z': ind=imgRGB.index(x,y,iSlice);  break;
      }
      float vv = (float(std::min(std::max(minvv,imgRGB(ind)),maxvv))-bgnv)*ZdelV;
      auto clr = svg::rgbtween(vv, svg::svg_color(10,11,150), svg::svg_color(255,51,50));
      auto c2r = (float(std::min(std::max(minaa,imgAlf(ind)),maxaa))-bgna)*ZdelA;
      row[x*3]=clr.r_*wv+wt*c2r; row[x*3+1]=clr.g_*wv+wt*c2r; row[x*3+2]=clr.b_*wv+wt*c2r;
    }
  }

   stbi_write_png(fnam.c_str(), width, height, CHANNEL_NUM, pixels, width * CHANNEL_NUM);
   delete[] pixels;

  return code;
}





template <typename T>
int slice2RGBPng(const voxelImageT<T>& vImage, char axis, std::string fnam, int iSlice, T bgnv, T endv)  {

  std::cout<<"Writing "<<fnam<<" \n";  // #WViz
  int code = 0;

  int width=0, height=0, CHANNEL_NUM=3;
  auto nnn=vImage.size3();
  switch (axis) {
    case 'x':  width=nnn[2]; height=nnn[1]; ensure(iSlice<nnn[0]," i_slice ?<"+_s(nnn[0]),2);  break;
    case 'y':  width=nnn[0]; height=nnn[2]; ensure(iSlice<nnn[1]," i_slice ?<"+_s(nnn[1]),2);  break;
    case 'z':  width=nnn[0]; height=nnn[1]; ensure(iSlice<nnn[2]," i_slice ?<"+_s(nnn[2]),2);  break;
  }

  uint8_t* pixels = new uint8_t[width * height * CHANNEL_NUM];

  // Write image data
  T minvv=bgnv, maxvv=endv;  if(minvv>maxvv) std::swap(minvv,maxvv);
  float ZdelV=1./(double(endv)-bgnv);
  int x, y;
  for (y=0 ; y<height ; y++) {
    auto row = pixels + (y * width) * CHANNEL_NUM;
    for (x=0 ; x<width ; x++)
    {
      size_t ind=0;
      switch (axis) {
        case 'x': ind=vImage.index(iSlice,y,x);  break;
        case 'y': ind=vImage.index(x,iSlice,y);  break;
        case 'z': ind=vImage.index(x,y,iSlice);  break;
      }
      float v = float(std::min(std::max(minvv,vImage(ind)),maxvv)-bgnv)*ZdelV;
      //auto clr = svg::rgbtween(v, svg::svg_color(20,30,0), svg::svg_color(250,220,221)); // good for rock, goes through pink
      //auto clr = svg::rgbtween(v, svg::svg_color(0,0,100), svg::svg_color(255,110,100));
      auto clr = svg::rgbtween(v, svg::svg_color(140,40,150), svg::svg_color(250,220,210));
      row[x*3]=clr.r_; row[x*3+1]=clr.g_; row[x*3+2]=clr.b_;
    }
  }

   stbi_write_png(fnam.c_str(), width, height, CHANNEL_NUM, pixels, width * CHANNEL_NUM);
   delete[] pixels;

  return code;
}


template <typename T>
int slice2GrayPng(const voxelImageT<T>& vImage, char axis, std::string fnam, int iSlice, T bgnv, T endv){

  std::cout<<"Writing "<<fnam<<" \n";  // #WViz
  int code = 0;

  int width=0, height=0, CHANNEL_NUM=1;
  auto nnn=vImage.size3();
  switch (axis) {
    case 'x':  width=nnn[2]; height=nnn[1]; ensure(iSlice<nnn[0]," i_slice ?<"+_s(nnn[0]),2);  break;
    case 'y':  width=nnn[0]; height=nnn[2]; ensure(iSlice<nnn[1]," i_slice ?<"+_s(nnn[1]),2);  break;
    case 'z':  width=nnn[0]; height=nnn[1]; ensure(iSlice<nnn[2]," i_slice ?<"+_s(nnn[2]),2);  break;
  }

  uint8_t* pixels = new uint8_t[width * height * CHANNEL_NUM];

  // Write image data
  T minvv=bgnv, maxvv=endv;   if(minvv>maxvv) std::swap(minvv,maxvv);
  float ZdelV=255./(double(endv)-bgnv);
  int x, y;
  for (y=0 ; y<height ; y++) {
    auto row = pixels + (y * width) * CHANNEL_NUM;
    for (x=0 ; x<width ; x++)
    {
      size_t ind=0;
      switch (axis) { // move out of the loop
        case 'z': ind=vImage.index(x,y,iSlice);  break;
        case 'x': ind=vImage.index(iSlice,y,x);  break;
        case 'y': ind=vImage.index(x,iSlice,y);  break;
      }
      row[x] = (float(std::min(std::max(minvv,vImage(ind)),maxvv))-bgnv)*ZdelV;
    }
  }

   stbi_write_png(fnam.c_str(), width, height, CHANNEL_NUM, pixels, width * CHANNEL_NUM);
   delete[] pixels;

  return code;
}


template <typename T>
int sliceFromPng(voxelImageT<T>& vImage, std::string normalAxis, std::string fnam, int iSlice, int bgnv, int endv){
  // currently only supports 8bit int (?)

  std::cout<<"Reading "<<fnam<<" \n";
  char axis = tolower(normalAxis[0]);
  int code = 0;

  int width=0, height=0, channels=0;
  // force 4 channel (rgba)
  unsigned char *data = stbi_load(fnam.c_str(), &width, &height, &channels, 4);
  ensure(data && width>0 && height>0, "Could not load file " + fnam, -1);

  if (vImage.nz()==0) {
    vImage.reset({width, height, 1}, 255);
  }

  auto nnn=vImage.size3();
  int exp_w=0, exp_h=0;
  switch (axis) {
    case 'x':  exp_w=nnn[2]; exp_h=nnn[1]; ensure(iSlice<nnn[0]," i_slice ?<"+_s(nnn[0]),2);  break;
    case 'y':  exp_w=nnn[0]; exp_h=nnn[2]; ensure(iSlice<nnn[1]," i_slice ?<"+_s(nnn[1]),2);  break;
    case 'z':  exp_w=nnn[0]; exp_h=nnn[1]; ensure(iSlice<nnn[2]," i_slice ?<"+_s(nnn[2]),2);  break;
  }

  double hFact=1., wFact=1;
  if (width != exp_w || height != exp_h) {
    // stbi_image_free(data);
    ensure(false, "Dimension mismatch: " + _s(width) + "x" + _s(height) + " vs " + _s(exp_w) + "x" + _s(exp_h) + ", using linear transformation");
    wFact = exp_w/double(width);
    hFact = exp_h/double(height);
  }

  double invZ = (double(endv)-bgnv)/255.0;
  for (int y=0 ; y<exp_h ; y++) {
    for (int x=0 ; x<exp_w ; x++) {
      size_t ind=0;
      switch (axis) {
        case 'x': ind=vImage.index(iSlice,y,x);  break;
        case 'y': ind=vImage.index(x,iSlice,y);  break;
        case 'z': ind=vImage.index(x,y,iSlice);  break;
      }

      size_t ix = std::min(int(x * wFact+0.5), width-1);
      size_t iy = std::min(int(y * hFact+0.5), height-1);
      unsigned char* rgba = data + 4*(iy*width+ix);
      unsigned int r=rgba[0], g=rgba[1], b=rgba[2], a=rgba[3];

      T vv;
      if constexpr (std::is_same_v<T, unsigned char>) {
        vv = (T)((std::max<unsigned int>(((r*77) + (g*150) +  (29*b))*a + (255-a)*256*256, 0) + 32768) >> 16);
      }
      else if constexpr (std::is_same_v<T, unsigned short>) {
        vv = (T)((std::max<unsigned int>(((r*77) + (g*150) +  (29*b))*a + (255-a)*256*256, 0) + 128) >> 8);
      }
      else {
        vv = (T)std::max<unsigned int>(((r*77) + (g*150) +  (29*b))*a + (255-a)*256*256, 0);
      }

      vImage(ind) = (T)(vv*invZ + bgnv);
    }
  }

  stbi_image_free(data);
  return code;
}


template <typename T>
static void fixData(const voxelImageT<T> &vImg, std::string& normalAxis,
                    std::string& fnam, int& iSlice, int& minv, int& maxv) {
  char axs = tolower(normalAxis[0]);
  normalAxis[0] = axs;

  int iDir=axs-'x'; iDir%=3;
  if (iSlice<-vImg.size3()[iDir]) iSlice=vImg.size3()[iDir]/2;

  if (maxv==-1000001)  { minv = accumulate(vImg,(std::min<T>));   maxv = accumulate(vImg,(std::max<T>));  }
  (std::cout<<" minmaxv: "<<minv<<" "<<maxv<<" ").flush();

  if (hasExt(fnam,".png"))  fnam = fnam.substr(0,fnam.size()-4);
  if (fnam[0]=='_') fnam=_s(axs)+_s(iSlice)+fnam+".png";
  else              fnam=fnam+"_"+_s(axs)+_s(iSlice)+".png";
  if (fnam.find('/')==std::string::npos) { mkdirs("fig"); fnam="fig/"+fnam; }
}


template<typename T>  bool sliceToPng(const voxelImageT<T> &vImg,
  std::string normalAxis, std::string fnam, int iSlice, int bgnv, int endv, std::string color="") {

  fixData(vImg, normalAxis, fnam, iSlice, bgnv, endv);

  if (color=="all") color="";
  if (color!="" && color!="RGB" && color!="gray") { std::cout<<"wrong color scheme, "<<color<<", expected 'gray' or 'RGB' or 'all'"<<std::endl;   color=""; }
  if (color=="" || color=="gray")
    slice2GrayPng(vImg, normalAxis[0], prepend("grey_",fnam), iSlice, T(bgnv), T(endv));  // grey_ is at begin for image slide-show order
  if (color=="" || color=="RGB")
    slice2RGBPng(vImg, normalAxis[0],                  fnam , iSlice, T(bgnv), T(endv));
  return 0;
}

template<typename T>  bool sliceMergeToPng(const voxelImageT<T> &vImg,
    std::string normalAxis, std::string fnam, int iSlice, int bgnv, int endv,
    const voxelImageT<T> &aImg, int mina, int maxa, float weight, std::string color="")
 {
  fixData(vImg, normalAxis, fnam, iSlice, bgnv, endv);
  if(maxa==-1000001)  { mina = accumulate(vImg,(std::min<T>));   maxa = accumulate(vImg,(std::max<T>));  }
  (std::cout<<"  minmaxa: "<<mina<<" "<<maxa<<"  w: "<<weight<<" ").flush();

  if (color=="all") color="";
  if (color!="" && color!="RGBA" && color!="RGBX") { std::cout<<"wrong color scheme, "<<color<<", expected 'RGBX' or 'RGBA' or 'all'"<<std::endl;   color=""; }
  if (color=="" || color=="RGBX")
    slice2RGBXPng(vImg, normalAxis[0], prepend("rgbx_",fnam), iSlice, T(bgnv), T(endv), aImg, T(bgnv), T(endv), weight);
  if (color=="" || color=="RGBA")
    slice2RGBAPng(vImg, normalAxis[0], prepend("rgba_",fnam), iSlice, T(bgnv), T(endv), aImg, T(bgnv), T(endv));
  return 0;
}


                namespace MCTProcessing _begins_


template<typename T>  bool sliceToPng( std::stringstream& ins, voxelImageT<T>& vImg)  {
  KeyHint("normalAxis(x/y/z) fileName iSlice  minv maxv  [al b e w]\n// if fileName starts with _, extra info are added as prefix otherwise as suffix to the filename");
  int minv(0); int maxv(-1000001); int iSlice(-1000000);
  std::string normalAxis("x");
  std::string fnam("aa.png"); std::string alphaImg;
  ins >> normalAxis;
  readOutFileNam(ins, fnam);
  ins >> iSlice >> minv >> maxv >> alphaImg;

  #ifdef _STOR_PUB
  if(len(alphaImg)) {
    int mina(0); int maxa(-1000001); float weight(0.9);
    ins  >> mina >> maxa >> weight;
    _dbgetOrReadImg(T,img2, alphaImg);
    sliceMergeToPng(vImg, normalAxis, fnam, iSlice, minv, maxv, img2, mina, maxa, weight, "");
  } //else
  #endif
  sliceToPng(vImg, normalAxis, fnam,  iSlice, minv, maxv, "");

  (std::cout<<".").flush();

  //slice2GrayPng(vImg,normalAxis[0],"grey_"+fnam+"_"+normalAxis+_s(iSlice)+".png", iSlice,T(minv),T(maxv));
  return true;
}


template<typename T>  bool sliceToPngBW( std::stringstream& ins, voxelImageT<T>& vImg) {
  if(ins.peek()=='?') { ins.str("normalAxis fileName iSlice  minv maxv"); return true;}
  int minv(0); int maxv(-1000001); int iSlice(-1000000);
  std::string normalAxis("x");
  std::string fnam("Img");
  ins >> normalAxis;
  readOutFileNam(ins, fnam);
  ins >> iSlice >> minv >> maxv;

  fixData(vImg, normalAxis, fnam, iSlice, minv, maxv);

  slice2GrayPng(vImg, normalAxis[0], prepend("grey_",fnam), iSlice,T(minv),T(maxv)); // grey_ is at begin for image slide-show order
  (std::cout<<".").flush();

  return true;
}


                _end_of_(namespace MCTProcessing)
