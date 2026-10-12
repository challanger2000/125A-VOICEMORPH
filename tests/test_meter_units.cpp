#include "MeterUnits.h"
#include <cassert>
#include <cmath>
int main(){
 assert(vm::meterToDbfs(0)==-120.);
 assert(std::abs(vm::meterToDbfs(1))<1e-8);
 assert(std::abs(vm::meterToDbfs(.5)+6.0205999)<1e-5);
 assert(std::abs(vm::dbfsToMeter(-6.0205999)-.5)<1e-6);
 assert(vm::dbfsToMeter(-120)==0);
 assert(vm::meterToDbfs(-1)==-120.);
}
