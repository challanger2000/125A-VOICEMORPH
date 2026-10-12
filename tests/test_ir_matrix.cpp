#include "IRMatrix.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
int main(){
 vm::WaveIR wave;wave.sampleRate=48000;wave.channels=4;
 // Frame 0: LL=1, RR=.5, LR=.25, RL=.125
 wave.interleaved={1.f,.5f,.25f,.125f};
 vm::PreparedIR prep;
 assert(prep.prepare(wave));
 assert(prep.channelCount==4&&prep.frames==1);
 assert(prep.channels[0][0]==1.f&&prep.channels[3][0]==.125f);
 vm::StereoIRMatrix matrix;assert(matrix.prepare(prep));
 float l=0,r=0;
 matrix.processSample(1,0,l,r);assert(std::fabs(l-1.f)<1e-6);
 assert(std::fabs(r-.25f)<1e-6);
 matrix.reset();matrix.processSample(0,1,l,r);
 assert(std::fabs(l-.125f)<1e-6);
 assert(std::fabs(r-.5f)<1e-6);
 wave.interleaved.push_back(0);
 assert(!prep.prepare(wave));
 wave.channels=1;wave.interleaved={.5f};
 assert(prep.prepare(wave));assert(matrix.prepare(prep));
 matrix.reset();matrix.processSample(1,1,l,r);
 assert(std::fabs(l-.5f)<1e-6&&std::fabs(r-.5f)<1e-6);
 std::cout<<"IR channel matrix PASS\n";
}
