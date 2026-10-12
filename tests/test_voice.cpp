#include "VoiceDSP.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 vm::VoiceDSP dsp;dsp.prepare(48000);
 for(int i=0;i<2048;++i)assert(dsp.processSample(0)==0);
 assert(dsp.meters().outputPeak.load()==0);
 assert(std::fabs(vm::gain(-12)-.25118864f)<1e-5);
 for(double rate:{44100.,48000.,96000.,192000.}){
  dsp.prepare(rate);vm::Parameters p;p.inputDb=6;p.outputDb=-6;p.denasal=.7f;p.chest=.5f;p.grit=.2f;p.compression=.5f;p.lowCut=.8f;dsp.setParameters(p);
  for(int i=0;i<8192;++i){float out=dsp.processSample(.3f*std::sin(2*vm::pi*i*210/rate));assert(std::isfinite(out)&&std::fabs(out)<10);}
  assert(dsp.meters().inputPeak.load()>.5f);assert(dsp.meters().outputPeak.load()>0);
 }
 dsp.prepare(48000);dsp.setParameters({});
 for(int i=0;i<2048;++i){float x=.3f*std::sin(2*vm::pi*i*200/48000);assert(std::fabs(dsp.processSample(x)-x)<.0001f);}
 dsp.prepare(48000);assert(dsp.processSample(NAN)==0);
 // Regression: dynamic detector remains finite across silence, impulses and hot signals.
 vm::Parameters nasal;nasal.denasal=1.f;dsp.prepare(48000);dsp.setParameters(nasal);
 for(int n=0;n<96000;++n){
  float x=(n%3000==0)?2.f:.4f*std::sin(2*vm::pi*920*n/48000.);
  float y=dsp.processSample(x);
  assert(std::isfinite(y)&&std::fabs(y)<10.f);
 }
 nasal.denasal=0;dsp.setParameters(nasal);
 for(int n=0;n<2048;++n)assert(std::isfinite(dsp.processSample(0.f)));
 // Each localized resonance must stay bounded across supported sample rates.
 for(double rate:{44100.,48000.,96000.})for(double hz:{680.,1050.,1450.}){
  dsp.prepare(rate);vm::Parameters p;p.denasal=1;dsp.setParameters(p);
  double sum=0;
  for(int n=0;n<static_cast<int>(rate);++n) {
   const float y=dsp.processSample(.4f*std::sin(2*vm::pi*hz*n/rate));
   assert(std::isfinite(y));sum+=double(y)*y;
  }
  assert(sum>0 && sum<rate);
 }
 // LOW CUT: 0 is transparent, 100% attenuates 40 Hz while preserving 1 kHz.
 for(float cut:{0.f,1.f}){
  vm::Parameters p;p.lowCut=cut;dsp.prepare(48000);dsp.setParameters(p);
  double low=0,high=0;
  for(int n=0;n<48000;++n){
   auto y=dsp.processSample(.2f*std::sin(2*vm::pi*40*n/48000));
   if(n>24000)low+=y*y;
  }
  dsp.prepare(48000);dsp.setParameters(p);
  for(int n=0;n<48000;++n){
   auto y=dsp.processSample(.2f*std::sin(2*vm::pi*1000*n/48000));
   if(n>24000)high+=y*y;
  }
  if(cut==0.f){assert(std::fabs(low-high)<2.0);}
  else assert(low<high*.15);
 }
 std::cout<<"Voice DSP foundation PASS\n";
}
