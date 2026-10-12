#pragma once
#include "WaveIR.h"
#include <cstdint>
#include <fstream>
#include <limits>
#include <string>
#include <vector>
namespace vm {
// File I/O and decoding are CONTROL-THREAD-ONLY. Never invoke in audio process().
inline bool loadWaveIRFile(const std::string& path,WaveIR& target,std::string& error,
                           std::size_t maxBytes=256ull*1024*1024) {
 if(path.empty()){error="Empty WAV path";return false;}
 std::ifstream file(path,std::ios::binary|std::ios::ate);
 if(!file){error="Cannot open IR file";return false;}
 const auto end=file.tellg();
 if(end<=0||static_cast<std::uint64_t>(end)>maxBytes){
  error="IR file empty or too large";return false;
 }
 std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
 file.seekg(0,std::ios::beg);
 if(!file.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()))){
  error="Cannot read entire IR file";return false;
 }
 return decodeWave(bytes.data(),bytes.size(),target,error,maxBytes);
}
}
