#include "PartitionedIR.h"
#include <cassert>
#include <cmath>
#include <vector>
#include <iostream>
int main(){
 constexpr int B=128;
 vm::PartitionedIR fft;
 assert(!fft.prepare({}));
 std::vector<float> taps(600);
 for(int i=0;i<600;++i)taps[i]=.2f*std::exp(-i/110.f)*std::cos(i*.12f);
 assert(fft.prepare(taps));
 assert(fft.partitionCount()==5&&fft.activePartitionCount()==5);
 std::vector<float> x(2048),ref(2048);
 x[0]=1;x[99]=.4f;x[256]=-.3f;
 for(int n=0;n<2048;++n)
  for(int k=0;k<600&&k<=n;++k)ref[n]+=taps[k]*x[n-k];
 for(int n=0;n<2048;++n){
  float got=fft.processSample(x[n]);
  float expected=(n>=B)?ref[n-B]:0.f;
  assert(std::isfinite(got)&&std::abs(got-expected)<.00015f);
 }
 fft.reset();
 for(int n=0;n<512;++n)assert(std::abs(fft.processSample(0))<1e-6f);
 // Sparse IR: only three populated partitions out of 300 should consume MACs.
 std::vector<float> sparse(128*300,0.f);
 sparse[0]=.75f;sparse[128*111]=-.25f;sparse[128*299]=.125f;
 assert(fft.prepare(sparse));
 assert(fft.ready()&&fft.partitionCount()==300&&fft.activePartitionCount()==3);
 for(int n=0;n<static_cast<int>(sparse.size())+140;++n){
  const float got=fft.processSample(n==0?1.f:0.f);
  const float expected=(n>=128&&n-128<static_cast<int>(sparse.size()))?sparse[n-128]:0.f;
  assert(std::fabs(got-expected)<.00015f);
 }
 // Repeated input impulses must survive many full-history wraparounds,
 // including across hundreds of pruned IR partitions.
 fft.reset();
 for(int n=0;n<static_cast<int>(sparse.size())*3;++n) {
  const float signal=(n==0?1.f:0.f)+(n==17?-.3f:0.f)
                    +(n==211?.6f:0.f)+(n==1270?-.4f:0.f);
  const float got=fft.processSample(signal);
  float expected=0.f;
  for(const int pulse:{0,17,211,1270}){
   const int k=n-128-pulse;
   const float amplitude=pulse==0?1.f:(pulse==17?-.3f:(pulse==211?.6f:-.4f));
   if(k>=0&&k<static_cast<int>(sparse.size()))expected+=amplitude*sparse[k];
  }
  assert(std::isfinite(got)&&std::fabs(got-expected)<.0002f);
 }
 // A silent impulse still produces a valid delayed zero response.
 assert(fft.prepare(std::vector<float>(1024,0.f)));
 assert(fft.ready()&&fft.partitionCount()==8&&fft.activePartitionCount()==0);
 for(int n=0;n<300;++n)assert(std::fabs(fft.processSample(n==0?1.f:0.f))<1e-6f);
 std::cout<<"Partitioned IR FFT reference PASS\n";
}
