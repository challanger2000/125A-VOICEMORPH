#pragma once
#include "FFTStereoIR.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>
namespace vm {
// Offline-prepared stereo IR insert with latency-aligned dry path and wet predelay.
// prepare() and configurePredelay() are NON-realtime, process() allocates nothing.
class StereoIRStage {
 FFTStereoIR convolution_;
 std::array<std::vector<float>,2> dry_,wet_;
 std::size_t dryCursor_=0,wetCursor_=0,delay_=0;
 float mix_=1.f,level_=1.f;
 bool active_=false;
public:
 bool prepare(const WaveIR& ir,double sr,std::size_t predelaySamples=0) {
  if(sr<8000||sr>384000||predelaySamples>static_cast<std::size_t>(sr*2))return false;
  FFTStereoIR replacement;
  if(!replacement.prepare(ir,sr))return false;
  const std::size_t latency=replacement.latencySamples();
  std::array<std::vector<float>,2> nextDry, nextWet;
  for(int c=0;c<2;++c) {
   nextDry[c].assign(latency,0.f);
   nextWet[c].assign(std::max<std::size_t>(1,predelaySamples),0.f);
  }
  convolution_=std::move(replacement);
  dry_=std::move(nextDry);wet_=std::move(nextWet);
  delay_=predelaySamples;dryCursor_=wetCursor_=0;active_=true;
  return true;
 }
 void setMix(float mix) noexcept {mix_=std::isfinite(mix)?std::clamp(mix,0.f,1.f):0.f;}
 void setWetLevel(float level) noexcept {level_=std::isfinite(level)?std::clamp(level,0.f,4.f):1.f;}
 bool ready()const noexcept{return active_;}
 std::size_t latencySamples()const noexcept{return active_?convolution_.latencySamples():0;}
 void reset()noexcept {
  convolution_.reset();
  for(auto& c:dry_)std::fill(c.begin(),c.end(),0.f);
  for(auto& c:wet_)std::fill(c.begin(),c.end(),0.f);
  dryCursor_=wetCursor_=0;
 }
 void process(float left,float right,float& outputL,float& outputR) noexcept {
  if(!active_){outputL=left;outputR=right;return;}
  float wetL=0.f,wetR=0.f;
  convolution_.process(left,right,wetL,wetR);
  const float dryL=dry_[0][dryCursor_],dryR=dry_[1][dryCursor_];
  dry_[0][dryCursor_]=left;dry_[1][dryCursor_]=right;
  dryCursor_=(dryCursor_+1)%dry_[0].size();
  const float lateL=wet_[0][wetCursor_],lateR=wet_[1][wetCursor_];
  wet_[0][wetCursor_]=wetL;wet_[1][wetCursor_]=wetR;
  wetCursor_=(wetCursor_+1)%wet_[0].size();
  // Wet predelay is relative to the already aligned dry signal.
  const float processedL=delay_==0?wetL:lateL;
  const float processedR=delay_==0?wetR:lateR;
  outputL=(1.f-mix_)*dryL+mix_*level_*processedL;
  outputR=(1.f-mix_)*dryR+mix_*level_*processedR;
 }
};
}
