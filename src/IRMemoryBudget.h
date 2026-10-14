#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
namespace vm {
// Bounds preparation *before* SRC/FFT allocation. Includes dense spectra,
// histories, source/converted taps and delay lines with a conservative margin.
// Per-stage limit also bounds three live stages plus three crossfade targets.
constexpr std::uint64_t kIRStageMemoryBudget=128ull*1024*1024;
inline std::uint64_t estimateIRMemory(std::size_t sourceFrames,unsigned channels,
                                    double sourceRate,double targetRate) noexcept {
 if(!std::isfinite(sourceRate)||!std::isfinite(targetRate)||sourceRate<8000.||
    targetRate<8000.||targetRate>384000.||sourceFrames==0||
    (channels!=1&&channels!=2&&channels!=4))return UINT64_MAX;
 const double frames=std::ceil(sourceFrames*targetRate/sourceRate);
 if(!std::isfinite(frames)||frames>targetRate*30.)return UINT64_MAX;
 const auto n=static_cast<std::uint64_t>(frames);
 const auto paths=channels==4?4u:2u;
 return ((n+127)/128)*paths*(256*8*2+64)+
        n*(channels+4)*sizeof(float)+
        static_cast<std::uint64_t>(sourceFrames)*channels*sizeof(float)+
        static_cast<std::uint64_t>(targetRate*2+129)*2*sizeof(float)+1024*1024;
}
}
