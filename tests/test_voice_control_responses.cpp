#include "VoiceDSP.h"
#include "FixedLatencyIRChain.h"
#include "ReverbMixLaw.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

// Separate isolated audible-response fixtures for EVERY VOICE tone control;
// not a test that merely finds its GUI XML tag.
struct Probe {
 int id;
 const char* name;
 float vm::Parameters::* value;
 double hz;
 float amplitude;
};
static double differenceRms(const Probe& c,int sr,float value){
 vm::VoiceDSP baseline,changed;
 baseline.prepare(sr);changed.prepare(sr);
 vm::Parameters neutral,active;
 active.*(c.value)=value;
 baseline.setParameters(neutral);changed.setParameters(active);
 double difference=0.;
 const int frames=sr;
 for(int i=0;i<frames;++i){
  const float signal=c.amplitude*std::sin(2.*vm::pi*c.hz*i/sr);
  const float a=baseline.processSample(signal),b=changed.processSample(signal);
  assert(std::isfinite(a)&&std::isfinite(b));
  if(i>=sr/3){
   const double diff=double(a)-b;
   difference+=diff*diff;
  }
 }
 return std::sqrt(difference/(frames-sr/3));
}
int main(){
 const std::array<Probe,7> probes{{
  {102,"DE-NASAL",&vm::Parameters::denasal,950.,.35f},
  {103,"CHEST",&vm::Parameters::chest,165.,.30f},
  {104,"WARMTH",&vm::Parameters::warmth,390.,.30f},
  {105,"GRIT",&vm::Parameters::grit,250.,.36f},
  {106,"PRESENCE",&vm::Parameters::presence,3400.,.30f},
  {107,"COMPRESSION",&vm::Parameters::compression,210.,.75f},
  {108,"LOW CUT",&vm::Parameters::lowCut,40.,.25f}
 }};
 for(const int sr:{44100,48000}){
  for(const Probe& p:probes){
   const double off=differenceRms(p,sr,0.f);
   const double on=differenceRms(p,sr,1.f);
   assert(off<=1e-9); // 0% is strictly neutral for this isolated control
   // Nonzero verified signal difference: detecting a disconnected/no-op knob.
   if(!(on>1e-5)){
    std::cerr<<"NO AUDIBLE DSP CHANGE: "<<p.name<<" "<<sr
             <<"Hz, deltaRMS="<<on<<"\n";
    return 2;
   }
  }
  // True input/output trim positions, independently measurable at exact PDC.
  vm::FixedLatencyIRChain chain;
  vm::VoiceDSP left,right;
  left.prepare(sr);right.prepare(sr);
  auto output=[&](float inputDb,float outputDb){
   chain.preparePitchLatency(0);
   chain.setInputOutputGains(inputDb,outputDb);
   left.prepare(sr);right.prepare(sr);
   float result=0.f;
   for(int n=0;n<430;++n){
    float a=0.f,b=0.f;
    chain.process(n==0?.1f:0.f,n==0?-.1f:0.f,a,b,
                  left,right,nullptr,nullptr,true,false);
    if(n==384){result=a;assert(std::fabs(a+b)<1.e-5f);}
   }
   return result;
  };
  const auto unity=output(0.f,0.f);
  const auto input6=output(6.f,0.f);
  const auto output6=output(0.f,6.f);
  assert(std::fabs(unity-.1f)<1.e-5f);
  assert(std::fabs(input6/unity-vm::gain(6.f))<1.e-4f);
  assert(std::fabs(output6/unity-vm::gain(6.f))<1.e-4f);
 }
 assert(vm::reverbWetMix(0.f)==0.f);
 assert(std::fabs(vm::reverbWetMix(.01f)-.0001f)<1e-7f);
 assert(vm::reverbWetMix(1.f)==1.f);
 std::cout<<"Seven isolated VOICE control effects + two true I/O trims 44.1/48 kHz PASS\n";
}
