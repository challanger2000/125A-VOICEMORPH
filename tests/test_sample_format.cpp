#include "VoiceSampleFormat.h"
#include "FixedLatencyIRChain.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

namespace {
template<class Sample> std::vector<Sample> render(int samples,bool spectralMode){
 vm::VoiceDSP left,right;
 left.prepare(48000);right.prepare(48000);
 vm::PitchFormantDSP spectral;
 assert(spectral.prepare(48000));
 if(spectralMode){
  spectral.setPitchSemitones(-7.f);
  spectral.setFormantSemitones(2.f);
 }
 vm::FixedLatencyIRChain chain;
 chain.preparePitchLatency(static_cast<std::size_t>(spectral.latencySamples()));
 std::vector<Sample> result(samples*2);
 constexpr double pi=3.14159265358979323846;
 for(int n=0;n<samples;++n){
  const Sample leftInput=static_cast<Sample>(.12*std::sin(2*pi*190.*n/48000.));
  const Sample rightInput=static_cast<Sample>(.08*std::sin(2*pi*266.*n/48000.));
  float l=0,r=0;
  chain.process(vm::audioToFloat(leftInput),vm::audioToFloat(rightInput),l,r,
                left,right,nullptr,nullptr,true,false,&spectral);
  result[n*2]=vm::floatToAudio<Sample>(l);
  result[n*2+1]=vm::floatToAudio<Sample>(r);
 }
 return result;
}
}
int main(){
 using vm::audioToFloat;using vm::floatToAudio;
 constexpr float f=std::numeric_limits<float>::max();
 assert(audioToFloat(1.25f)==1.25f);
 assert(audioToFloat(1.25)==1.25f);
 assert(audioToFloat(std::numeric_limits<float>::infinity())==0.f);
 assert(audioToFloat(-std::numeric_limits<double>::infinity())==0.f);
 assert(audioToFloat(std::numeric_limits<double>::quiet_NaN())==0.f);
 assert(audioToFloat(static_cast<double>(f)*2.)==0.f);
 assert(audioToFloat(-static_cast<double>(f)*2.)==0.f);
 assert(floatToAudio<double>(.25f)==.25);
 assert(floatToAudio<float>(std::numeric_limits<float>::quiet_NaN())==0.f);
 for(bool pitch:{false,true}){
  const auto audio32=render<float>(12000,pitch);
  const auto audio64=render<double>(12000,pitch);
  assert(audio32.size()==audio64.size());
  double peak=0.,maxDifference=0.;
  for(std::size_t n=0;n<audio32.size();++n){
   assert(std::isfinite(audio32[n])&&std::isfinite(audio64[n]));
   maxDifference=std::max(maxDifference,std::fabs(static_cast<double>(audio32[n])-audio64[n]));
   peak=std::max(peak,std::fabs(static_cast<double>(audio64[n])));
  }
  assert(peak>.03);
  assert(maxDifference<.0005);
  std::cout<<"VST3 32/64 sample equivalence "
           <<(pitch?"spectral":"neutral")<<" maxDiff "<<maxDifference<<"\n";
 }
 std::cout<<"VST3 sample formats PASS\n";
}
