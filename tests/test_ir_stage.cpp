#include "IRStage.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 vm::WaveIR ir;ir.channels=2;ir.sampleRate=48000;ir.interleaved={1.f,1.f};
 vm::StereoIRStage fx;
 float l=0,r=0;fx.process(.3f,-.3f,l,r);
 assert(l==.3f&&r==-.3f);
 assert(fx.prepare(ir,48000,16));assert(fx.latencySamples()==128);
 assert(fx.predelaySamples()==16);
 fx.setMix(.5f);fx.setWetLevel(1.f);
 for(int n=0;n<180;++n){
  fx.process(n==0?1.f:0.f,0.f,l,r);
  const float expected=n==128?.5f:(n==144?.5f:0.f);
  assert(std::fabs(l-expected)<.00002f&&std::fabs(r)<.00002f);
 }
 fx.reset();fx.setMix(1.f);
 for(int n=0;n<160;++n){
  fx.process(n==0?1.f:0.f,0.f,l,r);
  assert(std::fabs(l-(n==144?1.f:0.f))<.00002f);
 }
 // Sample-accurate parameter mapping: 0.001 -> 96 samples at 48 kHz / 2 s.
 fx.reset();fx.setPredelayNormalized(.001f);
 assert(fx.predelaySamples()==96);
 for(int n=0;n<240;++n) {
  fx.process(n==0?1.f:0.f,0.f,l,r);
  assert(std::fabs(l-(n==224?1.f:0.f))<.00002f);
 }
 fx.setPredelayNormalized(0.f);
 assert(fx.predelaySamples()==0);
 vm::WaveIR bad=ir;bad.sampleRate=0;
 assert(!fx.prepare(bad,48000));assert(fx.ready());
 // True-stereo crossfeed must use the correct opposite-channel impulse.
 vm::WaveIR quad;quad.channels=4;quad.sampleRate=48000;
 quad.interleaved={1.f,1.f,.25f,.125f};
 vm::StereoIRStage cross;
 assert(cross.prepare(quad,48000,0));
 cross.setMix(1.f);
 for(int n=0;n<132;++n) {
  cross.process(n==0?1.f:0.f,0.f,l,r);
  assert(std::fabs(l-(n==128?1.f:0.f))<.00002f);
  assert(std::fabs(r-(n==128?.25f:0.f))<.00002f);
 }
 cross.reset();
 for(int n=0;n<140;++n) {
  cross.process(0.f,0.f,l,r);
  assert(std::fabs(l)<.00002f&&std::fabs(r)<.00002f);
 }
 std::cout<<"IR stage predelay and dry alignment PASS\n";
}
