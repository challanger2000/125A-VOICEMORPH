#include "B1Correction.h"
#include "FixedLatencyIRChain.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <initializer_list>
namespace {
double rms(vm::B1Correction& eq,int sr,double freq,bool on){
 eq.setEnabled(on);eq.prepare(sr);
 double energy=0.;
 for(int n=0;n<sr;++n){
  float l=.2f*std::sin(2*vm::pi*freq*n/sr),r=l;
  eq.process(l,r);
  assert(std::isfinite(l)&&std::isfinite(r)&&std::fabs(l-r)<1e-6f);
  if(n>sr/2)energy+=l*l;
 }
 return std::sqrt(energy/(sr/2.));
}
}
int main(){
 for(const int sr:{44100,48000,96000}){
  vm::B1Correction eq;
  const double eq2=rms(eq,sr,2000.,true),eq10=rms(eq,sr,10000.,true);
  const double off2=rms(eq,sr,2000.,false),off10=rms(eq,sr,10000.,false);
  const double db2=20.*std::log10(eq2/off2);
  const double db10=20.*std::log10(eq10/off10);
  assert(db2<0.&&db2>-1.1);
  assert(db10<-3.8&&db10>-5.5);
  const double on200=rms(eq,sr,200.,true),off200=rms(eq,sr,200.,false);
  assert(std::fabs(20.*std::log10(on200/off200))<.1);
  vm::VoiceDSP l,r;l.prepare(sr);r.prepare(sr);
  vm::FixedLatencyIRChain chain;
  chain.preparePitchLatency(0);
  eq.setEnabled(false);eq.prepare(sr);
  for(int n=0;n<500;++n){
   float ol=0,orr=0;
   chain.process(n==0?1.f:0.f,n==0?-.5f:0.f,ol,orr,
                 l,r,nullptr,nullptr,true,false,nullptr,nullptr,&eq);
   assert(std::fabs(ol-(n==384?1.f:0.f))<1e-4f);
   assert(std::fabs(orr-(n==384?-.5f:0.f))<1e-4f);
  }
 }
 std::cout<<"B1 correction 44.1/48/96 kHz, neutral OFF and 3-stage PDC PASS\n";
}
