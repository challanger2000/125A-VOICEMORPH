#pragma once
#include "WaveIR.h"
#include "Convolution.h"
#include <cstddef>
#include <vector>
#include <cmath>
namespace vm {
// Non-realtime construction helper: takes a decoded mono/stereo/4-channel WaveIR,
// isolates channels and loads short-IR reference kernels. This verifies mapping;
// full-length FFT engine is still required for shipping reverb.
struct PreparedIR {
 std::vector<float> channels[4];
 std::size_t frames=0;
 unsigned sampleRate=0;
 unsigned channelCount=0;
 bool prepare(const WaveIR& w, std::size_t maxFrames=4096) {
  if(w.sampleRate<8000||w.channels<1||w.channels>4||w.frames()==0||
     w.frames()>maxFrames||w.interleaved.size()!=w.frames()*w.channels)return false;
  PreparedIR next;next.frames=w.frames();next.sampleRate=w.sampleRate;next.channelCount=w.channels;
  for(unsigned c=0;c<w.channels;++c){
   next.channels[c].resize(w.frames());
   for(std::size_t i=0;i<w.frames();++i) {
    const float x=w.interleaved[i*w.channels+c];
    if(!std::isfinite(x))return false;
    next.channels[c][i]=x;
   }
  }
  *this=std::move(next);return true;
 }
};
// A stereo FIR matrix representing L->L,L->R,R->L,R->R.
// Mono=diagonal copy, stereo=independent diagonal IR, four-channel=4 paths.
// This path is reference only; direct convolution length is bounded above.
class StereoIRMatrix {
 ShortIR paths_[4];
 bool loaded_=false;
public:
 bool prepare(const PreparedIR& p) {
  if(p.channelCount!=1&&p.channelCount!=2&&p.channelCount!=4)return false;
  const std::vector<float> zero(p.frames,0.f);
  const auto& ll=p.channels[0];
  const auto& rr=(p.channelCount>=2)?p.channels[1]:ll;
  const auto& lr=(p.channelCount==4)?p.channels[2]:zero;
  const auto& rl=(p.channelCount==4)?p.channels[3]:zero;
  ShortIR tmp[4];
  if(!tmp[0].prepare(ll)||!tmp[1].prepare(lr)||
     !tmp[2].prepare(rl)||!tmp[3].prepare(rr))return false;
  for(unsigned c=0;c<4;++c)paths_[c]=std::move(tmp[c]);
  loaded_=true;return true;
 }
 bool ready()const noexcept{return loaded_;}
 void reset()noexcept{for(auto& p:paths_)p.reset();}
 void processSample(float left,float right,float& outLeft,float& outRight) noexcept {
  if(!loaded_){outLeft=left;outRight=right;return;}
  // Channel ordering is LL, LR, RL, RR.
  outLeft=paths_[0].processSample(left)+paths_[2].processSample(right);
  outRight=paths_[1].processSample(left)+paths_[3].processSample(right);
 }
};
}
