#pragma once
// VST3-neutral finite sample conversion, used for both 32- and 64-bit audio.
// Host audio buffers are never modified; invalid and unrepresentable values
// are converted to zero before entering the internal float DSP.
#include <cmath>
#include <limits>
#include <type_traits>
namespace vm {
template<class Sample> float audioToFloat(Sample sample) noexcept {
 static_assert(std::is_floating_point_v<Sample>,"Floating-point VST3 audio required");
 if(!std::isfinite(sample))return 0.f;
 if constexpr(sizeof(Sample)>sizeof(float)){
  constexpr double limit=static_cast<double>(std::numeric_limits<float>::max());
  if(sample>limit||sample< -limit)return 0.f;
 }
 return static_cast<float>(sample);
}
template<class Sample> Sample floatToAudio(float sample) noexcept {
 static_assert(std::is_floating_point_v<Sample>,"Floating-point VST3 audio required");
 return static_cast<Sample>(std::isfinite(sample)?sample:0.f);
}
}
