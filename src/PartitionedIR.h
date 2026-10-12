#pragma once
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <vector>
namespace vm {
// Uniform partitioned overlap-add convolution, 128-sample partitions.
// IR preparation/FFT allocation are strictly non-realtime.
// processSample() does bounded, allocation-free work; latency exactly 128 samples.
// This is a first FFT reference engine and must still undergo profiling / host integration.
class PartitionedIR {
 static constexpr std::size_t B=128,N=256;
 using C=std::complex<float>;
 static void transform(std::vector<C>& a,bool inverse) noexcept {
  for(std::size_t i=1,j=0;i<N;++i){
   std::size_t bit=N>>1;
   for(;j&bit;bit>>=1)j^=bit;
   j^=bit;
   if(i<j)std::swap(a[i],a[j]);
  }
  for(std::size_t len=2;len<=N;len<<=1){
   float ang=(inverse?2.f:-2.f)*3.14159265358979323846f/static_cast<float>(len);
   C wlen(std::cos(ang),std::sin(ang));
   for(std::size_t i=0;i<N;i+=len){
    C w(1,0);
    for(std::size_t j=0;j<len/2;++j){
     C u=a[i+j],v=a[i+j+len/2]*w;
     a[i+j]=u+v;a[i+j+len/2]=u-v;w*=wlen;
    }
   }
  }
  if(inverse)for(auto& v:a)v/=static_cast<float>(N);
 }
 std::vector<std::vector<C>> H_,history_;
 std::vector<C> scratch_,freq_;
 std::vector<float> overlap_,incoming_,out_;
 std::size_t head_=0,fill_=0,read_=0;
public:
 static constexpr std::size_t latencySamples() noexcept{return B;}
 bool prepare(const std::vector<float>& impulse,std::size_t maximum=48000u*30u){
  if(impulse.empty()||impulse.size()>maximum)return false;
  for(float x:impulse)if(!std::isfinite(x))return false;
  auto partitions=(impulse.size()+B-1)/B;
  std::vector<std::vector<C>> nextH(partitions,std::vector<C>(N));
  for(std::size_t p=0;p<partitions;++p){
   for(std::size_t i=0;i<B&&p*B+i<impulse.size();++i)nextH[p][i]=C(impulse[p*B+i],0);
   transform(nextH[p],false);
  }
  std::vector<std::vector<C>> nextHistory(partitions,std::vector<C>(N));
  H_.swap(nextH);history_.swap(nextHistory);
  scratch_.assign(N,C{});freq_.assign(N,C{});
  overlap_.assign(B,0.f);incoming_.assign(B,0.f);out_.assign(B,0.f);
  head_=fill_=read_=0;
  return true;
 }
 bool ready()const noexcept{return !H_.empty();}
 void reset() noexcept {
  for(auto& h:history_)std::fill(h.begin(),h.end(),C{});
  std::fill(overlap_.begin(),overlap_.end(),0.f);
  std::fill(incoming_.begin(),incoming_.end(),0.f);
  std::fill(out_.begin(),out_.end(),0.f);
  head_=fill_=read_=0;
 }
 float processSample(float sample) noexcept {
  if(!ready())return sample;
  float output=out_[read_];
  read_=(read_+1)%B;
  incoming_[fill_++]=std::isfinite(sample)?sample:0.f;
  if(fill_==B){
   std::fill(scratch_.begin(),scratch_.end(),C{});
   for(std::size_t i=0;i<B;++i)scratch_[i]=C(incoming_[i],0);
   transform(scratch_,false);
   history_[head_]=scratch_;
   std::fill(freq_.begin(),freq_.end(),C{});
   for(std::size_t p=0;p<H_.size();++p){
    auto& h=history_[(head_+H_.size()-p)%H_.size()];
    for(std::size_t k=0;k<N;++k)freq_[k]+=H_[p][k]*h[k];
   }
   transform(freq_,true);
   for(std::size_t i=0;i<B;++i){
    out_[i]=freq_[i].real()+overlap_[i];
    overlap_[i]=freq_[i+B].real();
   }
   head_=(head_+1)%H_.size();fill_=0;
  }
  return std::isfinite(output)?output:0.f;
 }
};
}
