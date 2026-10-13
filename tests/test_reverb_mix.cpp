#include "ReverbMixLaw.h"
#include "IRStage.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>
#include <algorithm>
int main(){
 static_assert(sizeof(float)==4,"float semantics");
 assert(vm::reverbWetMix(0.f)==0.f);
 assert(std::fabs(vm::reverbWetMix(.01f)-.0001f)<1.e-7f);
 assert(std::fabs(vm::reverbWetMix(.25f)-.0625f)<1.e-7f);
 assert(std::fabs(vm::reverbWetMix(.5f)-.25f)<1.e-7f);
 assert(vm::reverbWetMix(1.f)==1.f);
 assert(vm::reverbWetMix(-1.f)==0.f);
 assert(vm::reverbWetMix(2.f)==1.f);
 constexpr int rate=48000,frames=4096;
 vm::WaveIR wave;
 wave.channels=2;wave.sampleRate=rate;
 wave.interleaved.resize(frames*2);
 std::uint32_t seed=0x25b1fa30u;
 // Dense, deliberately over-hot 85 ms room IR to reproduce excessive
 // reverb at a tiny Mix setting without depending on a user's private WAV.
 for(int n=0;n<frames;++n){
  seed=1664525u*seed+1013904223u;
  const float sign=(seed&0x80000000u)?1.f:-1.f;
  wave.interleaved[n*2]=.5f*sign*std::exp(-2.f*n/frames);
  wave.interleaved[n*2+1]=.3f*sign*std::exp(-2.f*n/frames);
 }
 auto renderNorm=[&](bool calibrate,float mix,float level=1.f){
  vm::StereoIRStage stage;
  assert(stage.prepare(wave,rate,0,calibrate));
  stage.setWetLevel(level);stage.setMix(vm::reverbWetMix(mix));
  double energy=0.;
  double dryDelta=0.;
  float l=0,r=0;
  for(int n=0;n<frames+300;++n){
   stage.process(n==0?1.f:0.f,0.f,l,r);
   // impulse response L2 energy under a unit impulse
   energy+=double(l)*l;
   const double delta=double(l)-(n==128?1.:0.);
   dryDelta+=delta*delta;
   assert(std::isfinite(l)&&std::isfinite(r));
  }
  return std::pair<double,double>{std::sqrt(energy),std::sqrt(dryDelta)};
 };
 const auto bad=renderNorm(false,1.f);
 const auto safe=renderNorm(true,1.f);
 // A raw, hot IR can be >10x stronger than the expected input;
 // the new reverb-only preparation stage never amplifies and bounds it.
 assert(bad.first>10.);
 assert(safe.first>.1&&safe.first<=1.01);
 const auto zero=renderNorm(true,0.f);
 assert(std::fabs(zero.first-1.)<1e-4);
 assert(zero.second<1e-4);
 const auto tiny=renderNorm(true,.01f);
 assert(tiny.second<.001); // L2 error, not a sum over 4096 taps
 const auto half=renderNorm(true,.5f);
 assert(half.second>tiny.second*100.);
 const auto full=renderNorm(true,1.f);
 assert(full.second>half.second);
 // Preamp/Mic remain unaffected when normalization flag is absent.
 vm::StereoIRStage neutralMic;
 assert(neutralMic.prepare(wave,rate));
 neutralMic.setMix(1.f);
 double micEnergy=0.;float l=0.f,r=0.f;
 for(int n=0;n<frames+300;++n){
  neutralMic.process(n==0?1.f:0.f,0.f,l,r);
  micEnergy+=double(l)*l;
 }
 assert(std::sqrt(micEnergy)>10.);
 std::cout<<"Reverb-only energy calibration and 0/1/25/50/100% musical mix PASS\n";
}
