#include "IRStateBlob.h"
#include "WaveIR.h"
#include "IRFileBuilder.h"
#include <fstream>
#include <cstdio>
#include <cmath>
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>
#include <cstring>
int main(){
 // Valid PCM16 1-sample RIFF.
 const vm::IRSourceBytes wav={
  'R','I','F','F',38,0,0,0,'W','A','V','E',
  'f','m','t',' ',16,0,0,0,1,0,1,0,0x80,0xbb,0,0,0,0x77,1,0,2,0,16,0,
  'd','a','t','a',2,0,0,0,0xff,0x7f
 };
 auto original=std::make_shared<const vm::IRSourceBytes>(wav);
 vm::IRBlobHeader header{};
 assert(vm::prepareIRBlobHeader(original,header));
 assert(header.size==wav.size()&&header.checksum!=0);
 assert(vm::validateIRBlob(header,wav));
 // Missing original path must not disable rate-change IR reconstruction.
 assert(vm::needsIRRateRebuild("",original,44100.,48000.));
 assert(vm::needsIRRateRebuild("",original,48000.,44100.));
 assert(!vm::needsIRRateRebuild("",original,48000.,48000.));
 assert(!vm::needsIRRateRebuild("",{},44100.,48000.));
 assert(vm::needsIRRateRebuild("deleted.wav",{},44100.,48000.));
 assert(!vm::needsIRRateRebuild("deleted.wav",{},44100.,44100.));
 vm::WaveIR decoded;std::string err;
 assert(vm::decodeWave(wav.data(),wav.size(),decoded,err));
 assert(decoded.sampleRate==48000&&decoded.channels==1&&decoded.frames()==1);
 auto corrupt=wav;corrupt.back()^=0x40;
 assert(!vm::validateIRBlob(header,corrupt));
 auto truncated=wav;truncated.pop_back();
 assert(!vm::validateIRBlob(header,truncated));
 auto wrongType=wav;wrongType[0]='X';
 assert(!vm::validateIRBlob(header,wrongType));
 vm::IRBlobHeader missing{};
 assert(vm::prepareIRBlobHeader({},missing));
 assert(vm::validateIRBlob(missing,{}));
 vm::IRBlobHeader invalidLength{static_cast<std::uint32_t>(vm::kMaxEmbeddedIRBytes+1),0};
 assert(!vm::validateIRBlob(invalidLength,{}));
 vm::IRBlobHeader invalidEmpty{0,1};
 assert(!vm::validateIRBlob(invalidEmpty,{}));
 // Integration: file selection caches the exact original bytes on the
 // worker; removal of the original WAV must not prevent a v7-style recall.
 const std::string filePath="v7_portable_ir_source.wav";
 {
  std::ofstream f(filePath,std::ios::binary);
  f.write(reinterpret_cast<const char*>(wav.data()),
          static_cast<std::streamsize>(wav.size()));
  assert(f.good());
 }
 std::string error;
 vm::SharedIRSource captured;
 auto loaded=vm::IRFileBuilder::load(filePath,48000,0,error,&captured);
 assert(loaded&&loaded->ready()&&captured&&*captured==wav);
 assert(std::remove(filePath.c_str())==0);
 assert(!vm::IRFileBuilder::load(filePath,48000,0,error));
 vm::IRBlobHeader portable{};
 assert(vm::prepareIRBlobHeader(captured,portable));
 assert(vm::validateIRBlob(portable,*captured));
 vm::WaveIR recalled;
 assert(vm::decodeWave(captured->data(),captured->size(),recalled,error));
 vm::StereoIRStage restored;
 assert(restored.prepare(recalled,96000,0)&&restored.ready());
 float peak=0.f;
 for(int n=0;n<400;++n){
  float left=0.f,right=0.f;
  restored.process(n==0?1.f:0.f,0.f,left,right);
  assert(std::isfinite(left)&&std::isfinite(right));
  peak=std::max(peak,std::fabs(left));
 }
 assert(peak>.1f);
 std::cout<<"Portable WAV-IR v7 checksum, missing slot, corruption and bounds PASS\n";
}
