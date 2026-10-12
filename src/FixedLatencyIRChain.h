#pragma once
#include "IRStage.h"
#include "VoiceDSP.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
namespace vm {
// Both FFT inserts have exactly 128 samples of processing latency.
// A constant two-insert signal path allows the host to compensate once,
// regardless of which IR is loaded and regardless of PRE/POST routing.
template<std::size_t N> class StereoFixedDelay {
 std::array<std::array<float,N>,2> history_{};
 std::size_t index_=0;
public:
 void reset() noexcept {for(auto& ch:history_)ch.fill(0.f);index_=0;}
 void process(float inL,float inR,float& outL,float& outR) noexcept {
  outL=history_[0][index_];outR=history_[1][index_];
  history_[0][index_]=inL;history_[1][index_]=inR;
  if(++index_==N)index_=0;
 }
};
class FixedLatencyIRChain {
 StereoFixedDelay<128> micEmpty_,reverbEmpty_;
 StereoFixedDelay<256> absoluteBypass_;
 float bypassAmount_=0.f;
 static void stage(StereoIRStage* ir,StereoFixedDelay<128>& empty,
                   float& l,float& r) noexcept {
  float plainL=0,plainR=0;
  empty.process(l,r,plainL,plainR); // keep the bypass history alive during IR swaps
  if(ir && ir->ready())ir->process(l,r,l,r);
  else {l=plainL;r=plainR;}
 }
public:
 static constexpr std::size_t kLatency=256;
 void reset() noexcept {micEmpty_.reset();reverbEmpty_.reset();absoluteBypass_.reset();bypassAmount_=0.f;}
 void process(float l,float r,float& outL,float& outR,
              VoiceDSP& left,VoiceDSP& right,
              StereoIRStage* mic,StereoIRStage* reverb,
              bool microphonePre,bool bypass) noexcept {
  if(!std::isfinite(l))l=0;
  if(!std::isfinite(r))r=0;
  float bypassL=0,bypassR=0;
  absoluteBypass_.process(l,r,bypassL,bypassR);
  // Keep the processing history advancing during bypass so IR tails and
  // filter state do not jump when bypass is released. Smooth both edges
  // over 128 samples without changing the fixed 256-sample host latency.
  constexpr float step=1.f/128.f;
  bypassAmount_=std::clamp(bypassAmount_+(bypass?step:-step),0.f,1.f);
  if(microphonePre)stage(mic,micEmpty_,l,r);
  l=left.processSample(l);
  r=right.processSample(r);
  if(!microphonePre)stage(mic,micEmpty_,l,r);
  stage(reverb,reverbEmpty_,l,r);
  outL=l*(1.f-bypassAmount_)+bypassL*bypassAmount_;
  outR=r*(1.f-bypassAmount_)+bypassR*bypassAmount_;
 }
};
}
