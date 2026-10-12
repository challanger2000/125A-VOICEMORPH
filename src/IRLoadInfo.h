#pragma once
#include <cstddef>
#include <string>
namespace vm {
enum class IRError : int { None=0, Missing=1, TooLarge=2, Invalid=3, Prepare=4, Other=5 };
inline bool validIRPath(const std::string& path,bool allowClear=false) noexcept {
 if(path.empty())return allowClear;
 if(path.size()>4096)return false;
 for(unsigned char c:path)if(c<32||c==127)return false;
 return true;
}
inline IRError identifyIRError(const std::string& error) noexcept {
 if(error.empty())return IRError::Other;
 if(error.find("open")!=std::string::npos||error.find("access")!=std::string::npos)return IRError::Missing;
 if(error.find("large")!=std::string::npos||error.find("oversized")!=std::string::npos||
    error.find("memory budget")!=std::string::npos)return IRError::TooLarge;
 if(error.find("prepare")!=std::string::npos||error.find("sample rate")!=std::string::npos)
  return IRError::Prepare;
 if(error.find("WAV")!=std::string::npos||error.find("RIFF")!=std::string::npos||
    error.find("codec")!=std::string::npos||error.find("format")!=std::string::npos||
    error.find("chunk")!=std::string::npos||error.find("encoding")!=std::string::npos||
    error.find("fmt")!=std::string::npos)return IRError::Invalid;
 return IRError::Other;
}
inline const char* irErrorLabel(int code) noexcept {
 switch(code){
 case 0:return "OK";
 case 1:return "FILE MISSING";
 case 2:return "FILE TOO LARGE";
 case 3:return "INVALID WAV";
 case 4:return "RATE / IR PREP";
 default:return "LOAD FAILED";
 }
}
} // namespace vm
