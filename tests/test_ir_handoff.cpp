#include "IRStageHandoff.h"
#include <cassert>
#include <memory>
int main(){
 vm::IRStageHandoff bridge;
 assert(bridge.acquireAudio()==nullptr);
 auto empty=std::make_unique<vm::StereoIRStage>();
 assert(!bridge.publish(empty)&&empty);
 vm::WaveIR wave;
 wave.channels=1;wave.sampleRate=48000;wave.interleaved={1.f};
 auto make=[&](){
  auto stage=std::make_unique<vm::StereoIRStage>();
  assert(stage->prepare(wave,48000));
  return stage;
 };
 auto first=make();
 assert(bridge.publish(first)&&!first);
 auto another=make();
 assert(!bridge.publish(another)&&another);
 assert(bridge.acquireAudio()!=nullptr);
 bridge.reclaimControl();
 assert(bridge.publish(another)&&!another);
 auto* second=bridge.acquireAudio();
 assert(second&&second->ready());
 auto third=make();
 assert(bridge.publish(third)&&!third);
 // Retirement occupies the single disposal slot; the pending exchange waits.
 assert(bridge.acquireAudio()==second);
 bridge.reclaimControl();
 assert(bridge.acquireAudio()!=second);
 bridge.reclaimControl();
 // Coalescing: a more recent IR replaces a pending one without blocking.
 auto newer=make();auto newest=make();
 assert(bridge.publishLatest(newer)&&!newer);
 assert(bridge.publishLatest(newest)&&!newest);
 assert(bridge.acquireAudio()!=nullptr);
 bridge.reclaimControl();
}
