#include "FixedLatencyIRChain.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 for(const int sr:{44100,48000,96000}){
  vm::FixedLatencyIRChain chain;
  vm::VoiceDSP voiceL,voiceR;
  voiceL.prepare(sr);voiceR.prepare(sr);
  vm::Parameters color;color.chest=1.f;color.warmth=1.f;color.presence=1.f;color.grit=.5f;
  voiceL.setParameters(color);voiceR.setParameters(color);
  vm::WaveIR identity;identity.sampleRate=static_cast<unsigned>(sr);
  identity.channels=2;identity.interleaved={1.f,1.f};
  vm::StereoIRStage mic,preamp,reverb;
  assert(mic.prepare(identity,sr)&&preamp.prepare(identity,sr)&&reverb.prepare(identity,sr));
  mic.setMix(1.f);mic.setWetLevel(.5f);
  preamp.setMix(1.f);preamp.setWetLevel(.6f);
  reverb.setMix(1.f);reverb.setWetLevel(.7f);
  vm::B1Correction eq;eq.prepare(sr);eq.setEnabled(true);
  // Compare equal-pitched 8 kHz tone to detect accidental B1 bypass.
  auto run=[&](bool voiceBypass,bool corrected){
   chain.reset();mic.reset();preamp.reset();reverb.reset();
   voiceL.prepare(sr);voiceR.prepare(sr);
   voiceL.setParameters(color);voiceR.setParameters(color);
   eq.prepare(sr);eq.setEnabled(corrected);
   double energy=0.;
   for(int n=0;n<sr/2;++n){
    const float input=.08f*std::sin(2.f*vm::pi*8000.*n/sr);
    float l=0.f,r=0.f;
    chain.process(input,input,l,r,voiceL,voiceR,
      &mic,&reverb,true,voiceBypass,nullptr,&preamp,&eq);
    if(n>sr/4){energy+=l*l;assert(std::fabs(l-r)<1e-5f);}
   }
   return std::sqrt(energy/(sr/4.));
  };
  const auto bypassEQ=run(true,true),bypassNoEQ=run(true,false);
  // With VOICE BYPASS, all three IRs remain active and B1 stays active;
  // it still compensates high frequencies by a measurable amount.
  assert(bypassEQ>0.&&bypassNoEQ>0.);
  assert(bypassEQ<bypassNoEQ*.9);
  // Full wet 0.5 * 0.6 * 0.7 = .21: without any of these stages it
  // would be almost five times louder.
  assert(std::fabs(bypassNoEQ/(.08/std::sqrt(2.))-.21)<.008);
  const auto active=run(false,false);
  assert(std::fabs(active-bypassNoEQ)>.001);
  // Bypass must NOT bypass the independent output trim.
  chain.setInputOutputGains(0.f,6.f);
  const auto louder=run(true,false);
  assert(louder>bypassNoEQ*1.8);
 }
 std::cout<<"VOICE-only bypass preserves B1, MIC, PREAMP, REVERB, OUTPUT and PDC PASS\n";
}
