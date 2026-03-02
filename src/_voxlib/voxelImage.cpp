/*-------------------------------------------------------------------------*\

This file is part of libvoxel, a C++ template library for handelling 3D images.

Developed by:
 - Ali Q Raeini (2010-2022)

\*-------------------------------------------------------------------------*/

#include <memory>
#include <sstream>


#ifdef TIFLIB
#include "voxelTiff.h"
#endif
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "voxelPng_stbi.h"

#include "globals.h"  // ensure...
#include "voxelImage.h"
#include "voxelImageI.h"
#include "voxelEndian.h"


#ifdef _WBASM // work around CLang bugs complaining about undefined function
#include "vxlPro1.cpp"
#include "vxlPro.cpp"
#endif
#include "InputFile.h"
using namespace std; //cin cout endl string stringstream  istream istringstream regex*


int maxNz = 1000000, _maxNz = 1000000|12; // use `reset maxNz 100` to limit the number of layers processed during fine-tuning of image processing params


template<typename T>
void voxelImageT<T>::readFromHeader(const string& hdrNam, int procesKeys)  {
  //! read image from file header, format detected based on image extension

  if (hdrNam.empty() || hdrNam=="NO_READ")  return;

  //! read image from file header, format detected based on image extension
  auto& vImg=*this;

  string fnam;
  int3 nnn(0,0,0);

  string BinaryData="XXX", flipSigByt="False";
  bool X0read=false, dxread=false, autoUnit=true; //auto unit only applies to .mhd format
  double unit_=1.;
  int nSkipBytes(0);
  if (hasExt(hdrNam,".raw.gz") || hasExt(hdrNam,".raw") || hasExt(hdrNam,".dat") || hasExt(hdrNam,".txt"))  { // detect size and voxel size from image name.
      string
      data=replaceFromTo(replaceFromTo(replaceFromTo(replaceFromTo(replaceFromTo(replaceFromTo(
                    hdrNam,".gz$",""), ".raw$"," "), ".dat$"," "), ".txt$"," "),"__","\n"),"_"," ");

      data=regex_replace(data,regex(".*/"), "");
      data=regex_replace(data, regex(R"(\b[a-zA-Z_]\w*)"), "");

      data=replaceFromTo(replaceFromTo(replaceFromTo(data,"voxel",""),"size"," "),"um ","\n");
      data=regex_replace(data,regex("( [0-9][0-9]*)c"), " $1 $1 $1 ", regex_constants::format_first_only);
      data=regex_replace(data,regex("( [0-9][0-9]*)[ x]*([0-9][0-9]*)[ x]*([0-9][0-9]* )"),
                                              "\n   reset_NdX $1 $2 $3 ", regex_constants::format_first_only);
      data=regex_replace(data,regex("^[^\n]*\n"), "", regex_constants::format_first_only);
      // data=regex_replace(data,regex("\n|($)"),"\n   read "+hdrNam+"\n", regex_constants::format_first_only);
      for(auto&da:data)  { if(da=='p') da='.'; else if(da=='\n') break; }
      // vxlProcess(data,vImg,hdrNam); // removed from Python build for simplicity
      procesKeys=0;
      stringstream ss(data);
      string cmd;
      while(ss >> cmd) {
        if(cmd=="reset_NdX") {
          ss >> nnn;
          double dxv=1.0;
          if (ss >> dxv) { vImg.dxCh()=dbl3(dxv,dxv,dxv); }
        }
      }
      // TODO support units nm and mm ?
      fnam = hdrNam;
  }
  else if (hasExt(hdrNam,".mhd") || hasExt(hdrNam,".py")) {
    (cout<<" "<<hdrNam<<": ").flush();
    std::ifstream fil{hdrNam};  ensure(fil,"Cannot open header file, "+hdrNam,-1);
    while (true)  {
      std::streampos begLine = fil.tellg();
      string ky, tmp;   fil>>ky>>tmp;
      stringstream ss;  if(fil.peek()!='\n') fil.get (*(ss.rdbuf()));
      if (fil.fail()) break;
      if (ky=="ObjectType")  {  ss>> tmp;  if (tmp != "Image") cout<<"  Warning: ObjectType != Image :="<<tmp<<endl;  }
      else if (ky=="NDims")  {  ss>> tmp;  if (tmp != "3"    ) cout<<"  Warning: NDims != 3 :="<<tmp<<endl;  }
      else if (ky=="ElementType")  { ss>> tmp;  if ((tmp != "MET_UCHAR") && (sizeof(T)==1)) cout<<"  Warning: ElementType != MET_UCHAR :="<<tmp<<endl;   }
      else if (ky=="Offset")       { ss>> vImg.X0_;   cout<<"  X0: "<<vImg.X0_<<",  ";  X0read=true; }
      else if (ky=="ElementSize"
            || ky=="ElementSpacing")  { ss>> vImg.dx_;  cout<<"  dX: "<<vImg.dx_<<",  ";  dxread=true;  }
      else if (ky=="DimSize")         { ss>> nnn;  if (nnn.z>1.1*maxNz) { nnn.z=maxNz; } cout<<"  Nxyz: "<<nnn<<",  ";  }
      else if (ky=="ElementDataFile") { if (fnam.empty()) ss>> fnam;
                                         if (size_t is=hdrNam.find_last_of("\\/"); is<hdrNam.size() && fnam[0]!='/' && fnam[1]!=':')
                                             fnam=hdrNam.substr(0,is+1)+fnam;
                                         cout<<"  Img: "<<fnam<<",  "; }
      else if (ky=="BinaryData")  {  ss>> BinaryData;     cout<<"  BinaryData: "<<BinaryData<<"  "<<endl; }
      else if (ky=="Unit")        {  ss>> unit_;  autoUnit=false;   cout<<"  Unit, OneMeter: "<<unit_<<endl;   }
      else if (ky=="HeaderSize")  {  ss>> nSkipBytes;         cout<<"  Ski pHeaderSize: "<<nSkipBytes<<endl;  }
      else if (ky=="OutputFormat" || ky=="DefaultImageFormat" )  {  if(tmp=="=") ss>> tmp;  cout<<"  OutputFormat: "<<tmp<<", suffix:"<<imgExt(tmp)<<"  "<<endl; }///. sets suffix+format
      else if (ky=="BinaryDataByteOrderMSB" || ky=="ElementByteOrderMSB")  {  ss>> flipSigByt; }
      else if (ky!="CompressedData" &&  ky!="CompressedDataSize" &&  ky!="TransformMatrix" &&
           ky!="ElementNumberOfChannels" && ky!="CenterOfRotation" && ky!="AnatomicalOrientation" && ky!="AnatomicalOrientation")  {
        fil.clear();  fil.seekg(begLine);
        (cout<<"; ").flush();
        break;
      }
    }
    cout<<endl;
  }
  #ifdef TIFLIB
  else if (hasExt(hdrNam,".tif"))  {  readTif(vImg, hdrNam);  return;  }
  #endif
  else if (hasExt(hdrNam,".am"))  {
    fnam=hdrNam;
    procesKeys=0;
  }
  else if (hasExt(hdrNam,".raw.gz") || hasExt(hdrNam,".raw") || hasExt(hdrNam,".dat") || hasExt(hdrNam,".txt"))  { // detect size and voxel size from image name.
    string
    data=replaceFromTo(replaceFromTo(replaceFromTo(replaceFromTo(replaceFromTo(replaceFromTo(
                  hdrNam,".gz$",""), ".raw$"," "), ".dat$"," "), ".txt$"," "),"__","\n"),"_"," ");

    data=regex_replace(data,regex(".*/"), "");
    data=regex_replace(data, regex(R"(\b[a-zA-Z_]\w*)"), "");

    data=replaceFromTo(replaceFromTo(replaceFromTo(data,"voxel",""),"size"," "),"um ","\n");
    data=regex_replace(data,regex("( [0-9][0-9]*)c"), " $1 $1 $1 ", regex_constants::format_first_only);
    data=regex_replace(data,regex("( [0-9][0-9]*)[ x]*([0-9][0-9]*)[ x]*([0-9][0-9]* )"),
                                            "\n   reset Nd0 $1 $2 $3 ", regex_constants::format_first_only);
    data=regex_replace(data,regex("^[^\n]*\n"), "", regex_constants::format_first_only);
    data=regex_replace(data,regex("\n|($)"),"\n   read "+hdrNam+"\n", regex_constants::format_first_only);
    for(auto&da:data)  { if(da=='p') da='.'; else if(da=='\n') break; }
    // vxlProcess(data,vImg,hdrNam); // removed from Python build for simplicity
    procesKeys=0;
  }

  if(nnn.z) vImg.reset(nnn);
  int readingImage=0;
  if( !fnam.empty() && fnam!="NO_READ" && procesKeys!=2)  {
    if (hasExt(fnam,".tif")) {
      dbl3 dx=vImg.dx_, X0=vImg.X0_;
      readingImage = vImg.readBin(fnam);
      if(X0read) vImg.X0_=X0;
      if(dxread) vImg.dx_=dx;
    }
    else if ((hasExt(fnam,".raw") && BinaryData!="False") || BinaryData=="True")   {
      readingImage = vImg.readBin(fnam, nSkipBytes);
    }
    else if (hasExt(fnam,".am"))    {
      int RLECompressed;
      dbl3 dx=vImg.dx_, X0=vImg.X0_;
      getAmiraHeaderSize(fnam, nnn,vImg.dx_,vImg.X0_,nSkipBytes,RLECompressed);
      readingImage = vImg.readBin(fnam, nSkipBytes);
      if(X0read) vImg.X0_=X0;
      if(dxread) vImg.dx_=dx;
    }
    else if (hasExt(fnam,".raw.gz")) {
      readingImage = vImg.readBin(fnam);
    }
    else {
      ensure(hasExt(fnam, ".dat") || hasExt(fnam, ".txt"), "assuming image is in ascii format");
      readAscii(fnam, nSkipBytes);
    }
  }
  ensure(readingImage==0, "cannot read image "+fnam,-1);

  if(flipSigByt=="True") {
    cout<<"  flipEndian "<<endl;
    flipEndian(vImg);  }

  if(autoUnit  && vImg.dx_[0]>1.0000001)  { //&& dxread, doggy
    cout<<"\n\n\n  WARNING dx="<<vImg.dx_[0]<<"(>1 -> assuming unit is um),\n  please set Unit manually if needed ****\n\n\n";
    unit_ = 1e-6;
  }
  if(abs(unit_-1.)>epsT(float)) {
    vImg.dx_*=unit_;
    vImg.X0_*=unit_;
    cout<<"\n  unit= "<<unit_<<" => dx= "<<vImg.dx_<<", X0= "<<vImg.X0_<<endl;
  }
  // static const voxelplugins<T> plagins;
  // if (procesKeys) plagins.vxProcess(InputFile(fil,hdrNam),vImg);
  cout<<"."<<endl;
}

template<class T>
std::unique_ptr<voxelImageTBase> readToUnique(const string& hdrNam, int procesKeys) {
  // NB! this is also called from voxelImage read constructor, please avoid syclic dependency
  voxelImageT<T> vImg;
  vImg.readFromHeader(hdrNam, procesKeys);
  return make_unique<voxelImageT<T>>(std::move(vImg));
}


std::unique_ptr<voxelImageTBase> readImage(string hdrNam,  int procesKeys)  {
  //! read or create image
  using namespace std;
  (cout<<"voxelImage \""<<hdrNam<<"\": ").flush();

  if (hasExt(hdrNam, ".png")) { // grey-scale atm
    voxelImage VImage;
    sliceFromPng(VImage, "z", hdrNam, 0, 0,255);
    return make_unique<voxelImageT<unsigned char>>(std::move(VImage));
  }

  if (hasExt(hdrNam,".am")) {
    string vtype = getAmiraDataType(hdrNam);
    cout<<"reading '"<<vtype<<"'s from .am file"<<endl;

    #ifndef _VoxBasic8
    if (vtype=="int")       return readToUnique<int>(hdrNam,0);
#ifdef _ExtraVxlTypes
    if (vtype=="short")     return readToUnique<short>(hdrNam,0);
#endif
    if (vtype=="ushort")    return readToUnique<unsigned short>(hdrNam,0);
    #endif
    if (vtype=="byte")      return readToUnique<unsigned char>(hdrNam,0);

    alert("data type "+vtype+" not supported, when reading "+hdrNam, -1);
  }

  #ifdef TIFLIB
  if (hasExt(hdrNam,".tif"))  return readTifAnyT(hdrNam);
  #endif

  string typ;
  std::ifstream fil(hdrNam); // header file
  if(!fil)
  {
    ensure(hdrNam.size()<4 || hdrNam[hdrNam.size()-4]!='.', "can not open header file '"+hdrNam+"', pwd: "+getpwd(), -1);
    typ = hdrNam; hdrNam="NO_READ";
  }
  else if (hasExt(hdrNam,".mhd")) {
    while (true)  {
      string ky;  fil>>ky;
      stringstream ss;
      if(fil.peek()!='\n') fil.get (*(ss.rdbuf()));
      if (fil.fail()) {  cout<<"\n\n\nWarning: in read-image, 'ElementType =' not set in "<<hdrNam<<", assuming MET_UCHAR"<<endl; break; }
      if (ky == "ElementType")  {  ss >> typ >> typ;  break; }
    }
  }
  fil.close();

  if (typ=="MET_UCHAR")        return readToUnique<unsigned char >(hdrNam, procesKeys);
  #ifndef _VoxBasic8
  if (typ=="MET_USHORT")       return readToUnique<unsigned short>(hdrNam, procesKeys);
  if (typ=="MET_INT")          return readToUnique<int>           (hdrNam, procesKeys);
  if (typ=="MET_FLOAT")        return readToUnique<float>         (hdrNam, procesKeys);
  #endif
#ifdef _ExtraVxlTypes
  if (typ=="MET_CHAR")         return readToUnique<char>          (hdrNam, procesKeys);
  if (typ=="MET_SHORT")        return readToUnique<short>         (hdrNam, procesKeys);
  if (typ=="MET_UINT")         return readToUnique<unsigned int>  (hdrNam, procesKeys);
  if (typ=="MET_DOUBLE")       return readToUnique<double>        (hdrNam, procesKeys);
  if (typ=="MET_FLOAT_ARRAY")  return readToUnique<float3>        (hdrNam, procesKeys);
  if (typ=="MET_DOUBLE_ARRAY") return readToUnique<dbl3>          (hdrNam, procesKeys);
#endif //_ExtraVxlTypes
  return                              readToUnique<unsigned char> (hdrNam, procesKeys);

}
