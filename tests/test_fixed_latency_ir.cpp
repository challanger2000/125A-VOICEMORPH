#include "FixedLatencyIRChain.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
int main(){
 vm::VoiceDSP left,right;left.prepare(48000);right.prepare(48000);
 vm::WaveIR wave;wave.channels=2;wave.sampleRate=48000;wave.interleaved={1.f,1.f};
 vm::StereoIRStage mic,rev;
 assert(mic.prepare(wave,48000));assert(rev.prepare(wave,48000));
 mic.setMix(1.f);rev.setMix(1.f);
 vm::FixedLatencyIRChain chain;
 assert(chain.kLatency==256);
 // Both inactive, either stage active, both stages active and both PRE/POST
 // choices must align sample-perfectly, even at full wet.
 for(int mode=0;mode<10;++mode){
  chain.reset();left.prepare(48000);right.prepare(48000);
  mic.reset();rev.reset();
  const bool m=mode==1||mode==3||mode==4||mode==6||mode==8;
  const bool v=mode==2||mode==3||mode==4||mode==7||mode==8;
  const bool pre=(mode%2)==0;
  const bool bypass=mode==5||mode==9;
  for(int n=0;n<400;++n){
   float l=0,r=0;
   chain.process(n==0?1.f:0.f,n==0?-.5f:0.f,l,r,left,right,
                 m?&mic:nullptr,v?&rev:nullptr,pre,bypass);
   assert(std::isfinite(l)&&std::isfinite(r));
   assert(std::fabs(l-(n==256?1.f:0.f))<.0001f);
   assert(std::fabs(r-(n==256?-.5f:0.f))<.0001f);
  }
 }
 // Loaded impulse matrix must preserve crossfeed, still with 256 latency.
 vm::WaveIR cross;cross.channels=4;cross.sampleRate=48000;
 cross.interleaved={1.f,1.f,.25f,.125f}; // LL,RR,LR,RL
 assert(mic.prepare(cross,48000));
 chain.reset();left.prepare(48000);right.prepare(48000);
 for(int n=0;n<270;++n){
  float l=0,r=0;
  chain.process(n==0?1.f:0.f,0.f,l,r,left,right,&mic,nullptr,true,false);
  assert(std::isfinite(l)&&std::isfinite(r));
  assert(std::fabs(l-(n==256?1.f:0.f))<1e-4f);
  assert(std::fabs(r-(n==256?.25f:0.f))<1e-4f);
 }
 // Toggle bypass during a running signal. Processing is kept warm;
 // crossfades prevent full-scale jumps at the toggle boundaries.
 chain.reset();left.prepare(48000);right.prepare(48000);
 vm::Parameters elevated;elevated.outputDb=12.f;
 left.setParameters(elevated);right.setParameters(elevated);
 // The preceding true-stereo crossfeed probe changed mic's IR. Restore
 // identity impulses here so bypass gain comparisons have a valid reference.
 assert(mic.prepare(wave,48000));
 assert(rev.prepare(wave,48000));
 mic.setMix(1.f);rev.setMix(1.f);
 float before=0,enter=0,full=0,exit=0,after=0;
 for(int n=0;n<1450;++n){
  float l=0,r=0;
  const bool bypass=n>=500&&n<900;
  chain.process(.2f,.2f,l,r,left,right,&mic,&rev,true,bypass);
  if(n==499)before=l;
  if(n==500)enter=l;
  if(n==650)full=l;
  if(n==900)exit=l;
  if(n==1040)after=l;
  assert(std::isfinite(l)&&std::isfinite(r));
 }
 const float wet=.2f*vm::gain(12.f);
 assert(std::fabs(before-wet)<.002f);
 assert(std::fabs(full-.2f)<.002f);
 assert(enter<before && enter>full);
 assert(exit>full && exit<after);
 assert(std::fabs(after-wet)<.002f);
 std::cout<<"Constant VST3 dual-IR latency PASS\n";
}
