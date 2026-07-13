#pragma once
//! \file SiRun.h declares data structures, utility functions and SiR
//! class  for holding all functions invokable through workflow.cc file
//  By:
//     Ali Qaseminejad Raeini (2017-2020)
//......................................................................

#include <unordered_map>
#include <functional>
#include <memory>

#include "IOUtils.h"

class InputFile;

using voidPtr  = std::unique_ptr<void, std::function<void(void *)>>;
using store_t  = std::unordered_map<std::string,voidPtr>; //!< database class
using storeT   = std::unordered_map<std::string,voidPtr>; //!< database class

#ifdef _STOR_PUB
#include "globals.h"
_Extrn  storeT  _STOR; // DEV:;  thread_local
#endif

//! use this function to retreive data from #storeT
inline void* dbget(storeT& sr, const std::string& ky, int importance=2) {
  auto pr=sr.find(ky);
  if(pr!=sr.end()) return pr->second.get();
  if(ky!="skip" && importance) std::cout<<"\n  *** Error "+ky+" not in store ***"<<std::endl;
  if(importance>1) throw std::invalid_argument( ky+" not in store" );
  else             return nullptr; }

template<typename T>  void deleterVerbose(void const* p) {
  if(p) { delete static_cast<T const*>(p);  std::cout<<"  deleted "<<typeid(T).name()<<" "<<std::endl; } }

//! use this function to save data in #storeT, use stor.erase(key) to remove
template<typename T>  void dbset(storeT& sr, const std::string& ky, T* pr)  {
  if(dbget(sr,ky,0)) std::cout<<"\n\nError: "+ky+" is already in store, overwriting...\n\n"<<std::endl;
  else sr.emplace(std::make_pair(ky, voidPtr(pr, &deleterVerbose<T>)));
  (std::cout<<std::string("  stored ")+typeid(T).name()+" as "+ky+" ").flush();
}

template<typename T>  void deleterSilent (void const* p) { if(p)  delete static_cast<T const*>(p); }

//! Used internally to share data via #storeT
template<typename T>  void rlset(  storeT& sr, const std::string& ky, T* pr)  {
  sr.emplace(std::make_pair(ky, voidPtr(pr, &deleterSilent<T>))); }

inline void leakerSilent  (void const* p) {}
//! Used internally to share data via #storeT
template<typename T>  void leakset(storeT& sr, const std::string& ky, T* pr)  {
  sr.emplace(std::make_pair(ky, voidPtr(pr, &leakerSilent))); }

                namespace SiM {
using ErC = bool; //!< Type of error code TODO: change to int

  using std::stringstream;
  using std::string;

  ErC run       ( std::stringstream& ins, storeT& stor);
  ErC help      ( std::stringstream& ins, storeT& stor);
  ErC runPar    ( std::stringstream& ins, storeT& stor);
  ErC runFor    ( std::stringstream& ins, storeT& stor);
  ErC runForX   ( std::stringstream& ins, storeT& stor);
  ErC forPar    ( std::stringstream& ins, storeT& stor);
  ErC runDetach ( std::stringstream& ins, storeT& stor);
  ErC echo      ( std::stringstream& ins, storeT& stor);
  ErC ignore    ( std::stringstream& ins, storeT& stor);
   ErC mkDir     ( std::stringstream& ins, storeT& stor);
  ErC _cd_      ( std::stringstream& ins, storeT& stor);
  ErC _sh_      ( std::stringstream& ins, storeT& stor);
  #ifdef _STOR_PUB
  ErC expose    ( std::stringstream& ins, storeT& stor);
  #endif
  ErC erase     ( std::stringstream& ins, storeT& stor);
  ErC clear     ( std::stringstream& ins, storeT& stor);

}//                _end_of_(namespace SiM)


class  SiR {
public:
  SiR();

  std::string help(std::string key, std::string subkey) const;

  void process(std::istream& ins, std::string dictNam="") const;

  void process(const InputFile& inp, std::string act="executing") const;

public:
  typedef ErC(*ProcessPP)(std::stringstream&, storeT&);

  std::unordered_map<std::string,ProcessPP> key_funs;
  std::string                               rootDir_;
  mutable storeT                            stor_;
};


int usage_SiR(const char* app_name);

int MainT(int argc, char *argv[], const SiR* cmacros, int (*usage_sir)(const char*) = usage_SiR);
