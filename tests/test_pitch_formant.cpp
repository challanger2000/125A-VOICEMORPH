#include "PitchFormantDSP.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
namespace {
constexpr float kRate=48000.f,kPi=3.14159265358979323846f;
float sine(int n,float hz){return .15f*std::sin(2*kPi*hz*n/kRate);}
float estimatePitch(const std::vector<float>& output){
 int crossings=0;
 constexpr int start=18000,stop=42000;
 for(int i=start+1;i<stop;++i)
  if(output[i-1]<=0.f&&output[i]>0.f)++crossings;
 return crossings*kRate/static_cast<float>(stop-start);
}
std::vector<float> render(float pitch,float formant,bool harmonics=false){
 vm::PitchFormantDSP fx;
 assert(fx.prepare(kRate));
 fx.setPitchSemitones(pitch);
 fx.setFormantSemitones(formant);
 std::vector<float> samples(48000);
 for(int n=0;n<48000;++n){
  float input=sine(n,harmonics?120.f:220.f);
  if(harmonics){
   input+=.08f*std::sin(2*kPi*240.f*n/kRate);
   input+=.07f*std::sin(2*kPi*360.f*n/kRate);
   input+=.045f*std::sin(2*kPi*480.f*n/kRate);
  }
  float l=0,r=0;
  fx.processSample(input,-input,l,r);
  assert(std::isfinite(l)&&std::isfinite(r));
  // Both channels in this experiment receive phase-reversed copies.
  assert(std::fabs(l+r)<.025f);
  samples[n]=l;
 }
 return samples;
}
}
int main(){
 vm::PitchFormantDSP fx;
 assert(!fx.prepare(4000));
 assert(fx.prepare(48000));
 assert(fx.ready()&&fx.latencySamples()>0&&fx.latencySamples()<12000);
 fx.setPitchSemitones(-12);
 fx.setFormantSemitones(-5);
 assert(fx.pitchSemitones()==-12&&fx.formantSemitones()==-5);
 fx.setPitchSemitones(-30);
 assert(fx.pitchSemitones()==-12); // bounded UI domain
 fx.setPitchSemitones(0);
 assert(fx.pitchSemitones()==0);
 const auto original=render(0,0);
 const auto down=render(-12,0);
 const auto up=render(6,0);
 const float frequencyDown=estimatePitch(down),frequencyUp=estimatePitch(up);
 assert(std::fabs(frequencyDown-110.f)<12.f);
 assert(std::fabs(frequencyUp-220.f*std::sqrt(2.f))<22.f);
 const auto formantsNormal=render(-4,0,true);
 const auto formantsLow=render(-4,-6,true);
 double delta=0;
 for(int n=18000;n<42000;++n)delta+=std::fabs(formantsNormal[n]-formantsLow[n]);
 assert(delta/24000.0>1e-5);
 std::cout<<"Pitch/-12 "<<frequencyDown<<" Hz, +6 "<<frequencyUp
          <<" Hz, formant difference "<<delta/24000.<<" PASS\n";
}
