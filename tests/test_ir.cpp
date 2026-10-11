#include "Convolution.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>
#include <iostream>
int main(){
 vm::ShortIR ir;
 assert(!ir.prepare({}));
 assert(!ir.prepare({1.f,std::numeric_limits<float>::quiet_NaN()}));
 assert(!ir.prepare(std::vector<float>(4097,0.f)));
 assert(ir.prepare({1.f}));assert(ir.processSample(.25f)==.25f);
 assert(ir.prepare({.5f,.25f,-.125f}));
 const float input[]={1,0,0,0,0};
 const float expect[]={.5f,.25f,-.125f,0,0};
 for(int i=0;i<5;++i)assert(std::abs(ir.processSample(input[i])-expect[i])<1e-6f);
 ir.reset();assert(ir.processSample(0)==0);
 vm::WetDryIR mic,reverb;
 assert(mic.prepare({.5f}));assert(reverb.prepare({0.f,1.f}));
 mic.setMix(1);reverb.setMix(.25f);
 float a=mic.processSample(1.f);assert(a==.5f);
 float b=reverb.processSample(a);assert(std::fabs(b-.375f)<1e-6);
 a=mic.processSample(0);b=reverb.processSample(a);assert(std::fabs(b-.125f)<1e-6);
 reverb.setMix(0);assert(reverb.processSample(.42f)==.42f);
 std::cout<<"Convolution reference PASS\n";
}
