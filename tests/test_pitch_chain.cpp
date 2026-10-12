#include "FixedLatencyIRChain.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
namespace {
constexpr float sr=48000.f,pi=3.14159265358979323846f;
float pitchOf(const std::vector<float>& x,int start,int stop){
 int rises=0;
 for(int n=start+1;n<stop;++n)if(x[n-1]<=0.f&&x[n]>0.f)++rises;
 return rises*sr/(stop-start);
}
}
int main(){
 vm::PitchFormantDSP spectral;
 assert(spectral.prepare(sr));
 const auto latency=spectral.latencySamples();
 assert(latency>0&&latency<12000);
 vm::FixedLatencyIRChain chain;
 chain.preparePitchLatency(static_cast<std::size_t>(latency));
 assert(chain.latencySamples()==static_cast<std::size_t>(latency)+256);
 vm::VoiceDSP lDSP,rDSP;
 lDSP.prepare(sr);rDSP.prepare(sr);
 // Neutral spectrum path is an exactly delayed copy, including opposite
 // channel polarity; switching either IR slot off does not change latency.
 for(int mode=0;mode<2;++mode){
  chain.reset();spectral.reset();lDSP.prepare(sr);rDSP.prepare(sr);
  for(int n=0;n<latency+300;++n){
   float l=0,r=0;
   chain.process(n==0?1.f:0.f,n==0?-.25f:0.f,l,r,lDSP,rDSP,
                 nullptr,nullptr,true,mode==1,&spectral);
   assert(std::isfinite(l)&&std::isfinite(r));
   const float expected=n==latency+256?1.f:0.f;
   assert(std::fabs(l-expected)<.0001f);
   assert(std::fabs(r+expected*.25f)<.0001f);
  }
 }
 // Enable a full octave pitch drop through the *actual host chain*.
 chain.reset();spectral.reset();lDSP.prepare(sr);rDSP.prepare(sr);
 spectral.setPitchSemitones(-12.f);spectral.setFormantSemitones(0.f);
 constexpr int count=48000;
 std::vector<float> output(count);
 for(int n=0;n<count;++n){
  float l=0,r=0;
  const float input=.12f*std::sin(2*pi*220.f*n/sr);
  chain.process(input,input,l,r,lDSP,rDSP,nullptr,nullptr,true,false,&spectral);
  assert(std::isfinite(l)&&std::isfinite(r));
  assert(std::fabs(l-r)<.01f);
  output[n]=l;
 }
 const float f=pitchOf(output,28000,45000);
 assert(std::fabs(f-110.f)<13.f);
 // Reaching a neutral state must reset the mixing target and eventually
 // regain exact dry latency rather than imposing spectral coloration.
 spectral.setPitchSemitones(0.f);
 for(int n=0;n<2*latency+128;++n){
  float l=0,r=0;
  chain.process(.12f,.12f,l,r,lDSP,rDSP,nullptr,nullptr,true,false,&spectral);
  assert(std::isfinite(l)&&std::isfinite(r));
 }
 // In bypass, the compensated host delay must stay active.
 chain.reset();spectral.reset();lDSP.prepare(sr);rDSP.prepare(sr);
 for(int n=0;n<latency+512;++n){
  float l=0,r=0;
  chain.process(n==0?1.f:0.f,0.f,l,r,lDSP,rDSP,nullptr,nullptr,true,true,&spectral);
  assert(std::isfinite(l)&&std::isfinite(r));
  assert(std::fabs(l-(n==latency+256?1.f:0.f))<.0001f);
 }
 // If a host presents a rate beyond the spectral engine's design,
 // retain a correct 256-sample IR-only fallback rather than stale PDC.
 spectral.disable();
 chain.preparePitchLatency(static_cast<std::size_t>(spectral.latencySamples()));
 assert(chain.latencySamples()==256);
 lDSP.prepare(sr);rDSP.prepare(sr);
 for(int n=0;n<512;++n){
  float l=0.f,r=0.f;
  chain.process(n==0?1.f:0.f,0.f,l,r,lDSP,rDSP,
                nullptr,nullptr,true,false,&spectral);
  assert(std::fabs(l-(n==256?1.f:0.f))<.0001f);
  assert(std::fabs(r)<.0001f);
 }
 std::cout<<"Integrated pitch and formant host chain PASS, pitch "<<f
          <<" Hz, latency "<<chain.latencySamples()<<" samples\n";
}
