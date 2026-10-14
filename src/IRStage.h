#pragma once
#include "FFTStereoIR.h"
#include "SmoothValue.h"
#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>
namespace vm {
// Prepared off-thread. Runtime transitions retain their predecessor until the
// new dry/wet paths are warm; reclamation is delegated to a control thread.
class StereoIRStage {
 FFTStereoIR convolution_;
 std::array<std::vector<float>,2> dry_,wet_;
 std::array<std::array<float,128>,2> emptyDelay_{};
 std::size_t emptyCursor_=0,dryCursor_=0,wetCursor_=0;
 std::size_t delay_=0,nextDelay_=0,desiredDelay_=0,maxDelay_=0;
 unsigned delayFade_=0,delayFadeLength_=240;
 SmoothValue mix_,level_;
 float mixTarget_=1.f,levelTarget_=1.f;
 bool active_=false,emptyReady_=false,started_=false;
 double rate_=48000.;
 StereoIRStage* previous_=nullptr;
 std::atomic<StereoIRStage*>* retirement_=nullptr;
 std::atomic<std::uint32_t>* audibleFrames_=nullptr;
 std::size_t transitionSamples_=0,warmSamples_=0;
 unsigned transitionFade_=0,transitionLength_=480;
 float delayed(unsigned channel,std::size_t samples,float immediate)const noexcept {
  if(samples==0)return immediate;
  return wet_[channel][(wetCursor_+wet_[channel].size()-samples)%wet_[channel].size()];
 }
 void render(float left,float right,float& outputL,float& outputR) noexcept {
  if(!active_){
   if(!emptyReady_){outputL=left;outputR=right;return;}
   outputL=emptyDelay_[0][emptyCursor_];outputR=emptyDelay_[1][emptyCursor_];
   emptyDelay_[0][emptyCursor_]=left;emptyDelay_[1][emptyCursor_]=right;
   emptyCursor_=(emptyCursor_+1)%128;return;
  }
  if(!started_){
   delay_=nextDelay_=desiredDelay_;mix_.reset(mixTarget_);level_.reset(levelTarget_);
   started_=true;
  }
  float wetL=0,wetR=0;convolution_.process(left,right,wetL,wetR);
  const float dryL=dry_[0][dryCursor_],dryR=dry_[1][dryCursor_];
  dry_[0][dryCursor_]=left;dry_[1][dryCursor_]=right;
  dryCursor_=(dryCursor_+1)%dry_[0].size();
  wet_[0][wetCursor_]=wetL;wet_[1][wetCursor_]=wetR;
  if(!delayFade_&&desiredDelay_!=delay_){nextDelay_=desiredDelay_;delayFade_=delayFadeLength_;}
  float a=delayed(0,delay_,wetL),b=delayed(1,delay_,wetR);
  if(delayFade_){
   const float fade=float(delayFadeLength_-delayFade_+1)/delayFadeLength_;
   a+=(delayed(0,nextDelay_,wetL)-a)*fade;
   b+=(delayed(1,nextDelay_,wetR)-b)*fade;
   if(--delayFade_==0)delay_=nextDelay_;
  }
  wetCursor_=(wetCursor_+1)%wet_[0].size();
  const float m=mix_.next(),level=level_.next();
  outputL=(1.f-m)*dryL+m*level*a;outputR=(1.f-m)*dryR+m*level*b;
 }
public:
 bool prepare(const WaveIR& ir,double sr,std::size_t predelaySamples=0,
              bool reverbEnergyNormalize=false){
  if(!std::isfinite(sr)||sr<8000||sr>384000||predelaySamples>static_cast<std::size_t>(sr*2))return false;
  FFTStereoIR replacement;
  if(!replacement.prepare(ir,sr,reverbEnergyNormalize))return false;
  const auto latency=replacement.latencySamples();
  std::array<std::vector<float>,2> dry,wet;
  for(int c=0;c<2;++c){dry[c].assign(latency,0.f);wet[c].assign(static_cast<std::size_t>(sr*2)+1,0.f);}
  convolution_=std::move(replacement);dry_=std::move(dry);wet_=std::move(wet);
  rate_=sr;maxDelay_=static_cast<std::size_t>(sr*2);
  desiredDelay_=delay_=nextDelay_=predelaySamples;
  dryCursor_=wetCursor_=0;active_=true;emptyReady_=false;started_=false;
  mix_.prepare(sr);level_.prepare(sr);mix_.reset(mixTarget_);level_.reset(levelTarget_);
  delayFadeLength_=static_cast<unsigned>(std::ceil(sr*.005));
  transitionLength_=static_cast<unsigned>(std::ceil(sr*.010));
  return true;
 }
 void prepareEmpty(double sr=48000.) noexcept {
  emptyReady_=true;rate_=std::isfinite(sr)&&sr>=8000.&&sr<=384000.?sr:48000.;
  transitionLength_=static_cast<unsigned>(std::ceil(rate_*.010));
  for(auto& channel:emptyDelay_)channel.fill(0.f);emptyCursor_=0;
 }
 void setPredelayNormalized(float normalized) noexcept {
  if(!std::isfinite(normalized))normalized=0;
  desiredDelay_=static_cast<std::size_t>(std::lround(std::clamp(normalized,0.f,1.f)*maxDelay_));
 }
 std::size_t predelaySamples()const noexcept{return desiredDelay_;}
 void setMix(float mix) noexcept {
  mixTarget_=std::isfinite(mix)?std::clamp(mix,0.f,1.f):0.f;
  if(started_)mix_.set(mixTarget_);else mix_.reset(mixTarget_);
 }
 void setWetLevel(float level) noexcept {
  levelTarget_=std::isfinite(level)?std::clamp(level,0.f,4.f):1.f;
  if(started_)level_.set(levelTarget_);else level_.reset(levelTarget_);
 }
 bool ready()const noexcept{return active_;}
 bool processingReady()const noexcept{return active_||emptyReady_||previous_;}
 bool transitioning()const noexcept{return previous_!=nullptr;}
 std::size_t responseFrames()const noexcept{return active_?convolution_.responseFrames():0;}
 std::size_t latencySamples()const noexcept{return processingReady()?128:0;}
 void beginTransition(StereoIRStage* previous,std::atomic<StereoIRStage*>* retirement,
                      std::atomic<std::uint32_t>* audibleFrames=nullptr) noexcept {
  if(!active_)prepareEmpty(previous?previous->rate_:48000.);
  previous_=previous;retirement_=retirement;transitionSamples_=0;transitionFade_=0;
  audibleFrames_=audibleFrames;
  if(audibleFrames_)audibleFrames_->store(static_cast<std::uint32_t>(
   std::max(responseFrames(),previous?previous->responseFrames():0)),std::memory_order_release);
 }
 // Only after processing has stopped; the handoff owns this predecessor.
 StereoIRStage* detachPreviousControl() noexcept {auto* p=previous_;previous_=nullptr;return p;}
 void reset() noexcept {
  convolution_.reset();for(auto& c:dry_)std::fill(c.begin(),c.end(),0.f);
  for(auto& c:wet_)std::fill(c.begin(),c.end(),0.f);
  for(auto& c:emptyDelay_)c.fill(0.f);
  dryCursor_=wetCursor_=emptyCursor_=0;started_=false;delayFade_=0;
 }
 void process(float left,float right,float& outputL,float& outputR) noexcept {
  float oldL=0,oldR=0;
  if(previous_)previous_->render(left,right,oldL,oldR);
  render(left,right,outputL,outputR);
  if(!previous_)return;
  if(transitionSamples_++==0)warmSamples_=128+desiredDelay_;
  if(transitionSamples_<=warmSamples_){outputL=oldL;outputR=oldR;return;}
  const float fade=std::min(1.f,float(++transitionFade_)/transitionLength_);
  outputL=oldL+(outputL-oldL)*fade;outputR=oldR+(outputR-oldR)*fade;
  if(transitionFade_>=transitionLength_){
   auto* old=previous_;previous_=nullptr;
   if(audibleFrames_)audibleFrames_->store(static_cast<std::uint32_t>(responseFrames()),std::memory_order_release);
   retirement_->store(old,std::memory_order_release);
  }
 }
};
}
