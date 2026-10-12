#pragma once
#include "WaveIR.h"
#include "PartitionedIR.h"
#include <array>
#include <vector>
#include <cmath>
namespace vm {
// Offline preparation and allocation only. Processing is fully allocation-free.
// Four-channel IR channel order is LL, RR, LR, RL (as specified by our decoder contract).
// All four convolution paths are processed even when a crossfeed is muted.
class FFTStereoIR {
 std::array<PartitionedIR,4> path_;
 bool loaded_=false;
public:
 bool prepare(const WaveIR& ir,double hostRate) {
  if(ir.channels!=1&&ir.channels!=2&&ir.channels!=4)return false;
  if(ir.sampleRate==0||std::fabs(static_cast<double>(ir.sampleRate)-hostRate)>.001)return false; // resampling is not silently approximated
  if(ir.frames()==0||ir.interleaved.size()!=ir.frames()*ir.channels)return false;
  std::array<std::vector<float>,4> taps;
  for(auto& t:taps)t.assign(ir.frames(),0.f);
  for(std::size_t n=0;n<ir.frames();++n) {
   taps[0][n]=ir.interleaved[n*ir.channels];
   taps[3][n]=ir.channels>=2?ir.interleaved[n*ir.channels+1]:taps[0][n];
   if(ir.channels==4) {
    taps[1][n]=ir.interleaved[n*4+2];
    taps[2][n]=ir.interleaved[n*4+3];
   }
  }
  // Prepare on non-audio thread. An invalid file has already been checked above.
  for(int i=0;i<4;++i) if(!path_[i].prepare(taps[i]))return false;
  loaded_=true;return true;
 }
 bool ready()const noexcept{return loaded_;}
 void reset() noexcept {for(auto& p:path_)p.reset();}
 void process(float l,float r,float& outL,float& outR) noexcept {
  if(!loaded_){outL=l;outR=r;return;}
  outL=path_[0].processSample(l)+path_[2].processSample(r);
  outR=path_[1].processSample(l)+path_[3].processSample(r);
 }
 std::size_t latencySamples()const noexcept{return loaded_?PartitionedIR::latencySamples():0;}
};
}
