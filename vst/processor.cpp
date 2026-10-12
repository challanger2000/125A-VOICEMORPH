#include "processor.h"
#include "ids.h"
#include "IRLoadInfo.h"
#include "ParameterPointCursor.h"
#include "VoiceSampleFormat.h"
#include "VoiceBusLayout.h"
#include "pluginterfaces/vst/vstspeaker.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include <string>
#include <cstring>
#include <algorithm>
#include <array>
#include <limits>
#include <cmath>
namespace Steinberg {
VMProcessor::VMProcessor(){
 setControllerClass(kVMControllerUID);
 stateMailbox.initialize(params);
 irWorker=std::thread([this]{irWorkerLoop();});
}
VMProcessor::~VMProcessor(){
 {
  std::lock_guard<std::mutex> lock(irJobMutex);
  stopIRWorker=true;
  hasMicJob=hasReverbJob=false;
 }
 irJobWake.notify_one();
 if(irWorker.joinable())irWorker.join();
 reclaimIRsControl();
}
void VMProcessor::queueIRJob(std::string path,bool mic){
 {
  std::lock_guard<std::mutex> lock(irJobMutex);
  // Each slot coalesces independently; the latest selection is authoritative.
  if(mic){
   micJobPath=std::move(path);hasMicJob=true;++micRevision;
   micIRStatus.store(1);micIRError.store(0);
  }else{
   reverbJobPath=std::move(path);hasReverbJob=true;++reverbRevision;
   reverbIRStatus.store(1);reverbIRError.store(0);
  }
 }
 irJobWake.notify_one();
}
void VMProcessor::irWorkerLoop(){
 for(;;){
  std::string path;
  bool mic=false;
  std::uint64_t revision=0;
  {
   std::unique_lock<std::mutex> lock(irJobMutex);
   irJobWake.wait(lock,[this]{return stopIRWorker||hasMicJob||hasReverbJob;});
   if(stopIRWorker)return;
   mic=hasMicJob&&(!hasReverbJob||preferMicJob);
   if(mic){path=std::move(micJobPath);hasMicJob=false;revision=micRevision;}
   else{path=std::move(reverbJobPath);hasReverbJob=false;revision=reverbRevision;}
   preferMicJob=!mic;
  }
  const double sampleRate=currentSampleRate.load(std::memory_order_acquire);
  std::string error;
  std::unique_ptr<vm::StereoIRStage> prepared;
  if(path.empty())prepared=std::make_unique<vm::StereoIRStage>();
  else if(sampleRate>=8000.&&sampleRate<=384000.)
   prepared=vm::IRFileBuilder::load(path,sampleRate,0,error);
  {
   std::lock_guard<std::mutex> lock(irJobMutex);
   if(stopIRWorker)return;
   if(revision!=(mic?micRevision:reverbRevision))continue; // stale result
   if(!path.empty()&&sampleRate!=currentSampleRate.load(std::memory_order_acquire)){
    // Host has changed sample rate during preparation: retry with current rate.
    if(mic){micJobPath=path;hasMicJob=true;}
    else{reverbJobPath=path;hasReverbJob=true;}
    irJobWake.notify_one();
    continue;
   }
   if(prepared){
    if(mic){
     micHandoff.reclaimControl();
     micHandoff.publishLatest(prepared);
     micStatePath=path;
     micIRStatus.store(path.empty()?0:2,std::memory_order_release);
     micIRError.store(0,std::memory_order_release);
    }else{
     reverbHandoff.reclaimControl();
     reverbHandoff.publishLatest(prepared);
     reverbStatePath=path;
     reverbIRStatus.store(path.empty()?0:2,std::memory_order_release);
     reverbIRError.store(0,std::memory_order_release);
    }
   }else{
    (mic?micIRError:reverbIRError).store(
     static_cast<int32_t>(vm::identifyIRError(error)),std::memory_order_release);
    (mic?micIRStatus:reverbIRStatus).store(3,std::memory_order_release);
   }
   // Failed files retain the last valid stage and path; the GUI can retry.
  }
 }
}
tresult PLUGIN_API VMProcessor::notify(Vst::IMessage* message){
 if(!message||!message->getMessageID())return AudioEffect::notify(message);
 const bool clear=std::strcmp(message->getMessageID(),"125A.ClearIR")==0;
 if(!clear&&std::strcmp(message->getMessageID(),"125A.LoadIR")!=0)
  return AudioEffect::notify(message);
 auto* attributes=message->getAttributes();
 if(!attributes)return kInvalidArgument;
 int64 microphone=0;
 if(attributes->getInt("microphone",microphone)!=kResultOk||
    (microphone!=0&&microphone!=1))return kInvalidArgument;
 std::string path;
 if(!clear){
  const void* raw=nullptr;uint32 length=0;
  if(attributes->getBinary("utf8Path",raw,length)!=kResultOk||
     !raw||length==0||length>4096)return kInvalidArgument;
  path.assign(static_cast<const char*>(raw),static_cast<const char*>(raw)+length);
 }
 if(!vm::validIRPath(path,clear))return kInvalidArgument;
 queueIRJob(std::move(path),microphone!=0);
 return kResultOk;
}
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
 microphoneIR.setPredelayNormalized(params[kMicPredelay-kInput]);
 reverbIR.setPredelayNormalized(params[kReverbPredelay-kInput]);
 pitchEngine.setPitchSemitones(params[kPitch-kInput]*18.f-12.f);
 pitchEngine.setFormantSemitones(params[kFormant-kInput]*14.f-8.f);
}
tresult PLUGIN_API VMProcessor::setupProcessing(Vst::ProcessSetup& setup){
 auto r=AudioEffect::setupProcessing(setup);
 if(r==kResultOk){
  currentSampleRate.store(setup.sampleRate,std::memory_order_release);
  for(auto& d:dsp)d.prepare(setup.sampleRate);
  // Steinberg Validator probes arbitrary positive sample rates (including
  // 1234.568 Hz and 1234567.8 Hz). Never fail the host lifecycle merely
  // because the spectral algorithm cannot run meaningfully at that rate.
  // Disable pitch/formants at unsupported rates, preserve the IR-only path,
  // and return to normal spectral processing on the next valid setup.
  if(!pitchEngine.prepare(setup.sampleRate))pitchEngine.disable();
  fixedIRChain.preparePitchLatency(static_cast<std::size_t>(pitchEngine.latencySamples()));
  totalLatencySamples.store(static_cast<uint32>(fixedIRChain.latencySamples()),std::memory_order_release);
  microphoneIR.reset();reverbIR.reset();apply();
  // Serialize clearing and requeueing against completed background IR loads.
  // Otherwise a successful worker publication may be overwritten by an
  // out-of-order CLEAR after it finishes.
  {
   std::lock_guard<std::mutex> lock(irJobMutex);
   auto clearMic=std::make_unique<vm::StereoIRStage>();
   auto clearRev=std::make_unique<vm::StereoIRStage>();
   micHandoff.publishLatest(clearMic);
   reverbHandoff.publishLatest(clearRev);
   micJobPath=micStatePath;hasMicJob=true;++micRevision;
   reverbJobPath=reverbStatePath;hasReverbJob=true;++reverbRevision;
   micIRStatus.store(micStatePath.empty()?0:1);
   reverbIRStatus.store(reverbStatePath.empty()?0:1);
   micIRError.store(0);reverbIRError.store(0);
  }
  irJobWake.notify_one();
 }
 return r;
}
uint32 PLUGIN_API VMProcessor::getLatencySamples(){
 return totalLatencySamples.load(std::memory_order_acquire);
}
tresult PLUGIN_API VMProcessor::setBusArrangements(
 Vst::SpeakerArrangement* inputs,int32 numIns,
 Vst::SpeakerArrangement* outputs,int32 numOuts){
 if(!inputs||!outputs||numIns!=1||numOuts!=1)return kResultFalse;
 const auto channels=[](Vst::SpeakerArrangement arrangement) noexcept ->int {
  if(arrangement==Vst::SpeakerArr::kMono)return 1;
  if(arrangement==Vst::SpeakerArr::kStereo)return 2;
  return 0;
 };
 if(!vm::supportedVoiceBusLayout(channels(inputs[0]),channels(outputs[0])))
  return kResultFalse;
 return AudioEffect::setBusArrangements(inputs,numIns,outputs,numOuts);
}
tresult PLUGIN_API VMProcessor::canProcessSampleSize(int32 n){
 return (n==Vst::kSample32||n==Vst::kSample64)?kResultTrue:kResultFalse;
}
tresult PLUGIN_API VMProcessor::process(Vst::ProcessData& data){
 // Project-state restore is published off-thread as atomics; consume only
 // at the audio block boundary, never mutate the live DSP from setState().
 const bool restored=stateMailbox.consumeAudio(params);
 if(restored)apply();
 // VST3 automation points are sample-offset events. Previously only the
 // last point was applied to the *first* sample of a buffer; that breaks
 // precise pitch/formant automation, bypass and parameter ramps.
 struct ActiveParam {
  Vst::ParamID id=kInput;
  Vst::IParamValueQueue* queue=nullptr;
  vm::ParameterPointCursor cursor;
 };
 std::array<ActiveParam,kParameterCount> active{};
 int activeCount=0;
 if(data.inputParameterChanges){
  auto* changes=data.inputParameterChanges;
  for(int32 i=0;i<changes->getParameterCount();++i){
   auto* queue=changes->getParameterData(i);
   if(!queue||queue->getPointCount()<=0)continue;
   const auto id=queue->getParameterId();
   if(id<kInput||id>kFormant||activeCount>=kParameterCount)continue;
   auto& entry=active[activeCount];
   entry.id=id;entry.queue=queue;
   entry.cursor.reset(queue->getPointCount(),
    [queue](int32 index,int32& offset,double& value) noexcept {
     return queue->getPoint(index,offset,value)==kResultTrue;
    });
   if(entry.cursor.pending())++activeCount;
  }
 }
 vm::StereoIRStage* liveMic=nullptr;
 vm::StereoIRStage* liveRev=nullptr;
 auto applyAutomationAt=[&](int32 sampleOffset) noexcept {
  bool changed=false;
  for(int i=0;i<activeCount;++i){
   auto& entry=active[i];
   auto* queue=entry.queue;
   const auto id=entry.id;
   changed|=entry.cursor.consumeThrough(sampleOffset,
    [queue](int32 index,int32& offset,double& value) noexcept {
     return queue->getPoint(index,offset,value)==kResultTrue;
    },
    [&](double value) noexcept {
     const float normalized=static_cast<float>(std::clamp(value,0.,1.));
     params[id-kInput]=normalized;
     stateMailbox.storeAutomationAudio(id-kInput,normalized);
    });
  }
  if(changed){
   apply();
   // The placeholder IR stages are updated by apply(), but an externally
   // loaded IR lives in the handoff stage and needs the same timed controls.
   if(liveMic){
    liveMic->setMix(params[kMicMix-kInput]);
    liveMic->setWetLevel(params[kMicLevel-kInput]*4.f);
    liveMic->setPredelayNormalized(params[kMicPredelay-kInput]);
   }
   if(liveRev){
    liveRev->setMix(params[kReverbMix-kInput]);
    liveRev->setWetLevel(params[kReverbLevel-kInput]*4.f);
    liveRev->setPredelayNormalized(params[kReverbPredelay-kInput]);
   }
  }
 };
 // Adopt prebuilt IR engines only at audio-block boundaries; no allocations here.
 auto* micLoaded=micHandoff.acquireAudio();
 auto* reverbLoaded=reverbHandoff.acquireAudio();
 vm::StereoIRStage* mic=micLoaded?micLoaded:&microphoneIR;
 vm::StereoIRStage* rev=reverbLoaded?reverbLoaded:&reverbIR;
 liveMic=mic;liveRev=rev;
 mic->setMix(params[kMicMix-kInput]);
 mic->setWetLevel(params[kMicLevel-kInput]*4.f);
 mic->setPredelayNormalized(params[kMicPredelay-kInput]);
 rev->setMix(params[kReverbMix-kInput]);
 rev->setWetLevel(params[kReverbLevel-kInput]*4.f);
 rev->setPredelayNormalized(params[kReverbPredelay-kInput]);
 if(data.numSamples<=0){
  // Steinberg allows zero-length calls carrying parameter updates.
  applyAutomationAt(std::numeric_limits<int32>::max());
  return kResultOk;
 }
 if(data.numInputs<1||data.numOutputs<1||!data.inputs||!data.outputs)return kResultFalse;
 auto& input=data.inputs[0];auto& output=data.outputs[0];
 const bool sample64=data.symbolicSampleSize==Vst::kSample64;
 if(!sample64&&data.symbolicSampleSize!=Vst::kSample32)return kResultFalse;
 const int32 inputChannels=input.numChannels;
 const int32 outputChannels=output.numChannels;
 if(!vm::supportedVoiceBusLayout(inputChannels,outputChannels))
  return kResultFalse;
 if(sample64){
  if(!input.channelBuffers64||!output.channelBuffers64)return kResultFalse;
  for(int32 channel=0;channel<inputChannels;++channel)
   if(!input.channelBuffers64[channel])return kResultFalse;
  for(int32 channel=0;channel<outputChannels;++channel)
   if(!output.channelBuffers64[channel])return kResultFalse;
 }else{
  if(!input.channelBuffers32||!output.channelBuffers32)return kResultFalse;
  for(int32 channel=0;channel<inputChannels;++channel)
   if(!input.channelBuffers32[channel])return kResultFalse;
  for(int32 channel=0;channel<outputChannels;++channel)
   if(!output.channelBuffers32[channel])return kResultFalse;
 }
 // Both VST3 precisions use the same internal float voice/spectral/IR DSP.
 // All 64-bit host samples are checked before float conversion; huge/nonfinite
 // inputs must never overflow or contaminate the internal filter histories.
 const auto readInput=[&](int32 channel,int32 frame) noexcept ->float {
  return sample64
   ?vm::audioToFloat(input.channelBuffers64[channel][frame])
   :vm::audioToFloat(input.channelBuffers32[channel][frame]);
 };
 const auto writeOutput=[&](int32 channel,int32 frame,float value) noexcept {
  if(sample64)
   output.channelBuffers64[channel][frame]=vm::floatToAudio<double>(value);
  else
   output.channelBuffers32[channel][frame]=vm::floatToAudio<float>(value);
 };
 // Always sanitize non-finite input samples before DSP. Avoid changing input
 // buffers because the host may share them with other processors.
 // Individual VoiceDSP instances already reject non-finite input values.
 // Snapshot dry signal before modifying any buffers (VST3 in-place processing).
 float dryPeak[2]={0.f,0.f};
 for(int32 channel=0;channel<inputChannels;++channel){
  for(int32 i=0;i<data.numSamples;++i)
   dryPeak[channel]=std::max(dryPeak[channel],std::fabs(readInput(channel,i)));
 }
 // Constant 256-sample signal path with two 128-sample inserts. Disabled
 // IRs are latency-matched; bypass also remains delay-compensated.
 for(int32 n=0;n<data.numSamples;++n){
  applyAutomationAt(n);
  const bool pre=params[kMicPosition-kInput]<.5f;
  const bool bypass=params[kBypass-kInput]>=.5f;
  const float inL=readInput(0,n);
  const float inR=vm::rightVoiceInput(
   inL,inputChannels>1?readInput(1,n):0.f,inputChannels,outputChannels);
  float outL=0.f,outR=0.f;
  fixedIRChain.process(inL,inR,outL,outR,
                       dsp[0],dsp[1],mic,rev,pre,bypass,&pitchEngine);
  writeOutput(0,n,outL);
  if(outputChannels>1)writeOutput(1,n,outR);
 }
 // An event at the exact end of a block becomes the initial value of the
 // *next* block; do not apply it to already rendered samples.
 applyAutomationAt(data.numSamples);
 for(int32 ch=outputChannels;ch<output.numChannels;++ch){
  if(sample64){
   if(output.channelBuffers64[ch])
    std::fill_n(output.channelBuffers64[ch],data.numSamples,0.);
  }else if(output.channelBuffers32[ch])
   std::fill_n(output.channelBuffers32[ch],data.numSamples,0.f);
 }
 output.silenceFlags=0;
 // Measure final output, including bypass and optional IR processing.
 float meterOutput[2]={0.f,0.f};
 for(int32 channel=0;channel<outputChannels;++channel) {
  for(int32 i=0;i<data.numSamples;++i){
   const float rendered=sample64
    ?vm::audioToFloat(output.channelBuffers64[channel][i])
    :vm::audioToFloat(output.channelBuffers32[channel][i]);
   meterOutput[channel]=std::max(meterOutput[channel],std::fabs(rendered));
  }
 }
 if(data.outputParameterChanges && data.numSamples>0) {
  const Vst::ParamID meterIds[4]={kInputPeakL,kInputPeakR,kOutputPeakL,kOutputPeakR};
  for(int32 channel=0;channel<2;++channel){
   // Report physical bus input peaks consistently, irrespective of input trim, bypass or IR routing.
   const float values[2]={dryPeak[channel],meterOutput[channel]};
   for(int kind=0;kind<2;++kind){
    int32 index=0;
    auto* q=data.outputParameterChanges->addParameterData(meterIds[kind*2+channel],index);
    if(q){int32 point=0;q->addPoint(data.numSamples-1,
     std::min(1.,static_cast<double>(std::max(0.f,values[kind]))),point);}
   }
  }
  const Vst::ParamID statusIds[2]={kMicIRStatus,kReverbIRStatus};
  const double statuses[2]={micIRStatus.load(std::memory_order_acquire)/3.,
                            reverbIRStatus.load(std::memory_order_acquire)/3.};
  for(int i=0;i<2;++i){
   int32 index=0;
   auto* queue=data.outputParameterChanges->addParameterData(statusIds[i],index);
   if(queue){int32 point=0;queue->addPoint(data.numSamples-1,statuses[i],point);}
  }
  const Vst::ParamID errorIds[2]={kMicIRError,kReverbIRError};
  const double errors[2]={micIRError.load(std::memory_order_acquire)/5.,
                          reverbIRError.load(std::memory_order_acquire)/5.};
  for(int i=0;i<2;++i){
   int32 index=0;
   auto* queue=data.outputParameterChanges->addParameterData(errorIds[i],index);
   if(queue){int32 point=0;queue->addPoint(data.numSamples-1,errors[i],point);}
  }
 }
 for(auto& channelDSP:dsp)channelDSP.resetMeters();
 return kResultOk;
}
namespace {
bool writeIRPath(IBStream* stream,const std::string& path){
 int32 length=static_cast<int32>(path.size());
 if(length>4096)return false;
 int32 actual=0;
 if(stream->write(&length,sizeof(length),&actual)!=kResultOk||actual!=sizeof(length))return false;
 if(!length)return true;
 return stream->write(const_cast<char*>(path.data()),length,&actual)==kResultOk&&actual==length;
}
bool readIRPath(IBStream* stream,std::string& path){
 int32 length=0,actual=0;
 if(stream->read(&length,sizeof(length),&actual)!=kResultOk||actual!=sizeof(length)||length<0||length>4096)return false;
 std::string next(static_cast<size_t>(length),'\0');
 if(length && (stream->read(&next[0],length,&actual)!=kResultOk||actual!=length))return false;
 if(next.find('\0')!=std::string::npos)return false;
 path=std::move(next);return true;
}
}
tresult PLUGIN_API VMProcessor::getState(IBStream* stream){
 if(!stream)return kResultFalse;
 IBStreamer io(stream,kLittleEndian);
 if(!io.writeInt32(6))return kResultFalse;
 float save[kParameterCount]{};
 stateMailbox.snapshot(save);
 for(float p:save)if(!io.writeFloat(p))return kResultFalse;
 std::string mic,rev;
 {
  std::lock_guard<std::mutex> lock(irJobMutex);
  mic=micStatePath;rev=reverbStatePath;
 }
 return writeIRPath(stream,mic)&&writeIRPath(stream,rev)?kResultOk:kResultFalse;
}
tresult PLUGIN_API VMProcessor::setState(IBStream* stream){
 if(!stream)return kResultFalse;
 IBStreamer io(stream,kLittleEndian);int32 version=0;
 if(!io.readInt32(version)||(version<1||version>6))return kResultFalse;
 float next[kParameterCount]={.5f,.5f,0,0,0,0,0,0,0,0,1,.25f,.3f,.25f,1.f,0.f,0.f,0.6666666667f,0.5714285714f};
 const int count=version==1?10:(version==2?14:(version==3?15:(version<=5?17:kParameterCount)));
 for(int i=0;i<count;++i)if(!io.readFloat(next[i])||!std::isfinite(next[i]))return kResultFalse;
 std::string mic,rev;
 if(version>=5&&(!readIRPath(stream,mic)||!readIRPath(stream,rev)))return kResultFalse;
 for(int i=0;i<kParameterCount;++i)next[i]=vm::clamp(next[i],0,1);
 stateMailbox.publish(next);
 {
  std::lock_guard<std::mutex> lock(irJobMutex);
  // The IR clear must be ordered under the same lock as worker publication.
  // On an immediate successful load, the worker's prepared stage wins.
  auto clearMic=std::make_unique<vm::StereoIRStage>();
  auto clearRev=std::make_unique<vm::StereoIRStage>();
  micHandoff.publishLatest(clearMic);
  reverbHandoff.publishLatest(clearRev);
  micStatePath=mic;reverbStatePath=rev;
  micIRStatus.store(mic.empty()?0:1);reverbIRStatus.store(rev.empty()?0:1);
  micIRError.store(0);reverbIRError.store(0);
  hasMicJob=hasReverbJob=true;
  micJobPath=mic;reverbJobPath=rev;
  ++micRevision;++reverbRevision;
 }
 irJobWake.notify_one();
 return kResultOk;
}
}
