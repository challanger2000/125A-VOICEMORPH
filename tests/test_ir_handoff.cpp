#include "IRStageHandoff.h"
#include <cassert>
#include <memory>
#include <thread>
#include <atomic>
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
 // Clear is explicitly allowed in the latest-only handoff.
 auto clearStage=std::make_unique<vm::StereoIRStage>();
 assert(bridge.publishLatest(clearStage)&&!clearStage);
 auto* cleared=bridge.acquireAudio();
 assert(cleared && !cleared->ready());
 bridge.reclaimControl();
 // Concurrent publisher and audio-block consumer: repeated swaps must never
 // result in use-after-free, incomplete stages or control-thread deadlock.
 vm::IRStageHandoff concurrent;
 std::atomic<bool> finished{false};
 std::atomic<int> reads{0};
 std::thread consumer([&]{
  while(!finished.load(std::memory_order_acquire)){
   auto* active=concurrent.acquireAudio();
   if(active){
    assert(active->ready());
    float left=0,right=0;
    active->process(.1f,.2f,left,right);
    assert(std::isfinite(left)&&std::isfinite(right));
    reads.fetch_add(1,std::memory_order_relaxed);
   }
   std::this_thread::yield();
  }
 });
 for(int n=0;n<100;++n){
  auto candidate=make();
  assert(concurrent.publishLatest(candidate)&&!candidate);
  concurrent.reclaimControl();
  std::this_thread::yield();
 }
 finished.store(true,std::memory_order_release);
 consumer.join();
 concurrent.reclaimControl();
 assert(concurrent.acquireAudio()!=nullptr);
}
