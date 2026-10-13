#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
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
 float run(float x) noexcept {double y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;if(!std::isfinite(y)){reset();return 0;}return static_cast<float>(y);}
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
  for(int i=0;i<3;++i){nose[i].reset();nasalDetector[i].reset();detectorEnvelope[i]=0;denasalReduction[i]=0;}
  body.reset();warm.reset();air.reset();lowCutFilter.reset();envelope=hpX=hpY=fullEnvelope=0;
  detectorCounter=0;update();meter.reset();
 }
 void setParameters(const Parameters& next) noexcept {p=next;update();}
 const Meters& meters() const noexcept {return meter;}
 void resetMeters() noexcept {meter.reset();}
 float processSample(float s) noexcept {
  if(!std::isfinite(s))s=0;
  const float input=s*inGain;peak(meter.inputPeak,std::fabs(input));
  // Second-order Butterworth (12 dB/oct); 0% bypasses the filter entirely.
  float x=p.lowCut<=0.f?input:lowCutFilter.run(input);
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
  if(drive>0){float d=1+drive;x=std::tanh(x*d)/std::tanh(d);}
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
