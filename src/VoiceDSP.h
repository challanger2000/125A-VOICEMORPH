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
 Biquad nose,body,warm,air;
 float inGain=1,outGain=1,envelope=0,attack=0,release=0,hpX=0,hpY=0,hpCoeff=0;
 Meters meter{};
 static void peak(std::atomic<float>& a,float val) noexcept {
  float v=a.load(std::memory_order_relaxed);
  while(v<val&&!a.compare_exchange_weak(v,val,std::memory_order_relaxed)){}
 }
 void update() noexcept {
  inGain=gain(clamp(p.inputDb,-12,12));outGain=gain(clamp(p.outputDb,-12,12));
  nose.peak(rate,920,2.1,-7*clamp(p.denasal,0,1));
  body.peak(rate,165,.72,4*clamp(p.chest,0,1));
  warm.peak(rate,390,.85,2.5*clamp(p.warmth,0,1));
  air.peak(rate,3400,.9,3*clamp(p.presence,0,1));
  attack=std::exp(-1/(.010*rate));release=std::exp(-1/(.100*rate));hpCoeff=std::exp(-2*pi*65/rate);
 }
public:
 VoiceDSP(){update();}
 void prepare(double sr) noexcept {
  rate=(sr>=8000&&sr<=384000)?sr:48000;
  nose.reset();body.reset();warm.reset();air.reset();envelope=hpX=hpY=0;update();meter.reset();
 }
 void setParameters(const Parameters& next) noexcept {p=next;update();}
 const Meters& meters() const noexcept {return meter;}
 void resetMeters() noexcept {meter.reset();}
 float processSample(float s) noexcept {
  if(!std::isfinite(s))s=0;
  const float input=s*inGain;peak(meter.inputPeak,std::fabs(input));
  const float hp=input-hpX+hpCoeff*hpY;hpX=input;hpY=hp;
  float x=(1-clamp(p.lowCut,0,1))*input+clamp(p.lowCut,0,1)*hp;
  x=air.run(warm.run(body.run(nose.run(x))));
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
