#pragma once
// Independent non-AI spectral pitch/formant experiment, not yet in VST3 signal path.
// Signalsmith Stretch and Linear are MIT licensed; license notices under third_party/.
#include "third_party/signalsmith-stretch.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
namespace vm {
class PitchFormantDSP {
 signalsmith::stretch::SignalsmithStretch<float> engine_{125};
 double rate_=48000.0;
 bool ready_=false;
 float pitchSemitones_=0,formantSemitones_=0;
public:
 bool prepare(double sampleRate){
  if(!std::isfinite(sampleRate)||sampleRate<8000.||sampleRate>384000.)return false;
  rate_=sampleRate;
  // Speech analysis window ~50 ms, hop ~12.5 ms. Exact latency is queried
  // from the library; arbitrary buffer sizes would be wrong for host PDC.
  const int window=std::max(1024,static_cast<int>(std::lround(rate_*.05)));
  const int interval=std::max(256,window/4);
  engine_.configure(2,window,interval,true);
  engine_.setTransposeSemitones(pitchSemitones_);
  engine_.setFormantSemitones(formantSemitones_,true);
  engine_.setFormantBase(static_cast<float>(150./rate_));
  engine_.reset();
  ready_=true;
  return true;
 }
 void reset(){if(ready_)engine_.reset();}
 void setPitchSemitones(float semitones) {
  if(!std::isfinite(semitones))return;
  semitones=std::clamp(semitones,-12.f,6.f);
  if(semitones==pitchSemitones_)return;
  pitchSemitones_=semitones;
  if(ready_)engine_.setTransposeSemitones(semitones);
 }
 void setFormantSemitones(float semitones) {
  if(!std::isfinite(semitones))return;
  semitones=std::clamp(semitones,-8.f,6.f);
  if(semitones==formantSemitones_)return;
  formantSemitones_=semitones;
  if(ready_)engine_.setFormantSemitones(semitones,true);
 }
 int latencySamples() const noexcept {
  return ready_?engine_.inputLatency()+engine_.outputLatency():0;
 }
 bool ready()const noexcept{return ready_;}
 float pitchSemitones()const noexcept{return pitchSemitones_;}
 float formantSemitones()const noexcept{return formantSemitones_;}
 void processSample(float inputL,float inputR,float& outputL,float& outputR) {
  if(!ready_){outputL=inputL;outputR=inputR;return;}
  float inL=std::isfinite(inputL)?inputL:0.f;
  float inR=std::isfinite(inputR)?inputR:0.f;
  float outL=0.f,outR=0.f;
  float* inputs[]={&inL,&inR};
  float* outputs[]={&outL,&outR};
  engine_.process(inputs,1,outputs,1);
  outputL=std::isfinite(outL)?outL:0.f;
  outputR=std::isfinite(outR)?outR:0.f;
 }
};
}
