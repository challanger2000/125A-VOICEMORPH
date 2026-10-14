#pragma once
// VST-neutral reader for queues of sample-offset automation points.
// The host-facing layer supplies a non-allocating fetch(index,offset,value)
// callback; no copies of unbounded point lists are stored in the audio thread.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
namespace vm {
class ParameterPointCursor {
 std::int32_t count_=0,next_=0,nextOffset_=std::numeric_limits<std::int32_t>::max();
 double nextValue_=0.;
 std::int32_t previousOffset_=-1;
 double previousValue_=0.;
 bool interpolate_=false;
public:
 template<class Fetch> void reset(std::int32_t count,Fetch&& fetch) noexcept {
  count_=std::max<std::int32_t>(0,count);next_=0;
  advance(fetch);
 }
 template<class Fetch,class Apply>
 bool consumeThrough(std::int32_t sampleOffset,Fetch&& fetch,Apply&& apply) noexcept {
  bool changed=false;
  while(next_<count_&&nextOffset_<=sampleOffset){
   if(std::isfinite(nextValue_)){
    previousOffset_=nextOffset_;previousValue_=nextValue_;
    apply(nextValue_);
    changed=true;
   }
   ++next_;
   advance(fetch);
  }
  return changed;
 }
 bool pending()const noexcept{return next_<count_;}
 // SDK IParamValueQueue is piecewise linear. The preceding block's final
 // value is the implicit point at offset -1. Switches use step events.
 template<class Fetch> void reset(std::int32_t count,Fetch&& fetch,
                                  double current,bool interpolate) noexcept {
  previousOffset_=-1;previousValue_=current;interpolate_=interpolate;
  reset(count,fetch);
 }
 template<class Fetch> double valueAt(std::int32_t offset,Fetch&& fetch) noexcept {
  consumeThrough(offset,fetch,[](double) noexcept {});
  if(interpolate_&&pending()&&std::isfinite(nextValue_)&&nextOffset_>previousOffset_){
   const double position=double(offset-previousOffset_)/(nextOffset_-previousOffset_);
   return previousValue_+(nextValue_-previousValue_)*std::clamp(position,0.,1.);
  }
  return previousValue_;
 }
private:
 template<class Fetch> void advance(Fetch&& fetch) noexcept {
  nextOffset_=std::numeric_limits<std::int32_t>::max();
  while(next_<count_){
   std::int32_t offset=0;
   double value=0.;
   if(fetch(next_,offset,value)){
    nextOffset_=std::max<std::int32_t>(offset,0);
    nextValue_=value;
    return;
   }
   ++next_;
  }
 }
};
}
