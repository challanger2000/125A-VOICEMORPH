#pragma once
#include "IRStage.h"
#include "VoiceDSP.h"
#include "PitchFormantDSP.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>
namespace vm {
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
// Host setup only. Never allocate or resize the delay during processing.
class StereoVariableDelay {
 std::array<std::vector<float>,2> samples_;
 std::size_t index_=0,length_=0;
public:
 void prepare(std::size_t delay) {
  length_=delay;index_=0;
  for(auto& channel:samples_)channel.assign(delay,0.f);
 }
 void reset() noexcept {
  for(auto& channel:samples_)std::fill(channel.begin(),channel.end(),0.f);
  index_=0;
 }
 void process(float inL,float inR,float& outL,float& outR) noexcept {
  if(length_==0){outL=inL;outR=inR;return;}
  outL=samples_[0][index_];outR=samples_[1][index_];
  samples_[0][index_]=inL;samples_[1][index_]=inR;
  if(++index_==length_)index_=0;
 }
};
class FixedLatencyIRChain {
 StereoFixedDelay<128> micEmpty_,reverbEmpty_;
 StereoVariableDelay pitchDry_,absoluteBypass_;
 std::size_t pitchLatency_=0;
 float bypassAmount_=0.f,pitchAmount_=0.f;
 static void stage(StereoIRStage* ir,StereoFixedDelay<128>& empty,
                   float& l,float& r) noexcept {
  float plainL=0,plainR=0;
  empty.process(l,r,plainL,plainR);
  if(ir && ir->ready())ir->process(l,r,l,r);
  else {l=plainL;r=plainR;}
 }
public:
 static constexpr std::size_t kLatency=256;
 FixedLatencyIRChain(){preparePitchLatency(0);} // also supports legacy IR-only hosts // Two 128-sample IR stages
 // Configure before audio processing, based on the actual spectral latency.
 void preparePitchLatency(std::size_t samples) {
  pitchLatency_=samples;
  pitchDry_.prepare(samples);
  absoluteBypass_.prepare(kLatency+samples);
  micEmpty_.reset();reverbEmpty_.reset();
  bypassAmount_=pitchAmount_=0.f;
 }
 std::size_t latencySamples()const noexcept{return kLatency+pitchLatency_;}
 void reset() noexcept {
  micEmpty_.reset();reverbEmpty_.reset();
  pitchDry_.reset();absoluteBypass_.reset();
  bypassAmount_=pitchAmount_=0.f;
 }
 void process(float l,float r,float& outL,float& outR,
              VoiceDSP& left,VoiceDSP& right,
              StereoIRStage* mic,StereoIRStage* reverb,
              bool microphonePre,bool bypass,
              PitchFormantDSP* pitch=nullptr) noexcept {
  if(!std::isfinite(l))l=0.f;
  if(!std::isfinite(r))r=0.f;
  float bypassL=0.f,bypassR=0.f;
  absoluteBypass_.process(l,r,bypassL,bypassR);
  constexpr float step=1.f/128.f;
  bypassAmount_=std::clamp(bypassAmount_+(bypass?step:-step),0.f,1.f);
  if(microphonePre)stage(mic,micEmpty_,l,r);
  if(pitch && pitch->ready()){
   float dryL=0.f,dryR=0.f,wetL=0.f,wetR=0.f;
   pitchDry_.process(l,r,dryL,dryR);
   pitch->processSample(l,r,wetL,wetR);
   const bool enabled=pitch->pitchSemitones()!=0.f||pitch->formantSemitones()!=0.f;
   pitchAmount_=std::clamp(pitchAmount_+(enabled?step:-step),0.f,1.f);
   l=dryL*(1.f-pitchAmount_)+wetL*pitchAmount_;
   r=dryR*(1.f-pitchAmount_)+wetR*pitchAmount_;
  }
  l=left.processSample(l);
  r=right.processSample(r);
  if(!microphonePre)stage(mic,micEmpty_,l,r);
  stage(reverb,reverbEmpty_,l,r);
  outL=l*(1.f-bypassAmount_)+bypassL*bypassAmount_;
  outR=r*(1.f-bypassAmount_)+bypassR*bypassAmount_;
 }
};
} // namespace vm
