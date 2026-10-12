#include "WaveIRFile.h"
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
int main(){
 const std::string path="voicemorph_test_ir.wav";
 // RIFF PCM16: one mono sample, +0.5.
 const std::uint8_t bytes[]={'R','I','F','F',38,0,0,0,'W','A','V','E',
 'f','m','t',' ',16,0,0,0,1,0,1,0,0x80,0xbb,0,0,
 0,0x77,1,0,2,0,16,0,
 'd','a','t','a',2,0,0,0,0,0x40};
 {
  std::ofstream out(path,std::ios::binary);
  out.write(reinterpret_cast<const char*>(bytes),sizeof(bytes));
 }
 vm::WaveIR ir;std::string error;
 assert(vm::loadWaveIRFile(path,ir,error));
 assert(ir.frames()==1&&ir.channels==1&&ir.sampleRate==48000);
 assert(ir.interleaved[0]>.49f&&ir.interleaved[0]<.51f);
 const auto old=ir.interleaved;
 assert(!vm::loadWaveIRFile(path,ir,error,16));
 assert(ir.interleaved==old);
 std::filesystem::remove(std::filesystem::u8path(path));
 assert(!vm::loadWaveIRFile(path,ir,error));
 assert(ir.interleaved==old);
 // Confirm Unicode filenames survive UTF-8 -> platform path conversion.
 const std::string unicodePath="voicemorph_\xC3\xBC_\xE9\x9F\xB3.wav";
 {
  std::ofstream out(std::filesystem::u8path(unicodePath),std::ios::binary);
  out.write(reinterpret_cast<const char*>(bytes),sizeof(bytes));
 }
 assert(vm::loadWaveIRFile(unicodePath,ir,error));
 assert(ir.channels==1&&ir.frames()==1);
 std::filesystem::remove(std::filesystem::u8path(unicodePath));
}
