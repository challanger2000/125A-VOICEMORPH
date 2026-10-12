#pragma once
#include "IRStage.h"
#include <atomic>
#include <memory>
namespace vm {
// Single-producer / single-audio-consumer handoff. Non-RT thread owns file I/O,
// FFT creation, publishing and reclamation. No new/delete/locks in acquireAudio().
class IRStageHandoff {
 std::atomic<StereoIRStage*> pending_{nullptr};
 std::atomic<StereoIRStage*> retired_{nullptr};
 StereoIRStage* current_=nullptr; // only audio thread accesses current_
public:
 IRStageHandoff()=default;
 IRStageHandoff(const IRStageHandoff&)=delete;
 IRStageHandoff& operator=(const IRStageHandoff&)=delete;
 // Call when processing has stopped, after external producer has stopped.
 ~IRStageHandoff(){
  auto* p=pending_.exchange(nullptr);
  auto* r=retired_.exchange(nullptr);
  if(p!=current_&&p!=r)delete p;
  if(r!=current_)delete r;
  delete current_;
 }
 // Control thread only. Caller retains ownership if a swap is already pending.
 bool publish(std::unique_ptr<StereoIRStage>& replacement) noexcept {
  if(!replacement||!replacement->ready())return false;
  StereoIRStage* expected=nullptr;
  if(!pending_.compare_exchange_strong(expected,replacement.get(),
     std::memory_order_release,std::memory_order_relaxed))return false;
  replacement.release();return true;
 }
 // Audio thread only, once per block, not once per sample.
 StereoIRStage* acquireAudio() noexcept {
  // Do not overwrite a retired pointer before control thread has reclaimed it.
  if(retired_.load(std::memory_order_acquire)==nullptr){
   StereoIRStage* incoming=pending_.exchange(nullptr,std::memory_order_acq_rel);
   if(incoming){
    StereoIRStage* old=current_;
    current_=incoming;
    if(old)retired_.store(old,std::memory_order_release);
   }
  }
  return current_;
 }
 // Control thread only; never release on the audio callback.
 void reclaimControl() noexcept {
  delete retired_.exchange(nullptr,std::memory_order_acq_rel);
 }
};
}
