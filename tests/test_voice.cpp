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
 std::cout<<"Voice DSP foundation PASS\n";
}
