#pragma once
#include "IRStage.h"
#include "VoiceDSP.h"
#include <array>
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
 static void stage(StereoIRStage* ir,StereoFixedDelay<128>& empty,
                   float& l,float& r) noexcept {
  float plainL=0,plainR=0;
  empty.process(l,r,plainL,plainR); // keep the bypass history alive during IR swaps
  if(ir && ir->ready())ir->process(l,r,l,r);
  else {l=plainL;r=plainR;}
 }
public:
 static constexpr std::size_t kLatency=256;
 void reset() noexcept {micEmpty_.reset();reverbEmpty_.reset();absoluteBypass_.reset();}
 void process(float l,float r,float& outL,float& outR,
              VoiceDSP& left,VoiceDSP& right,
              StereoIRStage* mic,StereoIRStage* reverb,
              bool microphonePre,bool bypass) noexcept {
  if(!std::isfinite(l))l=0;
  if(!std::isfinite(r))r=0;
  float bypassL=0,bypassR=0;
  absoluteBypass_.process(l,r,bypassL,bypassR);
  if(bypass){outL=bypassL;outR=bypassR;return;}
  if(microphonePre)stage(mic,micEmpty_,l,r);
  l=left.processSample(l);
  r=right.processSample(r);
  if(!microphonePre)stage(mic,micEmpty_,l,r);
  stage(reverb,reverbEmpty_,l,r);
  outL=l;outR=r;
 }
};
}
