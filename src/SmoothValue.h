#pragma once
#include <algorithm>
#include <cmath>

namespace vm {
// Finite 5 ms ramps; restored/startup values are initialized before audio.
// Repeated targets never restart a ramp. No allocation or locks.
class SmoothValue {
 float current_=0.f,target_=0.f,step_=0.f;
 unsigned length_=240,remaining_=0;
 bool initialized_=false;
public:
 void prepare(double rate,double seconds=.005) noexcept {
  length_=static_cast<unsigned>(std::max(1.,std::ceil(rate*seconds)));
  initialized_=false;remaining_=0;
 }
 void set(float value) noexcept {
  if(!std::isfinite(value))return;
  if(!initialized_){reset(value);return;}
  if(value==target_)return;
  target_=value;remaining_=length_;step_=(target_-current_)/remaining_;
 }
 void reset(float value) noexcept {
  current_=target_=value;step_=0;remaining_=0;initialized_=true;
 }
 float next() noexcept {
  if(remaining_&&--remaining_==0)current_=target_;
  else if(remaining_)current_+=step_;
  return current_;
 }
 float target()const noexcept{return target_;}
 float value()const noexcept{return current_;}
 bool moving()const noexcept{return remaining_!=0;}
};
}
