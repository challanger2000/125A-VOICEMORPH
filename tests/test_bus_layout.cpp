#include "VoiceBusLayout.h"
#include "VoiceSampleFormat.h"
#include "FixedLatencyIRChain.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

namespace {
template<typename Sample>
void testMonoToStereo(){
 vm::VoiceDSP left,right;
 left.prepare(48000);right.prepare(48000);
 vm::FixedLatencyIRChain chain;
 vm::WaveIR asymmetric;
 asymmetric.sampleRate=48000;asymmetric.channels=2;
 asymmetric.interleaved={1.f,.6f};
 vm::StereoIRStage reverb;
 assert(reverb.prepare(asymmetric,48000));
 reverb.setMix(1.f);
 for(int n=0;n<800;++n){
  const Sample input=static_cast<Sample>((n>=0&&n<600)?.12:0.);
  const float mono=vm::audioToFloat(input);
  const float second=vm::rightVoiceInput(mono,0.f,1,2);
  float l=0,r=0;
  chain.process(mono,second,l,r,left,right,nullptr,&reverb,true,false);
  assert(std::isfinite(l)&&std::isfinite(r));
  const Sample outL=vm::floatToAudio<Sample>(l);
  const Sample outR=vm::floatToAudio<Sample>(r);
  if(n>310&&n<600){
   assert(std::fabs(outL-.12)<.002);
   assert(std::fabs(outR-.072)<.002);
  }
 }
}
}
int main(){
 using vm::supportedVoiceBusLayout;
 assert(supportedVoiceBusLayout(1,1));
 assert(supportedVoiceBusLayout(1,2));
 assert(supportedVoiceBusLayout(2,2));
 for(auto shape:std::array<std::array<int,2>,7>{{
  {{2,1}},{{0,1}},{{1,0}},{{3,2}},{{2,3}},{{0,0}},{{1,3}}
 }})assert(!supportedVoiceBusLayout(shape[0],shape[1]));
 assert(vm::rightVoiceInput(.2f,-.1f,2,2)==-.1f);
 assert(vm::rightVoiceInput(.2f,-.1f,1,2)==.2f);
 assert(vm::rightVoiceInput(.2f,-.1f,1,1)==0.f);
 testMonoToStereo<float>();
 testMonoToStereo<double>();
 std::cout<<"VST3 mono/mono, mono/stereo and stereo/stereo bus routing PASS\n";
}
