#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
namespace vm {
// Deterministic reference convolution for short microphone IRs.
// prepare() is CONTROL THREAD ONLY; processSample() is allocation-free and bounded.
// O(N) per sample, intentionally a reference kernel rather than shipping long-IR reverb.
class ShortIR {
 std::vector<float> taps_, delay_;
 std::size_t cursor_=0;
public:
 bool prepare(const std::vector<float>& taps,std::size_t maxTaps=4096) {
  if(taps.empty()||taps.size()>maxTaps)return false;
  for(float v:taps)if(!std::isfinite(v))return false;
  std::vector<float> nextDelay(taps.size(),0.f);
  taps_=taps;delay_.swap(nextDelay);cursor_=0;return true;
 }
 bool ready() const noexcept {return !taps_.empty();}
 std::size_t length() const noexcept {return taps_.size();}
 void reset() noexcept {std::fill(delay_.begin(),delay_.end(),0.f);cursor_=0;}
 float processSample(float x) noexcept {
  if(!ready())return x;
  delay_[cursor_]=std::isfinite(x)?x:0.f;
  double sum=0;
  std::size_t p=cursor_;
  for(float tap:taps_){sum+=static_cast<double>(tap)*delay_[p];p=(p==0)?delay_.size()-1:p-1;}
  cursor_=(cursor_+1)%delay_.size();
  return std::isfinite(sum)?static_cast<float>(sum):0.f;
 }
};
class WetDryIR {
 ShortIR conv_;
 float mix_=0;
public:
 bool prepare(const std::vector<float>& taps,std::size_t maxTaps=4096) {return conv_.prepare(taps,maxTaps);}
 void setMix(float m) noexcept {mix_=std::isfinite(m)?std::max(0.f,std::min(1.f,m)):0.f;}
 float mix() const noexcept {return mix_;}
 bool ready() const noexcept {return conv_.ready();}
 void reset() noexcept {conv_.reset();}
 float processSample(float x) noexcept {
  if(!conv_.ready()||mix_<=0)return x;
  float wet=conv_.processSample(x);
  return x*(1-mix_)+wet*mix_;
 }
};
// Reverb-IR reference convolution is currently RESEARCH ONLY, not used for long IRs.
// It must be replaced with partitioned FFT convolution before release.
}
