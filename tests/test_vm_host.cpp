// Loads the actual compiled module. No inclusion of Processor/Controller classes.
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/common/memorystream.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/vstspeaker.h"
#include "base/source/fobject.h"
#include "IRStateBlob.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <vector>
#ifdef _WIN32
#include <objbase.h>
#endif
using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace VST3::Hosting;
#define CHECK(x) do {if(!(x))throw std::runtime_error(std::string(#x)+" line "+std::to_string(__LINE__));}while(false)
using Bytes=std::vector<char>;
static void u32(Bytes& b,std::uint32_t value){for(int i=0;i<4;++i)b.push_back(static_cast<char>(value>>(8*i)));}
static void f32(Bytes& b,float value){std::uint32_t v;std::memcpy(&v,&value,4);u32(b,v);}
static std::array<float,23> defaults(){return {.5f,.5f,0,0,0,0,0,0,0,0,1,.25f,.3f,.25f,0,0,0,2.f/3.f,4.f/7.f,0,1,.25f,0};}
static Bytes wav(int rate,float amplitude){
 Bytes b{'R','I','F','F'};u32(b,36+2048*4);b.insert(b.end(),{'W','A','V','E','f','m','t',' '});u32(b,16);
 b.insert(b.end(),{3,0,1,0});u32(b,rate);u32(b,rate*4);b.insert(b.end(),{4,0,32,0,'d','a','t','a'});u32(b,2048*4);
 for(int n=0;n<2048;++n)f32(b,n==0?amplitude:(n==401?amplitude*.2f:0.f));return b;
}
static Bytes state(const std::array<float,23>& params,int rate,bool ir=true){
 Bytes b;u32(b,8);for(float v:params)f32(b,v);u32(b,0);u32(b,0);
 auto blob=[&](float gain){
  if(!ir){u32(b,0);u32(b,0);return;}
  const auto w=wav(rate,gain);u32(b,static_cast<std::uint32_t>(w.size()));
  u32(b,vm::irCRC32(reinterpret_cast<const std::uint8_t*>(w.data()),w.size()));b.insert(b.end(),w.begin(),w.end());
 };
 blob(.7f);blob(.5f);u32(b,0);blob(.8f);return b;
}
class Handler final:public FObject,public IComponentHandler {
public:
 ParameterChanges changes{23};int begin=0,perform=0,end=0;
 tresult PLUGIN_API beginEdit(ParamID) override {++begin;return kResultOk;}
 tresult PLUGIN_API performEdit(ParamID id,ParamValue value) override {
  ++perform;int32 index=0,point=0;auto* q=changes.addParameterData(id,index);
  CHECK(q&&q->addPoint(0,value,point)==kResultOk);return kResultOk;
 }
 tresult PLUGIN_API endEdit(ParamID) override {++end;return kResultOk;}
 tresult PLUGIN_API restartComponent(int32) override {return kResultOk;}
 OBJ_METHODS(Handler,FObject)
 DEFINE_INTERFACES DEF_INTERFACE(IComponentHandler) END_DEFINE_INTERFACES(FObject)
 REFCOUNT_METHODS(FObject)
};
struct Plugin {
 HostApplication host;
 Module::Ptr module;
 IPtr<IComponent> component;
 IPtr<IEditController> controller;
 FUnknownPtr<IAudioProcessor> processor;
 FUnknownPtr<IConnectionPoint> cp,cc;
 IPtr<Handler> handler=owned(new Handler);
 bool active=false;
 explicit Plugin(const char* path){
  std::string error;module=Module::create(path,error);
  if(!module)throw std::runtime_error(std::string("Cannot load VST3 ")+path+": "+error);
  auto factory=module->getFactory();factory.setHostContext(&host);
  for(const auto& info:factory.classInfos())if(info.category()==kVstAudioEffectClass){CHECK(info.ID()==VST3::UID(0xFB9F1301,0x5F2946D1,0xA1F90017,0x0B11A009));CHECK(info.name()=="125A VoiceMorph");component=factory.createInstance<IComponent>(info.ID());break;}
  CHECK(component&&component->initialize(&host)==kResultOk);
  processor=FUnknownPtr<IAudioProcessor>(component.get());CHECK(processor);
  TUID cid{};CHECK(component->getControllerClassId(cid)==kResultOk);CHECK(VST3::UID(cid)==VST3::UID(0xFB9F1302,0x5F2946D1,0xA1F90017,0x0B11A009));
  controller=factory.createInstance<IEditController>(VST3::UID(cid));CHECK(controller);
  CHECK(controller->initialize(&host)==kResultOk);
  cp=FUnknownPtr<IConnectionPoint>(component.get());cc=FUnknownPtr<IConnectionPoint>(controller.get());
  CHECK(cp&&cc&&cp->connect(cc)==kResultOk&&cc->connect(cp)==kResultOk);
  CHECK(controller->setComponentHandler(handler)==kResultOk);
 }
 ~Plugin(){
  if(active){processor->setProcessing(false);component->setActive(false);}
  controller->setComponentHandler(nullptr);cc->disconnect(cp);cp->disconnect(cc);
  controller->terminate();component->terminate();
 }
 void setup(int rate,int format,int inputs=2,int outputs=2){
  CHECK(!active);SpeakerArrangement in=inputs==1?SpeakerArr::kMono:SpeakerArr::kStereo;
  SpeakerArrangement out=outputs==1?SpeakerArr::kMono:SpeakerArr::kStereo;
  CHECK(processor->setBusArrangements(&in,1,&out,1)==kResultOk);
  ProcessSetup setup{kOffline,format,512,double(rate)};CHECK(processor->setupProcessing(setup)==kResultOk);
 }
 void start(){CHECK(component->setActive(true)==kResultOk);CHECK(processor->setProcessing(true)==kResultOk);active=true;}
 void stop(){CHECK(processor->setProcessing(false)==kResultOk);CHECK(component->setActive(false)==kResultOk);active=false;}
 tresult restore(const Bytes& bytes){MemoryStream stream(const_cast<char*>(bytes.data()),bytes.size());return component->setState(&stream);}
 Bytes save(){MemoryStream stream;CHECK(component->getState(&stream)==kResultOk);const auto* b=static_cast<const char*>(stream.getData());return {b,b+stream.getSize()};}
 template<class Sample> std::vector<Sample> render(int rate,int block,int frames,int inputChannels=2,int outputChannels=2,bool impulse=false){
  std::array<std::array<Sample,512>,2> input{},output{};
  Sample* in[]={input[0].data(),input[1].data()};Sample* out[]={output[0].data(),output[1].data()};
  AudioBusBuffers ib{},ob{};ib.numChannels=inputChannels;ob.numChannels=outputChannels;
  ProcessData data{};data.processMode=kOffline;data.symbolicSampleSize=sizeof(Sample)==8?kSample64:kSample32;
  data.numInputs=data.numOutputs=1;data.inputs=&ib;data.outputs=&ob;
  if constexpr(sizeof(Sample)==8){ib.channelBuffers64=in;ob.channelBuffers64=out;}
  else{ib.channelBuffers32=in;ob.channelBuffers32=out;}
  std::vector<Sample> result(frames*outputChannels);
  for(int base=0;base<frames;base+=block){
   data.numSamples=std::min(block,frames-base);
   for(int n=0;n<data.numSamples;++n){
    const int t=base+n;const auto signal=impulse?(t==0?.2:0.):.14*std::sin(2*3.141592653589793*173.*t/rate)+.04*std::sin(2*3.141592653589793*901.*t/rate);
    input[0][n]=static_cast<Sample>(signal);input[1][n]=static_cast<Sample>(signal*.6);
   }
   data.inputParameterChanges=base==0?&handler->changes:nullptr;
   CHECK(processor->process(data)==kResultOk);
   for(int n=0;n<data.numSamples;++n)for(int c=0;c<outputChannels;++c){CHECK(std::isfinite(output[c][n]));result[(base+n)*outputChannels+c]=output[c][n];}
  }
  handler->changes.clearQueue();return result;
 }
};
static void recall(const char* path){
 for(int rate:{44100,48000})for(int format:{kSample32,kSample64}){
  auto p=defaults();p[2]=.25f;p[3]=.42f;p[4]=.35f;p[6]=.23f;p[7]=.32f;p[8]=.08f;p[18]=.5f;p[16]=.005f;
  const auto original=state(p,rate);
  Plugin a(path);a.setup(rate,format);CHECK(a.restore(original)==kResultOk);CHECK(a.save()==original);a.start();
  if(format==kSample32){
   const auto reference=a.render<float>(rate,128,rate/2);
   Plugin b(path);CHECK(b.restore(original)==kResultOk);b.setup(rate,format);b.start();
   const auto restored=b.render<float>(rate,31,rate/2);CHECK(reference==restored);
  }else{
   const auto reference=a.render<double>(rate,128,rate/2);
   Plugin b(path);CHECK(b.restore(original)==kResultOk);b.setup(rate,format);b.start();
   const auto restored=b.render<double>(rate,31,rate/2);CHECK(reference==restored);
  }
  auto broken=original;broken.back()^=1;const auto before=a.save();CHECK(a.restore(broken)!=kResultOk);CHECK(a.save()==before);
  broken=original;broken.resize(broken.size()-9);CHECK(a.restore(broken)!=kResultOk);CHECK(a.save()==before);
  a.stop();a.setup(rate==44100?48000:44100,format);a.start();
  if(format==kSample32)(void)a.render<float>(rate==44100?48000:44100,64,4096);
  else (void)a.render<double>(rate==44100?48000:44100,64,4096);
 }
}
static void inactiveRecall(const char* path){
 // Unchanged-rate project recall on both an incompletely filled and a
 // warmed instance must be equivalent to a fresh instance, without setup.
 for(int rate:{44100,48000})for(int format:{kSample32,kSample64})for(int oldFrames:{512,8192}){
  Plugin reused(path);reused.setup(rate,format);auto previous=defaults();
  previous[2]=.35f;previous[3]=.42f;previous[4]=.35f;previous[6]=.23f;
  previous[7]=.32f;previous[8]=.08f;previous[17]=.5f;previous[18]=.5f;
  CHECK(reused.restore(state(previous,rate))==kResultOk);reused.start();
  if(format==kSample32)(void)reused.render<float>(rate,64,oldFrames);
  else (void)reused.render<double>(rate,64,oldFrames);
  reused.stop();const auto empty=state(defaults(),rate,false);
  CHECK(reused.restore(empty)==kResultOk);
  Plugin fresh(path);fresh.setup(rate,format);
  CHECK(fresh.restore(empty)==kResultOk);fresh.start();reused.start();
  if(format==kSample32){
   const auto expected=fresh.render<float>(rate,32,8192,2,2,true);
   const auto recalled=reused.render<float>(rate,32,8192,2,2,true);
   CHECK(recalled==expected);
  }else{
   const auto expected=fresh.render<double>(rate,32,8192,2,2,true);
   const auto recalled=reused.render<double>(rate,32,8192,2,2,true);
   CHECK(recalled==expected);
  }
 }
}
static void controls(const char* path){
 Plugin a(path);a.setup(48000,kSample32);
 const auto empty=state(defaults(),48000,false);CHECK(a.restore(empty)==kResultOk);a.start();
 for(ParamID id:{100u,101u,102u,103u,104u,105u,106u,107u,108u,109u,110u,111u,112u,113u,116u,117u,118u,119u,120u,121u}){
  CHECK(a.controller->setParamNormalized(id,.75)==kResultOk);
  CHECK(a.handler->beginEdit(id)==kResultOk);CHECK(a.handler->performEdit(id,.75)==kResultOk);CHECK(a.handler->endEdit(id)==kResultOk);
 }
 (void)a.render<float>(48000,64,4096);
 const auto saved=a.save();for(ParamID id:{100u,101u,102u,103u,104u,105u,106u,107u,108u,109u,110u,111u,112u,113u,116u,117u,118u,119u,120u,121u}){
  float value=0;std::memcpy(&value,saved.data()+4+(id-100)*4,4);CHECK(value==.75f);
 }
 CHECK(a.handler->begin==20&&a.handler->perform==20&&a.handler->end==20);
 CHECK(a.controller->normalizedParamToPlain(100,.5)==0.);
 CHECK(a.controller->plainParamToNormalized(117,0.)==2./3.);
 // Verify post-input-trim telemetry on the actual processor, not a helper.
 a.stop();a.setup(48000,kSample32);auto p=defaults();p[0]=1.;CHECK(a.restore(state(p,48000,false))==kResultOk);a.start();
 ParameterChanges meters(12);int32 i=0,index=0;for(ParamID id=200;id<=211;++id){auto* q=meters.addParameterData(id,i);q->addPoint(0,0,index);}meters.clearQueue();
 std::array<float,512> in{},out{};in.fill(.4f);float* inputs[]={in.data(),in.data()};float* outputs[]={out.data(),out.data()};
 AudioBusBuffers ib{},ob{};ib.numChannels=ob.numChannels=2;ib.channelBuffers32=inputs;ob.channelBuffers32=outputs;
 ProcessData data{};data.processMode=kOffline;data.symbolicSampleSize=kSample32;data.numSamples=512;data.numInputs=data.numOutputs=1;data.inputs=&ib;data.outputs=&ob;data.outputParameterChanges=&meters;
 CHECK(a.processor->process(data)==kResultOk);bool peak=false,clip=false;
 for(int n=0;n<meters.getParameterCount();++n){auto* q=meters.getParameterData(n);int32 offset;double v;CHECK(q->getPoint(0,offset,v)==kResultOk);if(q->getParameterId()==200)peak=v==1.;if(q->getParameterId()==210)clip=v==1.;}
 CHECK(peak&&clip);
}
static void stoppedClear(const char* path){
 Plugin a(path);a.setup(48000,kSample32);
 CHECK(a.restore(state(defaults(),48000))==kResultOk);
 for(int slot=0;slot<3;++slot){
  HostMessage message;message.setMessageID("125A.ClearIR");
  CHECK(message.getAttributes()->setInt("slot",slot)==kResultOk);
  CHECK(a.cp->notify(&message)==kResultOk);
 }
 // Pending or just-completed CLEAR must survive inactive rate setup.
 a.setup(44100,kSample32);
 const auto empty=state(defaults(),48000,false);bool cleared=false;
 for(int n=0;n<200;++n){if(a.save()==empty){cleared=true;break;}std::this_thread::sleep_for(std::chrono::milliseconds(2));}
 CHECK(cleared);a.start();auto signal=a.render<float>(44100,32,8192,2,2,true);
 CHECK(std::fabs(signal[a.processor->getLatencySamples()*2]-.2f)<1e-5f);
 // A failed physical load preserves the accepted project and sounding stage.
 HostMessage bad;bad.setMessageID("125A.LoadIR");
 const std::string missing="/125A-does-not-exist/invalid.wav";
 CHECK(bad.getAttributes()->setInt("slot",0)==kResultOk);
 CHECK(bad.getAttributes()->setBinary("utf8Path",missing.data(),missing.size())==kResultOk);
 CHECK(a.cp->notify(&bad)==kResultOk);
 bool error=false;
 for(int n=0;n<200;++n){
  HostMessage query;query.setMessageID("125A.QueryIRStatus");CHECK(a.cp->notify(&query)==kResultOk);
  if(a.controller->getParamNormalized(204)==1.){error=true;break;}
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
 }
 CHECK(error);CHECK(a.save()==empty);
}
static void buses(const char* path){
 for(int format:{kSample32,kSample64})for(auto shape:std::array<std::array<int,2>,3>{{{1,1},{1,2},{2,2}}}){
  Plugin a(path);a.setup(48000,format,shape[0],shape[1]);CHECK(a.restore(state(defaults(),48000,false))==kResultOk);a.start();
  if(format==kSample32){auto x=a.render<float>(48000,17,8192,shape[0],shape[1],true);CHECK(std::fabs(x[a.processor->getLatencySamples()*shape[1]]-.2f)<1e-5);}
  else{auto x=a.render<double>(48000,17,8192,shape[0],shape[1],true);CHECK(std::fabs(x[a.processor->getLatencySamples()*shape[1]]-.2)<1e-5);}
 }
}
int main(int argc,char** argv){
 if(argc!=2)return 2;
#ifdef _WIN32
 if(FAILED(CoInitialize(nullptr)))return 2;
#endif
 int result=0;try {recall(argv[1]);inactiveRecall(argv[1]);controls(argv[1]);stoppedClear(argv[1]);buses(argv[1]);std::cout<<"Actual VST3: immediate audio recall, corruption rejection, rates, 32/64-bit, all controls, metering and buses PASS\n";}
 catch(const std::exception& e){std::cerr<<e.what()<<"\n";result=1;}
#ifdef _WIN32
 CoUninitialize();
#endif
 return result;
}
