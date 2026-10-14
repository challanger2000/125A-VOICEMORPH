#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace vm {
// Main-thread project restore -> audio-thread parameter delivery, without
// mutexes, allocations or modifications of live DSP objects from setState().
// Requires exactly one project-state producer (VST3 host lifecycle thread).
// A concurrent audio automation update may override individual parameters.
template<std::size_t N> class ParameterStateMailbox {
 std::array<std::atomic<float>,N> values_{};
 std::array<std::atomic<float>,N> automation_{};
 std::array<std::atomic<std::uint64_t>,N> automationRevision_{};
 std::atomic<std::uint64_t> seq_{0}; // even = published, odd = writer busy
 std::uint64_t consumed_=0;         // audio thread, or inactive SDK lifecycle only
public:
 void initialize(const float* initial) noexcept {
  for(std::size_t i=0;i<N;++i){
   values_[i].store(initial[i],std::memory_order_relaxed);
   automation_[i].store(initial[i],std::memory_order_relaxed);
   automationRevision_[i].store(0,std::memory_order_relaxed);
  }
  seq_.store(0,std::memory_order_relaxed);
  consumed_=0;
 }
 std::uint64_t publish(const float* next) noexcept {
  // Single producer; make it impossible for audio to commit a torn restore.
  seq_.fetch_add(1,std::memory_order_acq_rel);
  for(std::size_t i=0;i<N;++i)values_[i].store(next[i],std::memory_order_relaxed);
  return seq_.fetch_add(1,std::memory_order_release)+1;
 }
 bool consumeAudio(float* target) noexcept {
  const auto before=seq_.load(std::memory_order_acquire);
  if((before&1u)||before==consumed_)return false;
  std::array<float,N> tmp{};
  for(std::size_t i=0;i<N;++i)tmp[i]=values_[i].load(std::memory_order_relaxed);
  const auto after=seq_.load(std::memory_order_acquire);
  if(before!=after)return false; // retry next audio block
  for(std::size_t i=0;i<N;++i)target[i]=tmp[i];
  consumed_=after;
  return true;
 }
 void storeAutomationAudio(std::size_t i,float value) noexcept {
  if(i<N){
   automation_[i].store(value,std::memory_order_relaxed);
   automationRevision_[i].store(consumed_,std::memory_order_release);
  }
 }
 void acceptAudioRevision(std::uint64_t revision) noexcept {consumed_=revision;}
 // Host non-audio thread. Get a consistent project-state snapshot.
 void snapshot(float* output) const noexcept {
  for(;;){
   const auto before=seq_.load(std::memory_order_acquire);
   if(before&1u)continue;
   for(std::size_t i=0;i<N;++i){
    output[i]=automationRevision_[i].load(std::memory_order_acquire)==before?
     automation_[i].load(std::memory_order_relaxed):values_[i].load(std::memory_order_relaxed);
   }
   if(before==seq_.load(std::memory_order_acquire))return;
  }
 }
};
} // namespace vm
