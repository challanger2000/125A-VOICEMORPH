#pragma once
// UI-thread-only physical units <-> normalized values for VST3 text editors.
// UI format and parsing must be symmetric. No DSP or VST3 SDK dependencies.
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <string>
namespace vm {
inline bool isVoiceControl(int id) noexcept {return id>=100&&id<=118;}
inline bool formatVoiceControl(int id,double normalized,char* buf,std::size_t capacity) noexcept {
 if(!buf||capacity==0||!isVoiceControl(id)||!std::isfinite(normalized))return false;
 const double n=std::clamp(normalized,0.,1.);
 int result=0;
 if(id==100||id==101)result=std::snprintf(buf,capacity,"%.1f dB",(n-.5)*24.);
 else if((id>=102&&id<=107)||id==110||id==112)
  result=std::snprintf(buf,capacity,"%.0f %%",n*100.);
 else if(id==111||id==113)
  result=std::snprintf(buf,capacity,"%.0f %%",n*400.);
 else if(id==108)
  result=n<=0.?std::snprintf(buf,capacity,"OFF"):
    std::snprintf(buf,capacity,"%.0f Hz",35.+145.*n);
 else if(id==109)result=std::snprintf(buf,capacity,"%s",n>=.5?"ON":"OFF");
 else if(id==114)result=std::snprintf(buf,capacity,"%s",n>=.5?"POST":"PRE");
 else if(id==115||id==116)result=std::snprintf(buf,capacity,"%.0f ms",n*2000.);
 else if(id==117||id==118)
  result=std::snprintf(buf,capacity,"%+.1f st",id==117?n*18.-12.:n*14.-8.);
 return result>=0&&static_cast<std::size_t>(result)<capacity;
}
inline std::string trimLowerAscii(std::string s){
 auto trim=[](unsigned char c){return std::isspace(c)!=0;};
 while(!s.empty()&&trim(static_cast<unsigned char>(s.front())))s.erase(s.begin());
 while(!s.empty()&&trim(static_cast<unsigned char>(s.back())))s.pop_back();
 for(char& c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
 return s;
}
inline bool parseVoiceControl(int id,const std::string& input,double& normalized) {
 if(!isVoiceControl(id)||input.size()>96)return false;
 std::string s=trimLowerAscii(input);
 if(s.empty())return false;
 if(id==108&&s=="off"){normalized=0.;return true;}
 if(id==109){
  if(s=="on"||s=="1"){normalized=1.;return true;}
  if(s=="off"||s=="0"){normalized=0.;return true;}
  return false;
 }
 if(id==114){
  if(s=="post"||s=="1"){normalized=1.;return true;}
  if(s=="pre"||s=="0"){normalized=0.;return true;}
  return false;
 }
 // Accept German decimal commas, but disallow ambiguous mixed separators.
 if(s.find(',')!=std::string::npos){
  if(s.find('.')!=std::string::npos)return false;
  std::replace(s.begin(),s.end(),',','.');
 }
 errno=0;
 const char* first=s.c_str();
 char* end=nullptr;
 const double physical=std::strtod(first,&end);
 if(end==first||errno==ERANGE||!std::isfinite(physical))return false;
 const std::string unit=trimLowerAscii(std::string(end));
 auto accepts=[&](const char* expected){return unit.empty()||unit==expected;};
 double n=0.;
 if(id==100||id==101){
  if(!accepts("db")||physical< -12.||physical>12.)return false;
  n=physical/24.+.5;
 }else if((id>=102&&id<=107)||id==110||id==112){
  if(!accepts("%")||physical<0.||physical>100.)return false;
  n=physical/100.;
 }else if(id==111||id==113){
  if(!accepts("%")||physical<0.||physical>400.)return false;
  n=physical/400.;
 }else if(id==108){
  if(!accepts("hz")||physical<35.||physical>180.)return false;
  n=(physical-35.)/145.;
  // Entering the minimum enabled frequency should turn LOW CUT ON.
  if(n==0.)n=1.e-6;
 }else if(id==115||id==116){
  if(!accepts("ms")||physical<0.||physical>2000.)return false;
  n=physical/2000.;
 }else if(id==117||id==118){
  if(!accepts("st")||physical<(id==117?-12.:-8.)||physical>(id==117?6.:6.))
   return false;
  n=id==117?(physical+12.)/18.:(physical+8.)/14.;
 }else return false;
 if(!std::isfinite(n)||n<0.||n>1.)return false;
 normalized=n;
 return true;
}
} // namespace vm
