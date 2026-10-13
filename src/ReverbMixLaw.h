#pragma once
#include <algorithm>
#include <cmath>
namespace vm {
// Reverb-only wet/dry taper: 0% -> truly dry; 1% -> -80 dB wet
// coefficient (assuming normalized IR and unity LEVEL), 50% -> 25%
// linear wet, 100% -> fully wet. The UI denotes knob position;
// this is a deliberately non-linear musical control, not a fader in dB.
inline float reverbWetMix(float normalized) noexcept {
 if(!std::isfinite(normalized))return 0.f;
 const float n=std::clamp(normalized,0.f,1.f);
 return n*n;
}
}
