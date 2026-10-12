#include "IRFileBuilder.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <cstdint>
#include <vector>
int main(){
 const std::string path="ir_stereo_builder.wav";
 // Two-frame stereo PCM16 with independent L/R impulses.
 const unsigned char bytes[]={'R','I','F','F',42,0,0,0,'W','A','V','E',
 'f','m','t',' ',16,0,0,0,1,0,2,0,0x80,0xbb,0,0,
 0,0xee,2,0,4,0,16,0,'d','a','t','a',8,0,0,0,
 0,0x40,0,0x20,0,0,0,0};
 {std::ofstream file(path,std::ios::binary);
  file.write(reinterpret_cast<const char*>(bytes),sizeof(bytes));}
 std::string err;
 auto stage=vm::IRFileBuilder::load(path,48000,0,err);
 assert(stage&&err.empty());
 stage->setMix(1.f);stage->setWetLevel(1.f);
 float l=0,r=0;
 for(int n=0;n<260;++n){
  stage->process(n==0?1.f:0.f,n==0?1.f:0.f,l,r);
  if(n==128){assert(l>.49f&&l<.51f);assert(r>.24f&&r<.26f);}
 }
 // Existing instance remains alive after a failed independent replacement.
 auto missing=vm::IRFileBuilder::load("no_such_ir.wav",48000,0,err);
 assert(!missing&&!err.empty()&&stage->ready());
 std::remove(path.c_str());
}