#pragma once
/// \file miscellaneous utilities used everywhere...
// #include at the very top of .cpp files to avoid macro/pragma clash!


#include <iostream>
#include <sstream>
#include <stdexcept>


/// \section filesystem utilities

#include <filesystem>
namespace stdfs = std::filesystem;
inline std::string getpwd() { return stdfs::current_path().string(); }
inline int mkdirs(const std::string& dir) { std::error_code ec; stdfs::create_directories(dir, ec);  return ec.value(); }
inline int chdir( const std::string& dir) { std::error_code ec;  stdfs::current_path(dir,ec);  return ec.value(); }
#define  _TRY_(_syscmnd_) std::string((_syscmnd_==0) ?  " :/ " : " failed ")


/// \section Lazy Hacks

/// Generate SiRun and VxlPro help messages
#define KeyHint(_args_desc_)  if(ins.peek()=='?') { ins.str(_args_desc_); return 0; }
#define sirInfo(...) (std::cout << __VA_ARGS__)
#ifndef RELEASE_DATE
  #define RELEASE_DATE  __DATE__
#endif

//! Non-folding brackets for namespace '{' and '}', to be used in early stages of code development
#define _begins_       {
#define _end_of_(sec)  }


/// \section String utilities

template<typename T> std::string _s(const T& n){  std::ostringstream ss;  ss<<n;  return ss.str();  } //< using _s = std::to_string  is bad in decimal notation
template<typename T> inline bool hasExt(const T& path, const std::string_view& ext) { return path.size()>ext.size() && path.compare(path.size()-ext.size(), ext. size(),ext) == 0; }

//stringify macro args after (sub)macro expansion
#ifndef TOSTRING
#define STRINGIFY(xpandd) #xpandd
#define TOSTRING(x_macro) STRINGIFY(x_macro)
#endif

#define __FILEBASENAME__ std::string(strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__, strcspn (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__,"."))


/// \section Testing and error handling: ensure, alert runtime checks

// Run-time check and testing macros
inline bool _cerr_(std::string msg="", int xit=0) {
  // for debugger breakpoints: don't optimize out please !!!
  if(xit)
    throw std::runtime_error("\n\n"+msg+"\n");
  else
    std::cerr<< "\n"+msg <<std::endl;
  return true;
}
#ifdef DEVEL
  #define ERR_HDR(xit)  std::string(__FILE__)+":"+std::to_string(__LINE__)+" in "+ std::string(__FUNCTION__)+", "  \
                       +std::string(xit?" Error: ":" Warning: ")
#else
  #define ERR_HDR(xit)  std::string(xit?" Error: ":" Warning: ")
#endif
#define ensure1(isOk          ) (!((isOk)|| _cerr_(ERR_HDR(0  )+std::string(TOSTRING(isOk)))))
#define ensure2(isOk, msg     ) (!((isOk)|| _cerr_(ERR_HDR(0  )+"\""+msg+"\""     )))
#define enforce(isOk, msg     ) (!((isOk)|| _cerr_(ERR_HDR(1  )+"\""+msg+"\"",  1 )))
#define ensure3(isOk, msg, xit) (!((isOk)|| _cerr_(ERR_HDR(xit)+"\""+msg+"\"", xit)))
#define GET_MACRO3(_1,_2,_3,NAME,...) NAME

//! Validation/production phase ensure/assert. Usage:
//!   \code{.cpp} ensure(condition, "message", int throw_on_error=false); \endcode
#define EXPAND(x) x
#define ensure(...) EXPAND((void)GET_MACRO3(__VA_ARGS__, ensure3, ensure2, ensure1, "Only 1 to 3 args please")(__VA_ARGS__))
#define check(...) EXPAND(GET_MACRO3(__VA_ARGS__, ensure3, ensure2, ensure1, "Only 1 to 3 args please")(__VA_ARGS__))
#define alert(...)  EXPAND((void)GET_MACRO3(false,__VA_ARGS__, ensure3, ensure2, "Only 1 to 2 args please")(false,__VA_ARGS__))


/// \section debugging/fine-tuning

#ifdef _debugCompile_ // by default do not use the obsolete debugLevel/dAsrt
  template<class T> int debuglevel_(T level) {  static int l_=1;  if (level>=0) { l_=level; }  return l_;  }
  #define debugLevel    debuglevel_(-1)
  #define dAsrt(...) (debugLevel<=0 || ensure(__VA_ARGS__))  //! debug assert with message
#else
  #define debugLevel  0
  #define dAsrt(...)
#endif // _debugCompile_
// Note use addr2line for more debugging...


/// \section Hackish initialization of global variables in header files,
/// TODO use static inline vars instead?
/// Usage: `#define _InitGlobals` in one .cpp file before `#include "globals.h"`
/// and then `#include "other.h"` files (order matters), where globals  are defined as:
/// `_Extrn thread_local int III _SetTo( 1+1+1 );`
#ifdef _InitGlobals
  #define _Extrn
  #define _SetTo(...)  = __VA_ARGS__
#else
  #define _Extrn extern
  #define _SetTo(...)
#endif
