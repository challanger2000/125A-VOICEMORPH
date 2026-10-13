#pragma once
#include "VoiceDSP.h"
#include <algorithm>
#include <cmath>
namespace vm {
// Approximate, source-documented Studio Projects B1 inverse high-frequency contour.
// Bonedo 2009: gentle rise from around 2 kHz to about +5 dB near 10 kHz.
// Empirically shaped inverse; NOT a measured calibration of an individual capsule.
class B1Correction {
 Biquad filters_[2];
 bool enabled_=false;
 float mix_=0.f;
public:
 void prepare(double rate) noexcept {
  const double sr=std::isfinite(rate)&&rate>=8000.&&rate<=384000.?rate:48000.;
  for(auto& f:filters_){f.reset();f.peak(sr,8500.,0.6,-5.);}
  mix_=enabled_?1.f:0.f;
 }
 void setEnabled(bool enabled) noexcept {enabled_=enabled;}
 bool enabled()const noexcept{return enabled_;}
 void process(float& left,float& right) noexcept {
  const float target=enabled_?1.f:0.f;
  mix_=std::clamp(mix_+std::clamp(target-mix_,-1.f/128.f,1.f/128.f),0.f,1.f);
  const float wetL=filters_[0].run(left),wetR=filters_[1].run(right);
  left=left+(wetL-left)*mix_;right=right+(wetR-right)*mix_;
 }
};
} // namespace vm
