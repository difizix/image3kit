/*-------------------------------------------------------------------------*\

This file is part of libvoxel, a C++ template library for handelling 3D images.

Developed by:
 - Ali Q Raeini (2010-2022)

\*-------------------------------------------------------------------------*/

#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <regex>
#include "globals.h"
#include "typses.h"

namespace VoxLib {
using namespace std;


//! suffix,  set/get default suffix, uses static storage.
const std::string& imgExt(const std::string& defSuffix="") {
  #ifdef ZLIB
       static std::string defSuffix_=".raw.gz";
  #else
    #ifdef TIFLIB
     static std::string defSuffix_=".tif";
    #else
       static std::string defSuffix_=".raw";
    #endif //TIFLIB
  #endif //ZLIB

  ///. set via OutputFormat keyword
  if (defSuffix.size()) {
    if(defSuffix[0]!='.') defSuffix_="."+defSuffix;
    else                  defSuffix_=defSuffix;
    if( defSuffix_!=".tif" && defSuffix_!=".raw.gz" && defSuffix_!=".am" &&
        defSuffix_!=".raw" && defSuffix_!=".dat" && defSuffix_!=".txt")
      std::cout<<"\nError: wrong default image format: "<<defSuffix_<<"\n"<<std::endl;
  }
  return defSuffix_;
}

void fileNameToImgInfo(const string& fname, int3& nnn, dbl3& dx) {
      string
      data=replaceFromTo(replaceFromTo(replaceFromTo(replaceFromTo(replaceFromTo(replaceFromTo(
                    fname,".gz$",""), ".raw$"," "), ".dat$"," "), ".txt$"," "),"__","\n"),"_"," ");

      data = regex_replace(data, regex("^([0-9A-Za-z]*)"), ""); // why this was needed?

      data=regex_replace(data,regex(".*/"), "");
      data=regex_replace(data, regex("pt"), "p"); // voxel-spacing
      data=regex_replace(data, regex(R"(\b[a-zA-Z_]\w*)"), "");

      data=replaceFromTo(replaceFromTo(replaceFromTo(data,"voxel",""),"size"," "),"um ","\n");
      data=regex_replace(data,regex("( [0-9][0-9]*)c"), " $1 $1 $1 ", regex_constants::format_first_only);
      data=regex_replace(data,regex("( [0-9][0-9]*)[ x]*([0-9][0-9]*)[ x]*([0-9][0-9]* )"),
                                              "\n   reset_NdX $1 $2 $3 ", regex_constants::format_first_only);
      data=regex_replace(data,regex("^[^\n]*\n"), "", regex_constants::format_first_only);
      // data=regex_replace(data,regex("\n|($)"),"\n   read "+fname+"\n", regex_constants::format_first_only);
      for(auto&da:data)  { if(da=='p') da='.'; else if(da=='\n') break; }  // voxel-spacing

      stringstream ss(data);
      string cmd;
      while(ss >> cmd) {
        if(cmd=="reset_NdX") {
          ss >> nnn;
          double dxv=1.0;
          if (ss >> dxv) { dx = dbl3(dxv,dxv,dxv); }
        }
      }
}

string fileNameToElemType(const string& fname) {
    if (hasExt(fname, ".mhd")) {
        ifstream fil(fname);
        if (fil) {
            string ky, typ;
            while (fil >> ky) {
                stringstream ss;
                if (fil.peek() != '\n') fil.get(*(ss.rdbuf()));
                if (fil.fail()) {
                    cout << "\n\n\nWarning: in read-image, 'ElementType =' not set in " << fname << ", assuming MET_UCHAR" << endl;
                    break;
                }
                if (ky == "ElementType") {
                   ss >> typ >> typ;
                   return typ;
                }
            }
        }
    }
    string data = replaceFromTo(fname, R"(\.)", "_");
    if (data.find("8bit") != string::npos)   return "MET_UCHAR";
    if (data.find("16bit") != string::npos)  return "MET_USHORT";
    if (data.find("32bit") != string::npos)  return "MET_INT";
    if (data.find("uchar") != string::npos)  return "MET_UCHAR";
    if (data.find("ushort") != string::npos) return "MET_USHORT";
    if (data.find("float") != string::npos)  return "MET_FLOAT";
    if (data.find("_int_") != string::npos)  return "MET_INT";
    if (hasExt(fname, ".raw")) { // detect from file size
        std::smatch nxyzMatch;
        if (std::regex_search(data, nxyzMatch, std::regex("([0-9]+)[_x]([0-9]+)[_x]([0-9]+)")) && nxyzMatch.size() >= 4) {
            std::stringstream nxyzss(nxyzMatch[1].str() + " " + nxyzMatch[2].str() + " " + nxyzMatch[3].str());
            int3 n(0, 0, 0); nxyzss >> n;
            size_t denom = (size_t)n.x * n.y * n.z;
            if (denom > 0) {
                std::ifstream fil(fname, std::ios::binary);
                if (fil) {
                    fil.seekg(0, std::ios::end);
                    size_t file_size = fil.tellg();
                    size_t elmSize = file_size / denom;
                    if      (elmSize <= 1) return "MET_UCHAR";
                    else if (elmSize == 2) return "MET_USHORT";
                    else if (elmSize == 4) return "MET_INT";
                }
            }
        }
    }
    if (data.find("lems_raw") != string::npos) return "MET_INT"; // _VElems.raw and Slems.raw(.gz) default to int

    std::cout << "Warning, could not detect image data type from '" << fname << "', assuming MET_UCHAR" << endl;
    return "MET_UCHAR";
}


string getAmiraDataType(const string& fnam)  {
  std::ifstream hdr(fnam);
  ensure(hdr, "could not open "+fnam, -1);
  std::string tmp;
  while (true)  {
    hdr>>tmp;
    std::stringstream ss;
    if(hdr.peek()!='\n') hdr.get (*(ss.rdbuf()));
    if (hdr.fail()) {std::cout<<"Error reading "<<fnam<<",  after "<<tmp<<std::endl; break;}

    if (tmp=="Content")  { ss >> tmp >> tmp;  break;  }
    else if (tmp=="@1")    break;
  }
  if(tmp.size() && tmp.back()==',') tmp.resize(tmp.size()-1);
  return tmp;
}

void getAmiraHeaderSize(const string& fnam, int3& nnn, dbl3& dx_, dbl3& X0_, int& nSkipBytes, int& RLE)  {
  std::ifstream hdr(fnam);
  while (true)  {
    std::string tmpStr;    hdr>>tmpStr;

    std::stringstream ss;
    if(hdr.peek()!='\n') hdr.get (*(ss.rdbuf()));
    if (hdr.fail()) {std::cout<<"Error reading "<<fnam<<",  after "<<tmpStr<<std::endl; break;}
    std::string tmp;
    if (tmpStr == "define")  {
      ss >> tmp  >>nnn;
      if (tmp != "Lattice") std::cout<<" Warning: define != Lattice n3, read: "<<tmp<<std::endl;
    }
    else if (tmpStr == "BoundingBox")  {
      ss >> X0_.x >> dx_.x >> X0_.y >> dx_.y >> X0_.z >> dx_.z;
      int3 nn=nnn;
      for (int i:{0,1,2})  nn[i] = std::max(nn[i]-1, 1); // Why nnn-1? either Avizo did not properly convert voxel size to bounding box, or its users made a mistake!
      dx_=(dx_-X0_)/nn;
    }
    else if (tmpStr=="@1")     break;
    else if(tmpStr=="Lattice")  {
      while (tmpStr[0]!='@' && ss)    ss>>tmpStr;
      RLE = tmpStr.size()>11 && tmpStr.compare(3,9,"HxByteRLE") == 0;
    }
  }
  nSkipBytes = hdr.tellg(); ++nSkipBytes; //++ is for '\n' after "@1"
}

void getMhdHeaderData(const string& hdrNam, string& fnam, int3& nnn, dbl3& dx_, dbl3& X0_,
                      int& nSkipBytes, string& BinaryData, string& flipSigByt,
                      double& unit_, bool& X0read, bool& dxread, bool& autoUnit,
                      int maxNz, size_t elemSize) {
    (cout<<" "<<hdrNam<<": ").flush();
    ifstream fil{hdrNam};  ensure(fil,"Cannot open header file, "+hdrNam,-1);
    while (true)  {
      streampos begLine = fil.tellg();
      string ky, tmp;   fil>>ky>>tmp;
      stringstream ss;  if(fil.peek()!='\n') fil.get (*(ss.rdbuf()));
      if (fil.fail()) break;
      if (ky=="ObjectType")  {  ss>> tmp;  if (tmp != "Image") cout<<"  Warning: ObjectType != Image :="<<tmp<<endl;  }
      else if (ky=="NDims")  {  ss>> tmp;  if (tmp != "3"    ) cout<<"  Warning: NDims != 3 :="<<tmp<<endl;  }
      else if (ky=="ElementType")  { ss>> tmp;  if ((tmp != "MET_UCHAR") && (elemSize==1)) cout<<"  Warning: ElementType != MET_UCHAR :="<<tmp<<endl;   }
      else if (ky=="Offset")       { ss>> X0_;   cout<<"  X0: "<<X0_<<",  ";  X0read=true; }
      else if (ky=="ElementSize"
            || ky=="ElementSpacing")  { ss>> dx_;  cout<<"  dX: "<<dx_<<",  ";  dxread=true;  }
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

array<double,5> otsu_minAvThresholdAvMx(const piece<double>& hist, int  ibgn, int  iend, double shift, double scale) {
  /* binarization by Otsu's method based on maximization of inter-class variance */
  double sumw(0), sumiw(0);
  for (int i = ibgn; i <= iend; ++i) { sumw += hist[i];  sumiw += hist[i]*i; }

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

  double level = shift + ilevel*scale;
  double minv = shift + ibgn*scale;
  double maxv = shift + iend*scale;

  double  avgs[2] = {sunL/wL, (sumB-sunL)/(wB-wL)};
  cout<<"   Otsu: ** "<< level<<" **  N: "<< sumw<<",  cdf_"<<level<<": "<< std::setprecision(3)<<wL<<"  cdf_"<<maxv<<": "<< std::setprecision(3)<<wB;
  cout<<"   segment 2   "<<minv<<"  "<< int(avgs[0]) <<"  "<<level<<"  "<<int(avgs[1])<<"  "<<maxv<<endl;


  array<double,5> levels{{0.}};
  levels[0]=minv;   levels[1]=avgs[0];   levels[2]=level;   levels[3]=avgs[1];   levels[4]=maxv;
  return  levels;
}

} // namespace VoxLib
