#pragma once
#include "IRStage.h"
#include "VoiceDSP.h"
#include "PitchFormantDSP.h"
#include "B1Correction.h"
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
class StereoVariableDelay {
 std::array<std::vector<float>,2> samples_;
 std::size_t index_=0,length_=0;
public:
 void prepare(std::size_t delay){
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
 StereoFixedDelay<128> micEmpty_,preampEmpty_,reverbEmpty_;
 StereoVariableDelay pitchDry_,voiceDry_;
 std::size_t pitchLatency_=0;
 float bypassAmount_=0.f,pitchAmount_=0.f;
 SmoothValue inputGainSmooth_,outputGainSmooth_;
 float inputGain_=1.f,outputGain_=1.f;
 bool started_=false;
 static void stage(StereoIRStage* ir,StereoFixedDelay<128>& empty,
                   float& l,float& r) noexcept {
  float plainL=0.f,plainR=0.f;
  empty.process(l,r,plainL,plainR);
  if(ir&&ir->processingReady())ir->process(l,r,l,r);
  else{l=plainL;r=plainR;}
 }
public:
 static constexpr std::size_t kLatency=384;
 FixedLatencyIRChain(){prepareGains(48000.);preparePitchLatency(0);}
 void setInputOutputGains(float inputDb,float outputDb) noexcept {
  inputGainSmooth_.set(gain(clamp(inputDb,-12.f,12.f)));
  outputGainSmooth_.set(gain(clamp(outputDb,-12.f,12.f)));
  if(!started_){inputGainSmooth_.reset(inputGainSmooth_.target());outputGainSmooth_.reset(outputGainSmooth_.target());}
 }
 void prepareGains(double rate) noexcept {
  inputGainSmooth_.prepare(rate);outputGainSmooth_.prepare(rate);started_=false;
  inputGainSmooth_.reset(1.f);outputGainSmooth_.reset(1.f);
 }
 void preparePitchLatency(std::size_t samples){
  started_=false;
  pitchLatency_=samples;
  pitchDry_.prepare(samples);
  voiceDry_.prepare(samples);
  micEmpty_.reset();preampEmpty_.reset();reverbEmpty_.reset();
  bypassAmount_=pitchAmount_=0.f;
 }
 float inputGain()const noexcept{return inputGain_;}
 std::size_t latencySamples()const noexcept{return kLatency+pitchLatency_;}
 void reset() noexcept {
  started_=false;
  inputGainSmooth_.reset(inputGainSmooth_.target());outputGainSmooth_.reset(outputGainSmooth_.target());
  micEmpty_.reset();preampEmpty_.reset();reverbEmpty_.reset();
  pitchDry_.reset();voiceDry_.reset();
  bypassAmount_=pitchAmount_=0.f;
 }
 // Normal path: INPUT -> B1 -> MIC -> PREAMP -> PITCH/VOICE -> REVERB -> OUTPUT.
 // Legacy MIC POST places MIC after VOICE without altering IDs/state semantics.
 void process(float l,float r,float& outL,float& outR,
              VoiceDSP& left,VoiceDSP& right,
              StereoIRStage* mic,StereoIRStage* reverb,
              bool microphonePre,bool bypass,
              PitchFormantDSP* pitch=nullptr,
              StereoIRStage* preamp=nullptr,
              B1Correction* correction=nullptr) noexcept {
  if(!std::isfinite(l))l=0.f;
  if(!std::isfinite(r))r=0.f;
  (void)microphonePre; // Deprecated control: fixed MIC->PREAMP route.
  constexpr float step=1.f/128.f;
  bypassAmount_=std::clamp(bypassAmount_+(bypass?step:-step),0.f,1.f);
  started_=true;
  inputGain_=inputGainSmooth_.next();outputGain_=outputGainSmooth_.next();
  l*=inputGain_;r*=inputGain_;
  if(correction)correction->process(l,r);
  stage(mic,micEmpty_,l,r);
  stage(preamp,preampEmpty_,l,r);
  // Tap pre-VOICE after all preceding stages, align to spectral latency.
  float dryVoiceL=0.f,dryVoiceR=0.f;
  voiceDry_.process(l,r,dryVoiceL,dryVoiceR);
    if(pitch&&pitch->ready()){
   float dryL=0.f,dryR=0.f,wetL=0.f,wetR=0.f;
   pitchDry_.process(l,r,dryL,dryR);
   pitch->processSample(l,r,wetL,wetR);
   const bool enabled=pitch->pitchSemitones()!=0.f||pitch->formantSemitones()!=0.f;
   pitchAmount_=std::clamp(pitchAmount_+(enabled?step:-step),0.f,1.f);
   l=dryL*(1.f-pitchAmount_)+wetL*pitchAmount_;
   r=dryR*(1.f-pitchAmount_)+wetR*pitchAmount_;
  }
  // Voice processing remains warm during voice-only bypass.
  l=left.processSample(l);r=right.processSample(r);
  l=l*(1.f-bypassAmount_)+dryVoiceL*bypassAmount_;
  r=r*(1.f-bypassAmount_)+dryVoiceR*bypassAmount_;
  stage(reverb,reverbEmpty_,l,r);
  outL=l*outputGain_;outR=r*outputGain_;
 }
};
} // namespace vm
