// Offline sound-character comparison fixture; compile unchanged against both trees.
// Synthetic signal, not a listening test or proof for arbitrary voice/IR settings.
#include "FixedLatencyIRChain.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
int main(int argc,char** argv){
 if(argc!=3)return 2;const int rate=std::atoi(argv[1]);
 vm::VoiceDSP left,right;left.prepare(rate);right.prepare(rate);
 vm::Parameters p;p.denasal=.25f;p.chest=.42f;p.warmth=.35f;p.presence=.23f;p.compression=.32f;p.lowCut=.08f;
 left.setParameters(p);right.setParameters(p);
 vm::PitchFormantDSP pitch;if(!pitch.prepare(rate))return 3;pitch.setFormantSemitones(-1.f);
 vm::FixedLatencyIRChain chain;chain.preparePitchLatency(pitch.latencySamples());chain.setInputOutputGains(3.f,-3.f);
 vm::B1Correction correction;correction.setEnabled(true);correction.prepare(rate);
 vm::WaveIR wave{static_cast<unsigned>(rate),2,{.8f,.7f,0.f,0.f,.03f,.04f}};
 vm::StereoIRStage mic,preamp,reverb;
 if(!mic.prepare(wave,rate)||!preamp.prepare(wave,rate)||!reverb.prepare(wave,rate,0,true))return 4;
 mic.setMix(.7f);preamp.setMix(.5f);reverb.setMix(.09f);
 FILE* output=std::fopen(argv[2],"wb");if(!output)return 5;
 for(int n=0;n<rate*2;++n){
  const float x=.14f*std::sin(2*vm::pi*173.*n/rate)+.04f*std::sin(2*vm::pi*901.*n/rate);
  float l=0,r=0;chain.process(x,x*.6f,l,r,left,right,&mic,&reverb,true,false,&pitch,&preamp,&correction);
  if(std::fwrite(&l,4,1,output)!=1||std::fwrite(&r,4,1,output)!=1)return 6;
 }
 std::fclose(output);return 0;
}
