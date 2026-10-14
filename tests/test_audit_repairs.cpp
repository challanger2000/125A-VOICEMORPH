#include "ParameterPointCursor.h"
#include "VoiceDSP.h"
#include "FixedLatencyIRChain.h"
#include "IRStageHandoff.h"
#include "IRMemoryBudget.h"
#include "VoiceParameterText.h"
#include "RealtimeGuard.h"
#include <array>
#include <atomic>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>
#define CHECK(x) do {if(!(x))throw std::runtime_error(#x);}while(false)
static void automation(){
 vm::ParameterPointCursor cursor;
 auto fetch=[](int i,std::int32_t& offset,double& value){if(i)return false;offset=127;value=1.;return true;};
 cursor.reset(1,fetch,.5,true);
 for(int n=0;n<128;++n)CHECK(std::fabs(cursor.valueAt(n,fetch)-(.5+.5*(n+1)/128.))<1e-12);
 cursor.reset(1,fetch,0.,false);
 for(int n=0;n<127;++n)CHECK(cursor.valueAt(n,fetch)==0.);
 CHECK(cursor.valueAt(127,fetch)==1.);
 const std::array<int,3> offsets{0,31,127};
 const std::array<double,3> values{.25,.75,.5};
 auto points=[&](int i,std::int32_t& o,double& v){o=offsets[i];v=values[i];return true;};
 cursor.reset(3,points,0.,true);
 CHECK(cursor.valueAt(0,points)==.25);
 CHECK(std::fabs(cursor.valueAt(16,points)-(.25+.5*16/31.))<1e-12);
 CHECK(cursor.valueAt(31,points)==.75);CHECK(cursor.valueAt(127,points)==.5);
}
static void grit(){
 auto sample=[](float amount){vm::VoiceDSP fx;fx.prepare(48000);vm::Parameters p;p.grit=amount;fx.setParameters(p);return fx.processSample(.1f);};
 vm::VoiceDSP neutral,almost;neutral.prepare(48000);almost.prepare(48000);
 vm::Parameters low;low.lowCut=1e-6f;almost.setParameters(low);
 for(int n=0;n<1024;++n)CHECK(std::fabs(neutral.processSample(.1f)-almost.processSample(.1f))<2e-5f);
 const auto off=sample(0);
 CHECK(std::fabs(sample(1e-6f)-off)<1e-5f);
 for(float g:{.01f,.08f,.25f,.5f,1.f}){
  const float d=1+2*g;CHECK(sample(g)==std::tanh(.1f*d)/std::tanh(d));
 }
}
static void transfer(){
 for(unsigned target:{44100u,48000u,96000u}){
  vm::WaveIR source{48000,1,std::vector<float>(512,0)};source.interleaved[128]=1;
  vm::WaveIR result;CHECK(vm::resampleIR(source,target,result));
  double dc=0;for(float tap:result.interleaved)dc+=tap;
  CHECK(std::fabs(dc-1.)<.0002);
 }
 CHECK(vm::estimateIRMemory(48000*30,4,48000,48000)>vm::kIRStageMemoryBudget);
 CHECK(vm::estimateIRMemory(48000,4,48000,48000)<vm::kIRStageMemoryBudget);
 vm::PartitionedIR convolution;
 CHECK(!convolution.prepare(std::vector<float>(128,std::numeric_limits<float>::max())));
}
static void transitions(){
 vm::IRStageHandoff bridge;
 auto make=[](float amplitude){
  auto stage=std::make_unique<vm::StereoIRStage>();
  CHECK(stage->prepare(vm::WaveIR{48000,1,{amplitude}},48000));return stage;
 };
 auto a=make(1);CHECK(bridge.publishLatest(a));auto* live=bridge.acquireAudio();
 float l=0,r=0;
 for(int n=0;n<1024;++n)live->process(.2f,.2f,l,r);
 CHECK(std::fabs(l-.2f)<1e-5f);
 auto same=make(1);CHECK(bridge.publishLatest(same));live=bridge.acquireAudio();
 // Identical IR swaps must not insert the former 128-sample cold dropout.
 for(int n=0;n<1024;++n){live->process(.2f,.2f,l,r);CHECK(std::fabs(l-.2f)<1e-5f);}
 // A next publication is already pending when the completed old stage is
 // retired. Final reclamation must allow that pending stage to progress.
 auto b=make(.5f);CHECK(bridge.publishLatest(b));CHECK(bridge.acquireAudio()==live);
 bridge.reclaimControl();live=bridge.acquireAudio();
 for(int n=0;n<1024;++n){live->process(.2f,.2f,l,r);CHECK(std::isfinite(l)&&l>.09f&&l<.201f);}
 CHECK(std::fabs(l-.1f)<1e-5f);
 bridge.reclaimControl();
 auto clear=std::make_unique<vm::StereoIRStage>();clear->prepareEmpty();CHECK(bridge.publishLatest(clear));live=bridge.acquireAudio();
 for(int n=0;n<1024;++n){live->process(.2f,.2f,l,r);CHECK(l>.09f&&l<.201f);}
 CHECK(std::fabs(l-.2f)<1e-5f);
 bridge.reclaimControl();
}
static void controls(){
 CHECK(vm::voiceNormalizedToPlain(100,.5)==0.);
 CHECK(vm::voiceNormalizedToPlain(117,0.)==-12.);
 CHECK(vm::voicePlainToNormalized(116,1000.)==.5);
 for(int id=100;id<=122;++id){
  if(id==109||id==114||id==119)continue;
  for(double n:{0.,.25,.5,.75,1.})
   CHECK(std::fabs(vm::voicePlainToNormalized(id,vm::voiceNormalizedToPlain(id,n))-n)<1e-6);
 }
 vm::FixedLatencyIRChain chain;vm::VoiceDSP a,b;a.prepare(48000);b.prepare(48000);
 float l=0,r=0;for(int n=0;n<1024;++n)chain.process(.25f,.25f,l,r,a,b,nullptr,nullptr,true,false);
 chain.setInputOutputGains(0,12);
 chain.process(.25f,.25f,l,r,a,b,nullptr,nullptr,true,false);
 CHECK(std::fabs(l-.25f)<.01f);
 for(int n=0;n<500;++n)chain.process(.25f,.25f,l,r,a,b,nullptr,nullptr,true,false);
 CHECK(std::fabs(l-.25f*vm::gain(12))<1e-5f);
}
int main(int argc,char**){
 try {if(argc>1){CHECK(false);}automation();grit();transfer();transitions();controls();
  std::cout<<"SDK ramps, reference GRIT curve, convolution gain, warm swaps, smoothing and units PASS\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
