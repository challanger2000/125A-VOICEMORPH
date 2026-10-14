#include "IRStageHandoff.h"
#include <cassert>
#include <memory>
#include <thread>
#include <atomic>
#include <cmath>
int main(){
 vm::IRStageHandoff bridge;
 assert(bridge.acquireAudio()==nullptr);
 auto empty=std::make_unique<vm::StereoIRStage>();assert(!bridge.publish(empty)&&empty);
 auto make=[](float gain=1.f){auto stage=std::make_unique<vm::StereoIRStage>();
  assert(stage->prepare(vm::WaveIR{48000,1,{gain}},48000));return stage;};
 auto drain=[](vm::IRStageHandoff& b){auto* p=b.acquireAudio();float l=0,r=0;
  for(int n=0;n<1024;++n)p->process(.1f,.2f,l,r);
  assert(std::isfinite(l)&&std::isfinite(r));return l;};
 auto first=make();assert(bridge.publish(first)&&!first);
 auto another=make();assert(!bridge.publish(another)&&another);
 assert(bridge.acquireAudio()!=nullptr);drain(bridge);
 assert(bridge.publish(another)&&!another);auto* second=bridge.acquireAudio();assert(second&&second->ready());
 auto third=make(.5f);assert(bridge.publish(third)&&!third);
 // New IRs wait for the complete warmup/crossfade, then control reclamation.
 assert(bridge.acquireAudio()==second);drain(bridge);assert(bridge.acquireAudio()==second);
 bridge.reclaimControl();assert(bridge.acquireAudio()!=second);drain(bridge);bridge.reclaimControl();
 auto newer=make(.6f),newest=make(.7f);assert(bridge.publishLatest(newer)&&!newer);assert(bridge.publishLatest(newest)&&!newest);
 assert(std::fabs(drain(bridge)-.07f)<1e-5f);bridge.reclaimControl();
 auto clear=std::make_unique<vm::StereoIRStage>();assert(bridge.publishLatest(clear)&&!clear);
 auto* cleared=bridge.acquireAudio();assert(cleared&&!cleared->ready());assert(std::fabs(drain(bridge)-.1f)<1e-5f);bridge.reclaimControl();
 vm::IRStageHandoff concurrent;std::atomic<bool> finished{false};std::atomic<int> reads{0};
 std::thread consumer([&]{while(!finished.load(std::memory_order_acquire)){
  auto* active=concurrent.acquireAudio();if(active){float l=0,r=0;
   for(int n=0;n<64;++n){active->process(.1f,.2f,l,r);assert(std::isfinite(l)&&std::isfinite(r));}
   reads.fetch_add(1,std::memory_order_relaxed);
  }std::this_thread::yield();}});
 for(int n=0;n<100;++n){auto candidate=make();assert(concurrent.publishLatest(candidate)&&!candidate);concurrent.reclaimControl();std::this_thread::yield();}
 // Verify eventual adoption of the LAST publication, not just a non-null pointer.
 auto last=make(.25f);assert(concurrent.publishLatest(last));
 while(reads.load(std::memory_order_relaxed)==0)std::this_thread::yield();
 finished.store(true,std::memory_order_release);consumer.join();
 float final=0;for(int n=0;n<3;++n){concurrent.reclaimControl();final=drain(concurrent);}
 assert(std::fabs(final-.025f)<1e-5f);concurrent.reclaimControl();
}
