#pragma once
#include <algorithm>
#include <array>
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
 // Roots prepared off the audio thread. A real-valued transform has
 // conjugate-symmetric spectra: multiplication only needs N/2+1 bins.
 std::array<C,N/2> roots_{};
 void transform(std::vector<C>& a,bool inverse) const noexcept {
  for(std::size_t i=1,j=0;i<N;++i){
   std::size_t bit=N>>1;
   for(;j&bit;bit>>=1)j^=bit;
   j^=bit;
   if(i<j)std::swap(a[i],a[j]);
  }
  for(std::size_t len=2;len<=N;len<<=1){
   const std::size_t stride=N/len;
   for(std::size_t i=0;i<N;i+=len){
    for(std::size_t j=0;j<len/2;++j){
     const C w=inverse?std::conj(roots_[j*stride]):roots_[j*stride];
     const C u=a[i+j],v=a[i+j+len/2]*w;
     a[i+j]=u+v;a[i+j+len/2]=u-v;
    }
   }
  }
  if(inverse)for(auto& v:a)v/=static_cast<float>(N);
 }
 std::vector<std::vector<C>> H_,history_;
 std::vector<std::size_t> partitionOffsets_;
 std::vector<C> scratch_,freq_;
 std::vector<float> overlap_,incoming_,out_;
 std::size_t head_=0,fill_=0,read_=0;
public:
 static constexpr std::size_t latencySamples() noexcept{return B;}
 bool prepare(const std::vector<float>& impulse,std::size_t maximum=48000u*30u){
  if(impulse.empty()||impulse.size()>maximum)return false;
  for(float x:impulse)if(!std::isfinite(x))return false;
  // Generate one FFT root table per convolution object in prepare().
  // No trigonometry, vector growth or thread-safe static initialization in
  // the audio callback.
  for(std::size_t k=0;k<N/2;++k){
   const float angle=-2.f*3.14159265358979323846f*
      static_cast<float>(k)/static_cast<float>(N);
   roots_[k]=C(std::cos(angle),std::sin(angle));
  }
  auto partitions=(impulse.size()+B-1)/B;
  // Keep the delay history for the entire IR but only transform and
  // multiply partitions containing nonzero coefficients. This dramatically
  // reduces work for sparse IRs and avoids storing silent frequency responses.
  std::vector<std::vector<C>> nextH;
  std::vector<std::size_t> nextOffsets;
  nextH.reserve(partitions);
  nextOffsets.reserve(partitions);
  for(std::size_t p=0;p<partitions;++p){
   const std::size_t start=p*B;
   const std::size_t end=std::min(start+B,impulse.size());
   bool nonzero=false;
   for(std::size_t i=start;i<end;++i)
    if(impulse[i]!=0.f){nonzero=true;break;}
   if(!nonzero)continue;
   std::vector<C> taps(N,C{});
   for(std::size_t i=start;i<end;++i)taps[i-start]=C(impulse[i],0);
   transform(taps,false);
   nextH.push_back(std::move(taps));
   nextOffsets.push_back(p);
  }
  std::vector<std::vector<C>> nextHistory(partitions,std::vector<C>(N));
  H_.swap(nextH);history_.swap(nextHistory);partitionOffsets_.swap(nextOffsets);
  scratch_.assign(N,C{});freq_.assign(N,C{});
  overlap_.assign(B,0.f);incoming_.assign(B,0.f);out_.assign(B,0.f);
  head_=fill_=read_=0;
  return true;
 }
 bool ready()const noexcept{return !history_.empty();}
 std::size_t activePartitionCount()const noexcept{return H_.size();}
 std::size_t partitionCount()const noexcept{return history_.size();}
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
   const auto historySize=history_.size();
   for(std::size_t i=0;i<H_.size();++i){
    const C* h=history_[(head_+historySize-partitionOffsets_[i])%historySize].data();
    const C* ir=H_[i].data();
    // Half-spectrum frequency-domain convolution for real-valued input.
    // The other half is determined by Hermitian symmetry. This cuts the
    // dominant dense-IR multiply/accumulate work roughly in half.
    for(std::size_t k=0;k<=B;++k)freq_[k]+=ir[k]*h[k];
   }
   freq_[0]=C(freq_[0].real(),0.f);
   freq_[B]=C(freq_[B].real(),0.f);
   for(std::size_t k=1;k<B;++k)freq_[N-k]=std::conj(freq_[k]);
   transform(freq_,true);
   for(std::size_t i=0;i<B;++i){
    out_[i]=freq_[i].real()+overlap_[i];
    overlap_[i]=freq_[i+B].real();
   }
   // The input history spans every IR partition, including pruned silent ones.
   // Advancing modulo H_.size() corrupts delayed sparse taps (and divides by zero
   // for an entirely silent IR). Use the full history ring instead.
   head_=(head_+1)%history_.size();fill_=0;
  }
  return std::isfinite(output)?output:0.f;
 }
};
}
