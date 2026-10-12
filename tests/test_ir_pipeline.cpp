#include "WaveIR.h"
#include "IRStage.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
static void u16(std::vector<std::uint8_t>& b,unsigned x){b.push_back(x&255);b.push_back((x>>8)&255);}
static void u32(std::vector<std::uint8_t>& b,unsigned x){for(int i=0;i<4;++i)b.push_back((x>>(i*8))&255);}
int main(){
 // Stereo PCM16 WAV impulse, authored at 24k and played at 48k.
 std::vector<std::uint8_t> bytes={'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' '};
 u32(bytes,16);u16(bytes,1);u16(bytes,2);u32(bytes,24000);
 u32(bytes,96000);u16(bytes,4);u16(bytes,16);
 bytes.insert(bytes.end(),{'d','a','t','a'});u32(bytes,4);
 u16(bytes,16384);u16(bytes,8192);
 const unsigned size=static_cast<unsigned>(bytes.size()-8);
 for(int i=0;i<4;++i)bytes[4+i]=(size>>(8*i))&255;
 vm::WaveIR wave;std::string error;
 assert(vm::decodeWave(bytes.data(),bytes.size(),wave,error));
 assert(wave.channels==2&&wave.sampleRate==24000&&wave.frames()==1);
 vm::StereoIRStage stage;
 assert(stage.prepare(wave,48000,0));
 stage.setWetLevel(1.f);stage.setMix(1.f);
 double energyL=0,energyR=0;float l=0,r=0;
 for(int n=0;n<260;++n){
  stage.process(n==0?1.f:0.f,n==0?1.f:0.f,l,r);
  assert(std::isfinite(l)&&std::isfinite(r));
  if(n<128){assert(std::fabs(l)<1e-5f&&std::fabs(r)<1e-5f);}
  energyL+=std::fabs(l);energyR+=std::fabs(r);
 }
 assert(energyL>.1&&energyR>.05);
 assert(energyL>energyR*1.8&&energyL<energyR*2.2);
 // A corrupt file must not destroy the last prepared engine.
 bytes[0]='X';
 assert(!vm::decodeWave(bytes.data(),bytes.size(),wave,error));
 stage.reset();assert(stage.ready());
 std::cout<<"WAV-to-stereo-convolution pipeline PASS\n";
}
