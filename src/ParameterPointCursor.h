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
    apply(nextValue_);
    changed=true;
   }
   ++next_;
   advance(fetch);
  }
  return changed;
 }
 bool pending()const noexcept{return next_<count_;}
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
