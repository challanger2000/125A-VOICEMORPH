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
 std::cout<<"Partitioned IR FFT reference PASS\n";
}
