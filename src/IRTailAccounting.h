#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
namespace vm {
// Audio tail (without plugin latency). Sum the convolution spans because
// MICROPHONE/PREAMP/REVERB inserts are serial. Wet PREDELAY may vary under
// automation, so conservatively budget its full documented 0..2 seconds.
// Voice IIR filters have mathematical infinite support: the 2-second
// acoustic recovery allowance is an explicit engineering approximation.
inline std::uint32_t estimateTailSamples(
 const std::array<std::uint32_t,3>& frames,
 double hostRate,bool voiceFilterEnabled) noexcept {
 if(!std::isfinite(hostRate)||hostRate<8000.||hostRate>384000.)return 0;
 std::uint64_t count=0;
 for(std::uint32_t n:frames)if(n>0)count+=static_cast<std::uint64_t>(n-1);
 if(frames[2]>0)count+=static_cast<std::uint64_t>(std::ceil(hostRate*2.));
 if(voiceFilterEnabled)count+=static_cast<std::uint64_t>(std::ceil(hostRate*2.));
 return count>=std::numeric_limits<std::uint32_t>::max()?
  std::numeric_limits<std::uint32_t>::max()-1:static_cast<std::uint32_t>(count);
}
} // namespace vm
