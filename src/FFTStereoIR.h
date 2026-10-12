#pragma once
#include "WaveIR.h"
#include "IRResampler.h"
#include "PartitionedIR.h"
#include <array>
#include <algorithm>
#include <vector>
#include <cmath>
#include <utility>
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
  if(ir.sampleRate==0||hostRate<8000||hostRate>384000)return false;
  if(ir.frames()==0||ir.interleaved.size()!=ir.frames()*ir.channels)return false;
  WaveIR converted;
  if(!resampleIR(ir,static_cast<unsigned>(std::lround(hostRate)),converted,
                 static_cast<std::size_t>(hostRate*30.0)))return false;
  const WaveIR& source=converted;
  std::array<std::vector<float>,4> taps;
  for(auto& t:taps)t.assign(source.frames(),0.f);
  for(std::size_t n=0;n<source.frames();++n) {
   taps[0][n]=source.interleaved[n*source.channels];
   taps[3][n]=source.channels>=2?source.interleaved[n*source.channels+1]:taps[0][n];
   if(source.channels==4) {
    taps[1][n]=source.interleaved[n*4+2];
    taps[2][n]=source.interleaved[n*4+3];
   }
  }
  // Validate completely before modifying any DSP state.

  for(const auto& channel:taps)
   for(float sample:channel)if(!std::isfinite(sample))return false;
  // Only validated IRs reach the prepared convolution engines.
  std::array<PartitionedIR,4> replacement;
  for(int i=0;i<4;++i){
   const bool hasEnergy=std::any_of(taps[i].begin(),taps[i].end(),
                                   [](float v){return v!=0.f;});
   if(hasEnergy && !replacement[i].prepare(taps[i],
            static_cast<std::size_t>(hostRate*30.0)))return false;
  }
  path_=std::move(replacement);
  loaded_=true;return true;
 }
 bool ready()const noexcept{return loaded_;}
 std::size_t activePathCount()const noexcept {
  std::size_t count=0;for(const auto& p:path_)if(p.ready())++count;return count;
 }
 void reset() noexcept {for(auto& p:path_)p.reset();}
 void process(float l,float r,float& outL,float& outR) noexcept {
  if(!loaded_){outL=l;outR=r;return;}
  // Unused crossfeed channels must contribute zero, not pass-through audio.
  const float ll=path_[0].ready()?path_[0].processSample(l):0.f;
  const float lr=path_[1].ready()?path_[1].processSample(l):0.f;
  const float rl=path_[2].ready()?path_[2].processSample(r):0.f;
  const float rr=path_[3].ready()?path_[3].processSample(r):0.f;
  outL=ll+rl;outR=lr+rr;
 }
 std::size_t latencySamples()const noexcept{return loaded_?PartitionedIR::latencySamples():0;}
};
}
