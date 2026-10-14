#pragma once
#include <algorithm>
#include <atomic>
#include <array>
#include <cmath>
#include <cstddef>
#include "SmoothValue.h"
namespace vm {
constexpr double pi=3.14159265358979323846;
inline float gain(float db) noexcept {return std::pow(10.f,db/20.f);}
inline float clamp(float x,float a,float b) noexcept {return std::max(a,std::min(b,x));}
struct Biquad {
 double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
 void reset() noexcept {z1=z2=0;}
 void peak(double sr,double hz,double q,double db) noexcept {
  double A=std::pow(10.,db/40.),w=2*pi*std::min(hz,.45*sr)/sr,alpha=std::sin(w)/(2*q),co=std::cos(w),div=1+alpha/A;
  b0=(1+alpha*A)/div;b1=-2*co/div;b2=(1-alpha*A)/div;a1=-2*co/div;a2=(1-alpha/A)/div;
 }
 void highpass(double sr,double hz) noexcept {
  const double w=2*pi*std::min(hz,.45*sr)/sr;
  const double co=std::cos(w),alpha=std::sin(w)/(2*0.7071067811865476);
  const double inv=1/(1+alpha);
  b0=(1+co)*.5*inv;b1=-(1+co)*inv;b2=b0;
  a1=-2*co*inv;a2=(1-alpha)*inv;
 }
 float run(float x) noexcept {double y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;
  if(!std::isfinite(y)){reset();return 0;}
  if(std::fabs(z1)<1e-30)z1=0;if(std::fabs(z2)<1e-30)z2=0;
  return static_cast<float>(y);}
};
struct Parameters {
 float inputDb=0,outputDb=0,denasal=0,chest=0,warmth=0,grit=0,presence=0,compression=0,lowCut=0;
};
struct Meters {
 std::atomic<float> inputPeak{0},outputPeak{0};
 void reset() noexcept {inputPeak.store(0,std::memory_order_relaxed);outputPeak.store(0,std::memory_order_relaxed);}
};
class VoiceDSP {
 double rate=48000;
 Parameters p{};
 Biquad nose[3],body,warm,air,lowCutFilter;
 Biquad nasalDetector[3];
 float inGain=1,outGain=1,envelope=0,attack=0,release=0,hpX=0,hpY=0,hpCoeff=0;
 float detectorEnvelope[3]={},fullEnvelope=0,denasalReduction[3]={};
 float detectorAttack=0,detectorRelease=0;
 Meters meter{};
 unsigned detectorCounter=0;
 Parameters target_{};
 std::array<SmoothValue,9> controls_;
 bool started_=false;
 static std::array<float,9> values(const Parameters& p) noexcept {
  return {p.inputDb,p.outputDb,p.denasal,p.chest,p.warmth,p.grit,p.presence,p.compression,p.lowCut};
 }
 static void peak(std::atomic<float>& a,float val) noexcept {
  float v=a.load(std::memory_order_relaxed);
  while(v<val&&!a.compare_exchange_weak(v,val,std::memory_order_relaxed)){}
 }
 void update() noexcept {
  inGain=gain(clamp(p.inputDb,-12,12));outGain=gain(clamp(p.outputDb,-12,12));
  // Three bounded resonances track nasal concentration, rather than a fixed notch.
  constexpr double frequencies[3]={680.,1050.,1450.};
  for(int i=0;i<3;++i){
   nose[i].peak(rate,frequencies[i],2.0,-4.5*clamp(p.denasal,0,1)*denasalReduction[i]);
   nasalDetector[i].peak(rate,frequencies[i],2.0,10.);
  }
  body.peak(rate,165,.72,4*clamp(p.chest,0,1));
  warm.peak(rate,390,.85,2.5*clamp(p.warmth,0,1));
  air.peak(rate,3400,.9,3*clamp(p.presence,0,1));
  detectorAttack=std::exp(-1/(.025*rate));
  detectorRelease=std::exp(-1/(.140*rate));
  attack=std::exp(-1/(.010*rate));release=std::exp(-1/(.100*rate));hpCoeff=std::exp(-2*pi*(35.+165.*clamp(p.lowCut,0,1))/rate);
  lowCutFilter.highpass(rate,35.+165.*clamp(p.lowCut,0,1));
 }
public:
 VoiceDSP(){update();}
 void prepare(double sr) noexcept {
  rate=(sr>=8000&&sr<=384000)?sr:48000;
  p=target_;
  for(int i=0;i<3;++i){nose[i].reset();nasalDetector[i].reset();detectorEnvelope[i]=0;denasalReduction[i]=0;}
  body.reset();warm.reset();air.reset();lowCutFilter.reset();envelope=hpX=hpY=fullEnvelope=0;
  detectorCounter=0;update();meter.reset();
  started_=false;
  for(auto& control:controls_)control.prepare(rate);
  const auto initial=values(target_);
  for(std::size_t i=0;i<controls_.size();++i)controls_[i].reset(initial[i]);
 }
 void setParameters(const Parameters& next) noexcept {
  target_=next;const auto v=values(next);
  for(std::size_t i=0;i<controls_.size();++i){
   if(started_)controls_[i].set(v[i]);else controls_[i].reset(v[i]);
  }
  if(!started_){p=next;update();}
 }
 const Meters& meters() const noexcept {return meter;}
 void resetMeters() noexcept {meter.reset();}
 float processSample(float s) noexcept {
  started_=true;
  bool changing=false;for(const auto& control:controls_)changing|=control.moving();
  if(changing){
   auto& c=controls_;
   p={c[0].next(),c[1].next(),c[2].next(),c[3].next(),c[4].next(),
      c[5].next(),c[6].next(),c[7].next(),c[8].next()};
   update();
  }
  if(!std::isfinite(s))s=0;
  const float input=s*inGain;peak(meter.inputPeak,std::fabs(input));
  // Second-order Butterworth (12 dB/oct); 0% bypasses the filter entirely.
  // Keep the OFF filter warm and bridge its first displayed percent.
  // Otherwise even a smoothed positive value abruptly enables a 35-Hz HPF.
  const float cut=lowCutFilter.run(input);
  const float cutMix=clamp(p.lowCut*100.f,0.f,1.f);
  float x=cutMix>=1.f?cut:(cutMix<=0.f?input:input+(cut-input)*cutMix);
  // Detector compares nasal-band emphasis to broadband energy; smoothed and
  // recalculated every 128 samples to avoid audio-rate coefficient modulation.
  const float broadband=std::fabs(x);
  fullEnvelope=detectorRelease*fullEnvelope+(1-detectorRelease)*broadband;
  constexpr double frequencies[3]={680.,1050.,1450.};
  for(int i=0;i<3;++i){
   const float nasalEnergy=std::fabs(nasalDetector[i].run(x)-x);
   detectorEnvelope[i]=detectorAttack*detectorEnvelope[i]+(1-detectorAttack)*nasalEnergy;
   const float ratio=detectorEnvelope[i]/(fullEnvelope+1e-5f);
   const float activity=clamp((ratio-.16f)*1.8f,0.f,1.f);
   denasalReduction[i]+=(activity-denasalReduction[i])*.001f;
  }
  if((++detectorCounter&127u)==0u)
   for(int i=0;i<3;++i)
    nose[i].peak(rate,frequencies[i],2.0,-4.5*clamp(p.denasal,0,1)*denasalReduction[i]);
  for(auto& filter:nose)x=filter.run(x);
  x=air.run(warm.run(body.run(x)));
  const float drive=2*clamp(p.grit,0,1);
  if(drive>0){
   const float d=1+drive;
   const float saturated=std::tanh(x*d)/std::tanh(d);
   // Preserve the reference curve from 1% upward. The first displayed
   // percent bridges the former discontinuity continuously from true OFF.
   if(p.grit>=.01f)x=saturated;
   else x+=(p.grit*100.f)*(saturated-x);
  }
  float v=std::fabs(x);float coeff=v>envelope?attack:release;envelope=coeff*envelope+(1-coeff)*v;
  const float threshold=gain(-18);
  const float reduction=envelope>threshold?std::pow(threshold/envelope,.5f*clamp(p.compression,0,1)):1.f;
  float y=x*reduction*outGain;if(!std::isfinite(y))y=0;peak(meter.outputPeak,std::fabs(y));return y;
 }
 void process(const float* input,float* output,std::size_t count) noexcept {
  for(std::size_t i=0;i<count;++i)output[i]=processSample(input[i]);
 }
};
} // namespace vm
