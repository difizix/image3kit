#pragma once

// Input data file used by 3D image processing, network extraction,
// See the documentation of these codes for user guids and contact details,
// flow simulation  and other codes
// Developed by:
// - Ali Q. Raeini (2013-2021, 2025)


#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <vector>
#include <string>
#include "globals.h"


template<class T>  using  stvec  = std::vector<T>;
template<class T>  using  stvecs = stvec<stvec<T>>;
using                     ststr  = std::string;
using                     isstr  = std::istringstream;


inline int  readInt(isstr& in) {int n=0; in>>n; return n; } // to cast to char
inline bool readBoolOr(ststr st, std::istream& in) {
   in>>st; return st[0]=='T' || st[0]=='t' || st[0]=='Y' || st[0]=='y' || st[0]=='1'; }


//! Input data reader, using a similar format to JSON but less strict & more readable
class InputFile {
 public:
  using string = std::string;

  InputFile(bool multiline=false)
  :  multiline_(multiline)  {}


  InputFile(const string& fnam, bool multiline=false, bool init=true)
  :  multiline_(multiline)  {

    readFile(fnam);

    for(auto& kv:data_)
     if( (kv.first=="include" || kv.first=="append" ) && !kv.second.empty())  {
      kv.first = "included";
      readFile(kv.second);
    }

    if(init)  initIO(fnam);
  }

  InputFile(std::istream& in, const string& nam, bool multiline=false)
  :  multiline_(multiline)  { readStream(in, nam); }

  InputFile(const string& kwrds, const string& nam, bool multiline=false)
  :  multiline_(multiline)  {  std::istringstream ss(kwrds);  readStream(ss, nam);  }

  InputFile(const char* kwrds, const string& nam, bool multiline=false)
  :  InputFile(string(kwrds),nam,multiline)  {}

   InputFile(const InputFile& inp, const string& nam, int multiline=-1)
  :  data_(inp.data_), fileName_(inp.fileName_), folder_(inp.folder_), name_(nam),
    multiline_(multiline<0 ? inp.multiline_ : multiline)  {  set("OutputName", nam);  }

  bool safeGetline(std::istream& is, string& sr, bool noeq=false)  {
    /// \return readnext,  true: keep reading same key, if returned sr not empty
    sr.clear();
    auto begl = is.tellg();
    std::istream::sentry se(is, true);
    std::streambuf& sf = *is.rdbuf();
    for(;;) {
      int cr = sf.sbumpc(), cn=1;
      switch (cr) {
        case '\\': sr+=char(sf.sbumpc()); break; // read next
        #ifdef HASH_ENDS_LINE // backward compatibility with poreflow
        case '#': return false;
        case '/':  if(sf.sgetc()!='/') {  sr += '/';  break;  } [[fallthrough]];
        #else
        case '/':  if(sf.sgetc()!='/') {  sr += '/';  break;  } [[fallthrough]];
        case '#': [[fallthrough]];
        #endif
        case '%':
          while ((cr=sf.sbumpc())!='\n' && cr!=EOF);
          sr += '\t';
          return multiline_;

        case '=': case ':':
          if(noeq)  {  sr.clear(); is.seekg(begl);  return false;  }
          sr += '\t';         break;
        case ',':  sr += '\t';  break;  //! comma is treated as white space

        case EOF:  if(sr.empty())  is.setstate(std::ios::eofbit);  return false;

        case '{':
          if(sr.size() && !noeq) { sf.sungetc(); return true; } // try again with empty noeq=true (or empty sr to skip {})
          else if(noeq){ cr='\t';  do{ sr+=cr; cr=sf.sbumpc(); cn+=int(cr=='{')-(cr=='}'); }while(cn && cr!=EOF);  }
          [[fallthrough]];
        case '}':  sr += '\t';  break; //return  false;
        //case '{':  cr='\t'; do{ sr+=cr; cr=sf.sbumpc(); }while(cr!='}' && cr!=EOF); cr='\t';  break;
        case '\'': cr='\t'; do{ sr+=cr; cr=sf.sbumpc(); }while( cr!='\''&& cr!=EOF );     break;
        case '"':  cr='\t'; do{ sr+=cr; cr=sf.sbumpc(); }while( cr!='"' && cr!=EOF );     break;
        case '[':  cr='\t'; do{ sr+=cr; cr=sf.sbumpc(); cn+=int(cr=='[')-(cr==']'); }while(cn && cr!=EOF);    break;
        case '(':  cr='\t'; do{ sr+=cr; cr=sf.sbumpc(); cn+=int(cr=='(')-(cr==')'); }while(cn && cr!=EOF);    break;
        case '<':  cr='\t'; do{         cr=sf.sbumpc(); cn+=int(cr=='<')-(cr=='>'); }while(cn && cr!=EOF);    break; // primitive: filter out  xml tags

        case '\r':
          if(sf.sgetc()=='\n') { sf.sbumpc(); } [[fallthrough]];
        case '\n':
          cr=sf.sgetc();
          return (cr=='\n' || cr=='\r') ? false  : multiline_; //! double new lines are always treated as end of keyword

        case ';':  return  false;
        default :  sr += char(cr);
      }
    }
  }


  int readFile(const string& fnam, int seize=2)  {
    if(debugLevel) (std::cout<< " Reading "+fnam+";  ").flush();
    std::ifstream in(fnam);
    if (in)  return readStream(in, fnam);
    ensure(seize==0 || fnam=="NO_READ" || fnam=="-c", "can not open '"+fnam+"'", seize);
    return 1;
  }

  int readStream(std::istream& in, string fnam="")  {

    if(fnam.size()) fileName_ = fnam;

    string prev("NO_KEYWORD_READ");
    while(in.good())  {
      string key, kydata, bufr;
      bool readnext=safeGetline(in,bufr,false);

      if(bufr.empty())                           continue;

      {
        size_t bgn=bufr.find_first_not_of(" \t");
        if (bgn == string::npos)                 continue;

        size_t lst= bufr.find_last_not_of(" \t")+1;

        size_t endKy=
         #ifndef ALLOW_SPACE_IN_KEY // allowing space in key is a source of user error, deactivated by default
          std::min(bufr.find_first_of(":= \t", bgn+1), lst);
         #else
          bufr.find_first_of(":=", bgn+1);  if(endKy==string::npos) endKy = std::min(bufr.find_first_of(" \t",bgn+1), lst);
         #endif

        key = bufr.substr(bgn, endKy-bgn);
        if(key=="end")        break;
        ensure(key.size()>0 && key.size()<100, " after '"+prev+"'\n'"+bufr+"'\n@["+_s(endKy)+" "+_s(bgn)+"]",-1);

        bgn=bufr.find_first_not_of(" \t",endKy);
        if (bgn != string::npos) kydata = bufr.substr(bgn, lst-bgn);
      }
      while(readnext)  {
        readnext=safeGetline(in,bufr, true);
        size_t bgn=bufr.find_first_not_of(" \t");
        if (bgn==string::npos)                   continue;
        size_t lst=bufr.find_last_not_of(" \t");
        if(kydata.size())  {
          if(kydata[0]!='\n') kydata = "\n"+kydata;
          kydata += "\n";
        }
        kydata += bufr.substr(bgn, lst-bgn+1) ;
      }

      prev = key;

      #ifdef _debugCompile_
      if (key=="debugLevel")  debuglevel_(atoi(kydata.c_str()));
      #endif

      data_.push_back({key, kydata});
    }
    return 0;
  }


  void initIO(string fnam="")   {  //! call this after setting name and/or prefix

    string prf=getOr("prefix", string());
    if(prf.size())  {
      folder_.resize(0);
      size_t slashloc=prf.find_last_of("\\/");
      if (slashloc<prf.size()-1)  {
        folder_=prf.substr(0,slashloc+1);
        std::cout<<"Creating folder: "<<folder_<<", "
          << _TRY_(mkdirs(folder_))
          <<std::endl;
        prf=prf.substr(slashloc+1);
      }

      if (prf.size()>1 && (prf.back()=='/' || prf.back()=='\\'))  {
        folder_+=prf;
        std::cout<<"Creating folder: "<<folder_<<"  "
          << _TRY_(mkdirs(folder_))
          <<std::endl;
        prf="";
      }
    }

    if (lookup("OutputName",name_))  name_ = prf+name_;
    else if(prf.size()) name_ = prf;
    else {
      prf = getOr("NetworkFile", string(""));
      if (prf.empty()) { prf = fnam; }
      if (prf.empty()) { if(!lookup("ElementDataFile",prf)) prf = fileName(); }
      if (prf.size()>7 && prf.substr(prf.size()-3,3)==".gz") prf = prf.substr(0,prf.size()-3); // sync bname
      if (prf.size()>7 && (prf.substr(prf.size()-7)=="_ms.xmf" || prf.substr(prf.size()-7)=="_pn.xmf"))
        prf = prf.substr(0,prf.size()-7);
      else { size_t dl=prf.find_last_of(".");   if (dl<prf.size()) prf.erase(dl); }
      size_t sl=prf.find_last_of("\\/"); if (sl<prf.size()) prf=prf.substr(sl+1);
      name_ = prf;
    }
  }


  #ifdef _debugCompile_
   #define _debugInfo_(_prefx) if(debugLevel) std::cout<<_prefx+key+":"+kv.second<<std::endl
  #else
   #define _debugInfo_(_prefx)
  #endif


  int echoKeywords(std::ostream& out=std::cout) const  {
    char el = ';';
    if(out.tellp()==0)  { el='\n';  out<<"{/""/ -*- C -*- "<<outputName()<<" input keywords:\n\n"; }
    for(auto& kv:data_) {
      out <<" "<< kv.first  << ":\t";
      if(kv.second.find_first_of("[:;(\"\'{")!=string::npos)  out<<"{ "<<kv.second<<" }"<<el<<"\n";
      else out<< kv.second <<el<<"\n";
    }
    if(el=='\n')  out<<"}"<<std::endl;
    return 0;
  }

  void add(const string& key, const string& val)  { data_.emplace_back(key,val); }
  void add(const InputFile& inp)  { for(const auto& kw:inp.data())  data_.push_back(kw); }

  void set(string key, string val, bool overwrite=true)  {
    for(auto& kv:data_)  if(kv.first == key)  {
      if(overwrite) { _debugInfo_("Resetting "); kv.second = val; } return; }
    add(key,val);
  }
  void setDefault(const string& key, const string& val)  { set(key,val,false); }

  string& operator[](string key)  { /// not traceable, use set/add/get/giv instead
    for(auto& kv:data_)  if(kv.first == key)  { _debugInfo_("[] "); return kv.second; }
    add(key,"");
    return data_.back().second;
  }

  void renameKeys(const string& key, string newkey)  {
    for(auto& kv:data_)  if(kv.first == key)  { _debugInfo_("Renaming "); kv.first.swap(newkey); }
  }


  // Get functions:

  const string& kwrd(const string& key, int seize=0) const  { //! get
    for(auto& kv:data_) if(kv.first == key) { _debugInfo_("Reading ");     return (kv.second);  }
    Assert(seize<1, key, "missing keyword", seize>1);
    return empty_; // empty string
  }

  bool giv(const string& key, isstr& iss, int seize=0) const  { //! give me
    iss.clear();
    for(auto& kv:data_)  if(kv.first == key) { _debugInfo_("Reading ");  iss.str(kv.second);  return true; }
    Assert(seize<1, key, "missing keyword", seize>1);
    return false;
  }
  template<class T> bool giv(const string& key, T& var, int seize=0) const  {//! give me
    isstr iss;
    if(giv(key, iss, seize)) { iss>>var; return true; }
    else  return false;
  }
  bool giv(const string& key, bool& var, int seize=0) const {
    isstr iss;
    if(giv(key, iss, seize)) { char c; iss>>c;  var=(c=='T'||c=='t'||c=='Y'||c=='y'||c=='1'); return true; }
    else  return false;
  }
  template<class T> bool lookup(const string& key, T& var) const { return giv(key,var);  } /// openfoam-ish!
  template<class T> T    getOr(const string& key, T var)  const {  giv(key, var);  return var;  }


  void Assert(bool isOK, const string& key, const string message="", bool severe=true) const  {
    if(!isOK)  { std::cerr<<"\n\n"<<(severe?"Error":"Warning")<<" in file "+fileName()+", keyword:\n  "+key+": "+kwrd(key,0)+";\n  "+message+"\n"<<std::endl;
      if (severe) throw std::runtime_error("keyword error"); }
  }
  void checkEndOfData(isstr& iss, const string& key, const string message="", bool severe=true) const  {
    Assert(!iss.fail(), key,"Incomplete/wrong data, "+message, severe);
    char c; Assert(!iss.get(c), key,"Too much data, "+message, severe);
  }

  string        outputName() const { return folder_+name_; }
  const string& folder()     const { return folder_; }
  const string& name()       const { return name_; }
  const string& fileName()   const { return fileName_; } // optional

  const stvec<std::pair<string,string>>&  data() const { return data_; };
  stvec<std::pair<string,string>>&        dataRef()    { return data_; };

private:

  stvec<std::pair<string,string>>   data_;      // TODO use std::flat_map?
  string                            fileName_;  //!< input file name, optional
  string                            folder_;
  string                            name_;
  string const                      empty_;
  bool                              multiline_; //!< if true, need ';' or "\n\n" for end of keyword data
};
