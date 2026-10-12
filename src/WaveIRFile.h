#pragma once
#include "WaveIR.h"
#include "IRStateBlob.h"
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <exception>
#include <limits>
#include <string>
#include <vector>
namespace vm {
// File I/O and decoding are CONTROL-THREAD-ONLY. Never invoke in audio process().
inline bool loadWaveIRFile(const std::string& path,WaveIR& target,std::string& error,
                           std::size_t maxBytes=256ull*1024*1024,
                           SharedIRSource* sourceBytes=nullptr) {
 if(path.empty()){error="Empty WAV path";return false;}
 // VSTGUI supplies UTF-8. u8path also handles non-ASCII Windows filenames.
 try {
 std::ifstream file(std::filesystem::u8path(path),std::ios::binary|std::ios::ate);
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
 if(!decodeWave(bytes.data(),bytes.size(),target,error,maxBytes))return false;
 if(sourceBytes)*sourceBytes=std::make_shared<const IRSourceBytes>(std::move(bytes));
 return true;
 } catch(const std::exception&) {error="Invalid IR path or file access failure";return false;}
}
}
