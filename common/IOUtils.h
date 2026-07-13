#pragma once

#include <iostream>
#include <string>
#include <sstream>

inline bool _noArowtoStrip(std::string& nam)  {
  if (nam.size()>2 && nam[1]=='>' && (nam[0]=='-' || nam[0]=='>')) {
    for (size_t ii=2; ii<nam.size(); ++ii) if (!std::isspace(nam[ii])) {
      nam = nam.substr(ii); return false; } }
  return true;
}

inline std::string tryReadOutput(std::stringstream& ins)  {
  std::string nam;  auto tg = ins.tellg();  ins>>nam;
  if(nam=="->" || nam==">>") ins>>nam;
  else if (_noArowtoStrip(nam)) { ins.seekg(tg); }
  return nam;
}
inline void readResVar(std::stringstream& ins, std::string& nam, bool warn=true)  {
  ins>>nam;
  if(nam=="->" || nam==">>") ins>>nam;
  else if(_noArowtoStrip(nam) && warn) (std::cout<<" **Warning, missing:'->'\""<<nam<<"\"**  ").flush();  }

inline void readOutFileNam(std::stringstream& ins, std::string& nam, bool warn=true) {
  ins>>nam;
  if(nam==">>") ins>>nam;
  else if(_noArowtoStrip(nam) && warn) ( std::cout<<" **Warning, missing:'>>'\""<<nam<<"\"**  ").flush(); }

inline void startHtDetails(const std::string& summary) {
  // sync with html log formatter
  std::cout<<"\n\n<details><summary>    "+summary+"    </summary>"<<std::endl ;
}

inline void endHtDetails() {
  std::cout<<"\n</details>"<<std::endl ;
}

using ErC = bool; //!< Type of error code TODO: change to int
