#include "IRTailAccounting.h"
#include "IRStage.h"
#include <cassert>
#include <cstdint>
#include <cmath>
#include <iostream>
int main(){
 for(double rate:{44100.,48000.,96000.}){
  const std::uint32_t sr=static_cast<std::uint32_t>(rate);
  assert(vm::estimateTailSamples({0,0,0},rate,false)==0);
  const auto pending=vm::pendingIRFrames(rate);
  assert(pending==sr*30);
  assert(vm::estimateTailSamples({0,0,pending},rate,false)>=sr*30-1);
  // A failed replacement must restore the previously accepted sample count;
  // the restored count is passed through the same deterministic accounting.
  assert(vm::estimateTailSamples({0,0,sr},rate,false)==sr*3-1);
  assert(vm::estimateTailSamples({0,0,0},rate,true)==2*sr);
  assert(vm::estimateTailSamples({sr,0,0},rate,false)==sr-1);
  assert(vm::estimateTailSamples({0,0,sr},rate,false)==3*sr-1);
  assert(vm::estimateTailSamples({sr,2*sr,sr/2},rate,true)==
   (sr-1)+(2*sr-1)+(sr/2-1)+4*sr);
  vm::WaveIR impulse;
  impulse.channels=2;impulse.sampleRate=sr;
  impulse.interleaved.assign(2048*2,0.f);
  impulse.interleaved[0]=1.f;impulse.interleaved[1]=1.f;
  vm::StereoIRStage stage;
  assert(stage.responseFrames()==0);
  assert(stage.prepare(impulse,rate));
  assert(stage.responseFrames()==2048);
  assert(stage.latencySamples()==128);
  assert(vm::estimateTailSamples({0,0,static_cast<std::uint32_t>(
   stage.responseFrames())},rate,false)==2047+2*sr);
 }
 assert(vm::estimateTailSamples({UINT32_MAX,UINT32_MAX,UINT32_MAX},48000.,true)==UINT32_MAX-1);
 std::cout<<"3 serial IR tails + predelay + voice allowance, excluding PDC PASS\n";
}
