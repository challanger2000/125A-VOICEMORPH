#include "FixedLatencyIRChain.h"
#include "IRStateBlob.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <vector>

namespace {
constexpr double pi=3.14159265358979323846;
void verifyHostRate(int rate){
 vm::PitchFormantDSP pitch;
 assert(pitch.prepare(rate));
 assert(pitch.ready()&&pitch.latencySamples()>0);
 const auto spectralLatency=static_cast<std::size_t>(pitch.latencySamples());
 vm::VoiceDSP left,right;
 left.prepare(rate);right.prepare(rate);
 vm::FixedLatencyIRChain chain;
 chain.preparePitchLatency(spectralLatency);
 assert(chain.latencySamples()==spectralLatency+384);
 const vm::WaveIR impulse{static_cast<std::uint32_t>(rate),2,{1.f,1.f}};
 vm::StereoIRStage mic,reverb;
 assert(mic.prepare(impulse,rate));
 assert(reverb.prepare(impulse,rate));
 mic.setMix(1.f);reverb.setMix(1.f);
 const int length=static_cast<int>(chain.latencySamples()+390);
 // Host callback sizes vary independently of the 128-sample IR partitions.
 for(const int block:{16,32,64,128,256,512}){
  chain.reset();pitch.reset();mic.reset();reverb.reset();
  left.prepare(rate);right.prepare(rate);
  for(int base=0;base<length;base+=block){
   for(int n=base;n<std::min(base+block,length);++n){
    float l=0.f,r=0.f;
    chain.process(n==0?1.f:0.f,n==0?-.25f:0.f,l,r,
                  left,right,&mic,&reverb,true,false,&pitch);
    assert(std::isfinite(l)&&std::isfinite(r));
    const float expected=n==static_cast<int>(chain.latencySamples())?1.f:0.f;
    assert(std::fabs(l-expected)<.0004f);
    assert(std::fabs(r+.25f*expected)<.0004f);
   }
  }
 }
 // Reinitializing the same engine at another host rate must recompute its
 // latency and reset all IR/pitch histories, with no stale 44.1/48-k data.
 chain.reset();pitch.reset();mic.reset();reverb.reset();
 left.prepare(rate);right.prepare(rate);
 pitch.setPitchSemitones(-12.f);
 pitch.setFormantSemitones(-2.f);
 constexpr double hz=220.;
 std::vector<float> signal(rate);
 for(int n=0;n<rate;++n){
  float l=0.f,r=0.f;
  const float x=.12f*static_cast<float>(std::sin(2*pi*hz*n/rate));
  chain.process(x,x,l,r,left,right,&mic,&reverb,true,false,&pitch);
  assert(std::isfinite(l)&&std::isfinite(r));
  signal[n]=l;
 }
 const int begin=rate*55/100,stop=rate*95/100;
 int rises=0;
 for(int n=begin+1;n<stop;++n)
  if(signal[n-1]<=0.f&&signal[n]>0.f)++rises;
 const double outputHz=static_cast<double>(rises)*rate/(stop-begin);
 assert(std::fabs(outputHz-110.)<22.);
 std::cout<<"Host "<<rate<<" Hz: dual-IR PDC "<<chain.latencySamples()
          <<" samples, -12 st pitch "<<outputHz<<" Hz PASS\n";
}
void verifyPortableResample(){
 // An IR originally sampled at 44.1 kHz is resampled off the audio
 // thread when the video session is opened at 48 kHz, and vice versa.
 for(const int origin:{44100,48000}){
  const int target=origin==44100?48000:44100;
  vm::WaveIR stored{static_cast<std::uint32_t>(origin),2,{1.f,.7f}};
  vm::StereoIRStage restored;
  assert(restored.prepare(stored,target));
  restored.setMix(1.f);
  float maxL=0.f,maxR=0.f;
  for(int n=0;n<384;++n){
   float l=0.f,r=0.f;
   restored.process(n==0?1.f:0.f,n==0?1.f:0.f,l,r);
   assert(std::isfinite(l)&&std::isfinite(r));
   maxL=std::max(maxL,std::fabs(l));
   maxR=std::max(maxR,std::fabs(r));
  }
  assert(maxL>.35f&&maxR>.20f);
  assert(std::fabs(maxL-maxR)>.03f);
 }
}
}
int main(){
 verifyHostRate(44100);
 verifyHostRate(48000);
 // Repeating the primary rate simulates returning from video to music.
 verifyHostRate(44100);
 verifyPortableResample();
 std::cout<<"Private Studio One 44.1/48 kHz rate-switch regression PASS\n";
}
