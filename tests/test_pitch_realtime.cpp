#include "FixedLatencyIRChain.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <new>
#include <numeric>
#include <vector>

// Counts C++ heap allocations from the exercised audio callback path only.
// Does not intercept direct C malloc inside dependencies.
namespace {
std::atomic<bool> gTrack{false};
std::atomic<std::size_t> gAllocations{0};
}
void* operator new(std::size_t size) {
 if(gTrack.load(std::memory_order_relaxed))
  gAllocations.fetch_add(1,std::memory_order_relaxed);
 if(void* ptr=std::malloc(size?size:1))return ptr;
 throw std::bad_alloc();
}
void* operator new[](std::size_t size) {
 if(gTrack.load(std::memory_order_relaxed))
  gAllocations.fetch_add(1,std::memory_order_relaxed);
 if(void* ptr=std::malloc(size?size:1))return ptr;
 throw std::bad_alloc();
}
void operator delete(void* ptr) noexcept {std::free(ptr);}
void operator delete[](void* ptr) noexcept {std::free(ptr);}
void operator delete(void* ptr,std::size_t) noexcept {std::free(ptr);}
void operator delete[](void* ptr,std::size_t) noexcept {std::free(ptr);}

namespace {
using Clock=std::chrono::steady_clock;
constexpr int kInputPeriod=128;
constexpr int kMaxBlock=512;
constexpr int kBlocks=540;
constexpr float kPi=3.14159265358979323846f;
struct Metrics {
 double meanUs=0,p95Us=0,p99Us=0,maxUs=0,deadlineUs=0;
 int overruns=0;
 std::size_t allocations=0;
};
vm::WaveIR makeDenseIR(int rate,int durationMs,bool trueStereo){
 vm::WaveIR wave;
 wave.sampleRate=static_cast<unsigned>(rate);
 wave.channels=trueStereo?4:2;
 const std::size_t frames=static_cast<std::size_t>(rate)*durationMs/1000;
 wave.interleaved.resize(frames*wave.channels);
 for(std::size_t n=0;n<frames;++n){
  const float envelope=std::exp(-6.f*static_cast<float>(n)/static_cast<float>(frames));
  const float a=(n==0?1.f:0.f)+envelope*.012f*std::sin(.41f*n+.1f);
  const float b=(n==0?.8f:0.f)+envelope*.013f*std::sin(.29f*n+.4f);
  const auto offset=n*wave.channels;
  wave.interleaved[offset]=a;
  wave.interleaved[offset+1]=b;
  if(trueStereo){
   wave.interleaved[offset+2]=envelope*.005f*std::sin(.33f*n+.7f);
   wave.interleaved[offset+3]=envelope*.005f*std::sin(.37f*n+.2f);
  }
 }
 return wave;
}
Metrics profile(int rate,bool dense=false,int blockSize=128) {
 assert(blockSize>=16&&blockSize<=kMaxBlock);
 vm::PitchFormantDSP spectral;
 assert(spectral.prepare(rate));
 spectral.setPitchSemitones(-8.f);
 spectral.setFormantSemitones(-4.f);
 vm::VoiceDSP left,right;
 left.prepare(rate);right.prepare(rate);
 vm::FixedLatencyIRChain chain;
 chain.preparePitchLatency(static_cast<std::size_t>(spectral.latencySamples()));
 vm::StereoIRStage mic,reverb;
 vm::StereoIRStage* micPtr=nullptr;
 vm::StereoIRStage* revPtr=nullptr;
 if(dense){
  // Exact same full signal path as the empty-IR probe, but both FFT stages
  // are filled with reproducible non-sparse 700 ms IRs. PRE microphone uses
  // true-stereo (4 convolution paths) and reverb uses stereo (2 paths).
  const auto micWave=makeDenseIR(rate,700,true);
  const auto revWave=makeDenseIR(rate,700,false);
  assert(mic.prepare(micWave,rate)&&reverb.prepare(revWave,rate));
  mic.setMix(.5f);reverb.setMix(.4f);
  mic.setWetLevel(.8f);reverb.setWetLevel(.8f);
  micPtr=&mic;revPtr=&reverb;
 }

 assert(chain.latencySamples()==static_cast<std::size_t>(spectral.latencySamples())+256);
 const auto latency=chain.latencySamples();
 assert(latency>=256&&latency<30000);
 std::array<float,kMaxBlock> a{},b{};
 for(int i=0;i<kMaxBlock;++i){
  const float p=2.f*kPi*(i%kInputPeriod)*183.f/static_cast<float>(rate);
  a[i]=.13f*std::sin(p)+.05f*std::sin(2.f*p);
  b[i]=.13f*std::sin(p+.42f)+.05f*std::sin(2.f*p+.42f);
 }
 // Pre-roll all spectral internal buffers, then enable C++ heap monitoring.
 for(int n=0;n<rate;++n){
  float l=0,r=0;
  chain.process(a[n%kMaxBlock],b[n%kMaxBlock],l,r,
                left,right,micPtr,revPtr,true,false,&spectral);
  assert(std::isfinite(l)&&std::isfinite(r));
 }
 // Sample small-buffer FFT spikes over enough completed partitions.
 const int blocks=dense?(blockSize<128?512:180):kBlocks;
 std::vector<double> durations;
 durations.reserve(blocks);
 gAllocations.store(0,std::memory_order_relaxed);
 for(int block=0;block<blocks;++block) {
  const auto start=Clock::now();
  gTrack.store(true,std::memory_order_relaxed);
  if(block%32==0) {
   const bool low=(block/32)%2==0;
   spectral.setPitchSemitones(low?-12.f:6.f);
   spectral.setFormantSemitones(low?-6.f:4.f);
  }
  float lastL=0,lastR=0;
  for(int n=0;n<blockSize;++n){
   chain.process(a[n],b[n],lastL,lastR,left,right,micPtr,revPtr,
                 true,block%91>=75,&spectral);
  }
  gTrack.store(false,std::memory_order_relaxed);
  const auto end=Clock::now();
  assert(std::isfinite(lastL)&&std::isfinite(lastR));
  durations.push_back(std::chrono::duration<double,std::micro>(end-start).count());
 }
 const auto allocations=gAllocations.load(std::memory_order_relaxed);
 const double deadlineUs=blockSize*1.e6/static_cast<double>(rate);
 const int overruns=static_cast<int>(std::count_if(durations.begin(),durations.end(),
                            [deadlineUs](double us){return us>deadlineUs;}));
 const double meanUs=std::accumulate(durations.begin(),durations.end(),0.0)/durations.size();
 std::sort(durations.begin(),durations.end());
 const auto pct=[&durations](double fraction){
  return durations[std::min(durations.size()-1,
    static_cast<std::size_t>(std::ceil(fraction*durations.size()))-1)];
 };
 return {meanUs,pct(.95),pct(.99),durations.back(),deadlineUs,overruns,allocations};
}
}
int main(){
 for(const int rate:{44100,48000,96000}){
  const Metrics m=profile(rate);
  std::cout<<"pitch-rt rate="<<rate<<" ir=none block=128 frames, "
           <<"mean="<<m.meanUs<<"us p95="<<m.p95Us<<"us p99="<<m.p99Us
           <<"us max="<<m.maxUs<<"us deadline="<<m.deadlineUs
           <<"us overruns="<<m.overruns<<" cpp_allocations="<<m.allocations
           <<"\n";
  // Allocation-free is a hard gate; shared runner timing remains diagnostic.
  if(m.allocations!=0){
   std::cerr<<"FAIL: audio callback made C++ heap allocations\n";
   return 1;
  }
 }
 // Dense true-stereo IRs are measured at multiple *host* block sizes:
 // the uniform 128-sample FFT kernel has burst costs that 128-sample
 // averages can hide. Do not use these GitHub runner values as DAW proof.
 // This is a private Studio One plugin. Measure dense loaded-IR stress
 // at the two actual project rates instead of prioritizing 96 kHz.
 for(const int rate:{44100,48000}){
  for(const int block:{16,32,64,128,256,512}){
  const Metrics m=profile(rate,true,block);
  std::cout<<"pitch-rt rate="<<rate
           <<" ir=700ms true-stereo-mic+stereo-reverb block="<<block<<" frames, "
           <<"mean="<<m.meanUs<<"us p95="<<m.p95Us<<"us p99="<<m.p99Us
           <<"us max="<<m.maxUs<<"us deadline="<<m.deadlineUs
           <<"us overruns="<<m.overruns<<" cpp_allocations="<<m.allocations
           <<"\n";
  if(m.allocations!=0){
   std::cerr<<"FAIL: dense IR callback made C++ heap allocations\n";
   return 1;
  }
  }
 }
 std::cout<<"pitch-rt C++ allocation-free callback PASS\n";
}
