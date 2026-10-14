#pragma once
#include "WaveIR.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>
namespace vm {
// Off-audio-thread, bandlimited windowed-sinc IR conversion. 48 taps per output.
// Applies lowpass anti-aliasing when downsampling. Never used in process().
inline bool resampleIR(const WaveIR& src,unsigned targetRate,WaveIR& dst,
                       std::size_t maxFrames=384000u*30u,
                       bool preserveTransfer=true) {
 if(src.channels<1||src.channels>4||src.sampleRate<8000||targetRate<8000||
    targetRate>384000||src.frames()==0||
    src.interleaved.size()!=src.frames()*src.channels)return false;
 for(float v:src.interleaved)if(!std::isfinite(v))return false;
 const double ratio=double(targetRate)/src.sampleRate;
 const double count=std::ceil(src.frames()*ratio);
 if(!std::isfinite(count)||count<1||count>double(maxFrames))return false;
 const auto frames=static_cast<std::size_t>(count);
 WaveIR next;next.channels=src.channels;next.sampleRate=targetRate;
 if(src.sampleRate==targetRate){next.interleaved=src.interleaved;dst=std::move(next);return true;}
 next.interleaved.resize(frames*src.channels,0.f);
 constexpr int radius=24;
 constexpr double pi=3.14159265358979323846;
 const double cutoff=std::min(1.0,ratio)*.945;
 for(std::size_t n=0;n<frames;++n){
  const double pos=double(n)/ratio;
  const auto center=static_cast<std::int64_t>(std::floor(pos));
  double weights[2*radius+1]{};
  double norm=0;
  for(int j=-radius;j<=radius;++j){
   const double t=pos-double(center+j);
   const double at=std::fabs(t)/double(radius+1);
   if(at>=1)continue;
   const double a=pi*cutoff*t;
   const double sinc=(std::fabs(a)<1e-12)?1.0:std::sin(a)/a;
   const double window=.42+.5*std::cos(pi*at)+.08*std::cos(2*pi*at);
   const double w=cutoff*sinc*window;
   weights[j+radius]=w;
   // Do not renormalize truncated edge kernels: preserve IR transient amplitude.
   norm+=w;
  }
  (void)norm;
  for(unsigned ch=0;ch<src.channels;++ch){
   double sum=0;
   for(int j=-radius;j<=radius;++j){
    const auto index=center+j;
    if(index>=0&&static_cast<std::size_t>(index)<src.frames())
     sum+=weights[j+radius]*src.interleaved[static_cast<std::size_t>(index)*src.channels+ch];
   }
   // A discrete convolution sums taps, unlike normal audio playback SRC.
   // Compensate the new sample density: H(0)=sum(h) stays invariant.
   next.interleaved[n*src.channels+ch]=static_cast<float>(sum/(preserveTransfer?ratio:1.));
  }
 }
 dst=std::move(next);return true;
}
}
