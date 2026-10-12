#include "processor.h"
#include "ids.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include <algorithm>
#include <cmath>
namespace Steinberg {
VMProcessor::VMProcessor(){setControllerClass(kVMControllerUID);}
tresult PLUGIN_API VMProcessor::initialize(FUnknown* context){
 auto r=AudioEffect::initialize(context);if(r!=kResultOk)return r;
 addAudioInput(STR16("Stereo In"),Vst::SpeakerArr::kStereo);
 addAudioOutput(STR16("Stereo Out"),Vst::SpeakerArr::kStereo);
 apply();return kResultOk;
}
void VMProcessor::apply() noexcept {
 vm::Parameters p;
 p.inputDb=(params[0]-.5f)*24.f;p.outputDb=(params[1]-.5f)*24.f;
 p.denasal=params[2];p.chest=params[3];p.warmth=params[4];p.grit=params[5];
 p.presence=params[6];p.compression=params[7];p.lowCut=params[8];
 dsp[0].setParameters(p);dsp[1].setParameters(p);
 microphoneIR.setMix(params[kMicMix-kInput]);
 microphoneIR.setWetLevel(params[kMicLevel-kInput]*4.f);
 reverbIR.setMix(params[kReverbMix-kInput]);
 reverbIR.setWetLevel(params[kReverbLevel-kInput]*4.f);
}
tresult PLUGIN_API VMProcessor::setupProcessing(Vst::ProcessSetup& setup){
 auto r=AudioEffect::setupProcessing(setup);
 if(r==kResultOk){for(auto& d:dsp)d.prepare(setup.sampleRate);microphoneIR.reset();reverbIR.reset();apply();}
 return r;
}
tresult PLUGIN_API VMProcessor::canProcessSampleSize(int32 n){
 return n==Vst::kSample32?kResultTrue:kResultFalse;
}
tresult PLUGIN_API VMProcessor::process(Vst::ProcessData& data){
 if(data.inputParameterChanges){
  auto* changes=data.inputParameterChanges;
  for(int32 i=0;i<changes->getParameterCount();++i){
   auto* q=changes->getParameterData(i);if(!q||q->getPointCount()<1)continue;
   int32 offset=0;Vst::ParamValue value=0;
   if(q->getPoint(q->getPointCount()-1,offset,value)!=kResultTrue)continue;
   auto id=q->getParameterId();
   if(id>=kInput&&id<=kReverbPredelay)params[id-kInput]=static_cast<float>(std::max(0.,std::min(1.,value)));
  }
  apply();
 }
 if(data.numSamples<=0||data.numInputs<1||data.numOutputs<1)return kResultOk;
 auto& input=data.inputs[0];auto& output=data.outputs[0];
 if(data.symbolicSampleSize!=Vst::kSample32)return kResultFalse;
 const int32 channels=std::min({input.numChannels,output.numChannels,2});
 if(channels==2 && (!input.channelBuffers32[0]||!input.channelBuffers32[1]||
                    !output.channelBuffers32[0]||!output.channelBuffers32[1]))
  return kResultFalse;
 // Snapshot dry signal before modifying any buffers (VST3 in-place processing).
 float dryPeak[2]={0.f,0.f};
 for(int32 channel=0;channel<channels;++channel){
  auto* samples=input.channelBuffers32[channel];
  if(!samples)continue;
  for(int32 i=0;i<data.numSamples;++i)
   if(std::isfinite(samples[i]))dryPeak[channel]=std::max(dryPeak[channel],std::fabs(samples[i]));
 }
 // Both prepared IR stages use latency-aligned dry/wet paths.
 // Unloaded stages are identity; global bypass remains absolute.
 if(channels==2 && params[kBypass-kInput]<.5f &&
    (microphoneIR.ready()||reverbIR.ready())) {
  for(int32 n=0;n<data.numSamples;++n){
   float l=input.channelBuffers32[0][n];
   float r=input.channelBuffers32[1][n];
   float nextL=0.f,nextR=0.f;
   const bool microphonePre=params[kMicPosition-kInput]<.5f;
   if(microphonePre&&microphoneIR.ready()){
    microphoneIR.process(l,r,nextL,nextR);l=nextL;r=nextR;
   }
   l=dsp[0].processSample(l);
   r=dsp[1].processSample(r);
   if(!microphonePre&&microphoneIR.ready()){
    microphoneIR.process(l,r,nextL,nextR);l=nextL;r=nextR;
   }
   if(reverbIR.ready()){
    reverbIR.process(l,r,nextL,nextR);l=nextL;r=nextR;
   }
   output.channelBuffers32[0][n]=l;
   output.channelBuffers32[1][n]=r;
  }
 } else {
  for(int32 ch=0;ch<channels;++ch){
   auto* src=input.channelBuffers32[ch];auto* dst=output.channelBuffers32[ch];
   if(!src||!dst)continue;
   if(params[kBypass-kInput]>=.5f){if(src!=dst)std::copy_n(src,data.numSamples,dst);}
   else dsp[ch].process(src,dst,static_cast<std::size_t>(data.numSamples));
  }
 }
 for(int32 ch=channels;ch<output.numChannels;++ch){
  if(output.channelBuffers32[ch])std::fill_n(output.channelBuffers32[ch],data.numSamples,0.f);
 }
 output.silenceFlags=0;
 // Measure final output, including bypass and optional IR processing.
 float meterOutput[2]={0.f,0.f};
 for(int32 channel=0;channel<channels;++channel) {
  auto* rendered=output.channelBuffers32[channel];
  if(!rendered)continue;
  for(int32 i=0;i<data.numSamples;++i)
   if(std::isfinite(rendered[i]))meterOutput[channel]=std::max(meterOutput[channel],std::fabs(rendered[i]));
 }
 if(data.outputParameterChanges && data.numSamples>0) {
  const Vst::ParamID meterIds[4]={kInputPeakL,kInputPeakR,kOutputPeakL,kOutputPeakR};
  for(int32 channel=0;channel<2;++channel){
   const float values[2]={params[kBypass-kInput]>=.5f?dryPeak[channel]:
    dsp[channel].meters().inputPeak.load(std::memory_order_relaxed),meterOutput[channel]};
   for(int kind=0;kind<2;++kind){
    int32 index=0;
    auto* q=data.outputParameterChanges->addParameterData(meterIds[kind*2+channel],index);
    if(q){int32 point=0;q->addPoint(data.numSamples-1,
     std::min(1.,static_cast<double>(std::max(0.f,values[kind]))),point);}
   }
  }
 }
 for(auto& channelDSP:dsp)channelDSP.resetMeters();
 return kResultOk;
}
tresult PLUGIN_API VMProcessor::getState(IBStream* stream){
 if(!stream)return kResultFalse;
 IBStreamer io(stream,kLittleEndian);
 if(!io.writeInt32(4))return kResultFalse;
 for(float p:params)if(!io.writeFloat(p))return kResultFalse;
 return kResultOk;
}
tresult PLUGIN_API VMProcessor::setState(IBStream* stream){
 if(!stream)return kResultFalse;
 IBStreamer io(stream,kLittleEndian);int32 version=0;
 if(!io.readInt32(version)||(version<1||version>4))return kResultFalse;
 float next[kParameterCount]={.5f,.5f,0,0,0,0,0,0,0,0,1,.25f,.3f,.25f,1.f,0.f,0.f};
 const int count=version==1?10:(version==2?14:(version==3?15:kParameterCount));
 for(int i=0;i<count;++i)if(!io.readFloat(next[i])||!std::isfinite(next[i]))return kResultFalse;
 for(int i=0;i<kParameterCount;++i)params[i]=vm::clamp(next[i],0,1);
 apply();return kResultOk;
}
}
