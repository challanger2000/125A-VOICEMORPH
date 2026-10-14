#include "IRResampler.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
int main(){
 vm::WaveIR a;a.channels=1;a.sampleRate=48000;
 a.interleaved.assign(4800,1.f);
 vm::WaveIR b;assert(vm::resampleIR(a,44100,b));
 assert(b.frames()==4410&&b.sampleRate==44100);
 // IR transfer (sum of taps) is preserved: amplitude compensates sample density.
 for(std::size_t n=100;n+100<b.frames();n+=113)
  assert(std::fabs(b.interleaved[n]-48000.f/44100.f)<.003f);
 vm::WaveIR back;assert(vm::resampleIR(b,48000,back));
 assert(back.frames()==4800);
 // Stereo channels must not leak.
 a.channels=2;a.interleaved.assign(9600,0);
 for(int n=0;n<4800;++n)a.interleaved[2*n]=.5f;
 assert(vm::resampleIR(a,96000,b));
 for(std::size_t n=100;n+100<b.frames();n+=101){
  assert(std::fabs(b.interleaved[2*n]-.25f)<.003f);
  assert(std::fabs(b.interleaved[2*n+1])<1e-6f);
 }
 auto old=b;
 a.interleaved[0]=std::numeric_limits<float>::quiet_NaN();
 assert(!vm::resampleIR(a,44100,b));
 assert(old.interleaved==b.interleaved);
 std::cout<<"IR resampling PASS\n";
}
