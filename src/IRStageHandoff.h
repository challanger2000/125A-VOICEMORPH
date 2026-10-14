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
 std::atomic<std::uint32_t> audibleFrames_{0};
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
  if(current_)delete current_->detachPreviousControl();
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
 // Producer thread only: newer completed loads supersede any not-yet-consumed
 // IR. The displaced stage is reclaimed by this non-realtime producer.
 // Consumer uses atomic exchange, so it can never observe freed memory.
 bool publishLatest(std::unique_ptr<StereoIRStage>& replacement) noexcept {
  if(!replacement)return false;
  // An unprepared stage represents an explicit clear; never delete on audio thread.
  auto* next=replacement.release();
  auto* displaced=pending_.exchange(next,std::memory_order_acq_rel);
  delete displaced;
  return true;
 }
 // Audio thread only, once per block, not once per sample.
 StereoIRStage* acquireAudio(bool transition=true) noexcept {
  // Do not overwrite a retired pointer before control thread has reclaimed it.
  if(canInstallAudio()){
   StereoIRStage* incoming=pending_.exchange(nullptr,std::memory_order_acq_rel);
   if(incoming){
    StereoIRStage* old=current_;
    current_=incoming;
    incoming->beginTransition(transition?old:nullptr,&retired_,&audibleFrames_);
    if(!transition&&old)retired_.store(old,std::memory_order_release);
   }
  }
  return current_;
 }
 bool canInstallAudio()const noexcept {
  return retired_.load(std::memory_order_acquire)==nullptr&&
         (!current_||!current_->transitioning());
 }
 // All three restored slots are installed as one audio-boundary transaction.
 void installAudio(std::unique_ptr<StereoIRStage>& stage,bool transition=true) noexcept {
  auto* old=current_;current_=stage.release();
  current_->beginTransition(transition?old:nullptr,&retired_,&audibleFrames_);
  if(!transition&&old)retired_.store(old,std::memory_order_release);
 }
 // Setup/control only, with processing stopped. Never called from process().
 void replaceStoppedControl(std::unique_ptr<StereoIRStage> stage) noexcept {
  delete pending_.exchange(nullptr,std::memory_order_acq_rel);
  reclaimControl();
  if(current_)delete current_->detachPreviousControl();
  delete current_;current_=stage.release();
  audibleFrames_.store(current_?static_cast<std::uint32_t>(current_->responseFrames()):0,std::memory_order_release);
 }
 void discardPendingControl() noexcept {delete pending_.exchange(nullptr,std::memory_order_acq_rel);}
 std::uint32_t audibleFrames()const noexcept{return audibleFrames_.load(std::memory_order_acquire);}
 // Control thread only; never release on the audio callback.
 void reclaimControl() noexcept {
  delete retired_.exchange(nullptr,std::memory_order_acq_rel);
 }
};
}
