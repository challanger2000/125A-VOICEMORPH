#include "ParameterPointCursor.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
struct Point {std::int32_t offset;double normalized;};
int main(){
 const std::array<Point,6> points{{{3,.2},{3,.5},{15,.8},{33,.35},{33,.3},{48,.1}}};
 int calls=0;
 auto fetch=[&](int i,std::int32_t& off,double& value){
  ++calls;
  if(i==4)return false; // malformed point must not drop later points
  off=points.at(i).offset;value=points.at(i).normalized;
  return true;
 };
 vm::ParameterPointCursor cursor;
 cursor.reset(static_cast<int>(points.size()),fetch);
 int emitted=0;
 double latest=0.;
 for(int sample=0;sample<48;++sample){
  const bool changed=cursor.consumeThrough(sample,fetch,[&](double value){
   latest=value;++emitted;
  });
  if(sample<3)assert(!changed);
  if(sample==3)assert(changed&&std::fabs(latest-.5)<1e-9);
  if(sample==15)assert(changed&&std::fabs(latest-.8)<1e-9);
  if(sample==33)assert(changed&&std::fabs(latest-.35)<1e-9);
  if(sample>33)assert(!changed);
 }
 assert(emitted==4);
 assert(cursor.pending());
 // Offset exactly at block end updates the next block's state, not the
 // previous block's final processed sample.
 assert(cursor.consumeThrough(48,fetch,[&](double v){latest=v;}));
 assert(latest==.1&&!cursor.pending());
 assert(calls==static_cast<int>(points.size()));
 // Host zero-sample processing may still deliver all queued changes.
 cursor.reset(static_cast<int>(points.size()),fetch);
 assert(cursor.consumeThrough(std::numeric_limits<std::int32_t>::max(),fetch,
  [&](double v){latest=v;}));
 assert(latest==.1&&!cursor.pending());
 // No points is a no-op regardless of the requested offset.
 cursor.reset(0,fetch);
 assert(!cursor.consumeThrough(999,fetch,[&](double){assert(false);}));
 std::cout<<"Sample-offset automation cursor: ordered, duplicate, malformed and zero-length PASS\n";
}
