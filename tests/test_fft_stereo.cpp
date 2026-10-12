#include "FFTStereoIR.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <limits>
int main() {
 vm::WaveIR wave;wave.sampleRate=48000;wave.channels=4;
 wave.interleaved={1.f,.5f,.25f,.125f};
 vm::FFTStereoIR ir;
 assert(!ir.prepare(wave,44100));
 assert(ir.prepare(wave,48000));
 float l=0,r=0;
 for(int n=0;n<260;++n){
  ir.process(n==0?1.f:0.f,0.f,l,r);
  float expectedL=n==128?1.f:0.f,expectedR=n==128?.25f:0.f;
  assert(std::fabs(l-expectedL)<.00001f);
  assert(std::fabs(r-expectedR)<.00001f);
 }
 ir.reset();
 for(int n=0;n<260;++n){
  ir.process(0.f,n==0?1.f:0.f,l,r);
  assert(std::fabs(l-(n==128?.125f:0.f))<.00001f);
  assert(std::fabs(r-(n==128?.5f:0.f))<.00001f);
 }
 vm::WaveIR invalid=wave;
 invalid.interleaved[0]=std::numeric_limits<float>::quiet_NaN();
 assert(!ir.prepare(invalid,48000));
 assert(ir.ready());
 ir.reset();
 for(int n=0;n<130;++n){
  ir.process(n==0?1.f:0.f,0.f,l,r);
  if(n==128)assert(std::fabs(l-1.f)<.00001f);
 }
 vm::WaveIR bad=wave;
 bad.interleaved[0]=std::numeric_limits<float>::quiet_NaN();
 assert(!ir.prepare(bad,48000));
 assert(ir.ready());
 ir.reset();
 for(int n=0;n<130;++n){
  ir.process(n==0?1.f:0.f,0.f,l,r);
  if(n==128)assert(std::fabs(l-1.f)<.00001f);
 }
 std::cout<<"FFT stereo IR signal routing PASS\n";
}
