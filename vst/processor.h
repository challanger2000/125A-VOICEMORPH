#pragma once
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "VoiceDSP.h"
#include "IRStage.h"
#include "IRStageHandoff.h"
#include "IRFileBuilder.h"
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "ids.h"
namespace Steinberg {
class VMProcessor final:public Vst::AudioEffect {
public:
 static FUnknown* createInstance(void*) {return static_cast<Vst::IAudioProcessor*>(new VMProcessor);}
 VMProcessor();
 ~VMProcessor() override;
 // Non-realtime producer API; a controller/message bridge must invoke these off audio thread.
 bool publishMicIR(std::unique_ptr<vm::StereoIRStage>& stage) noexcept {return micHandoff.publishLatest(stage);}
 bool publishReverbIR(std::unique_ptr<vm::StereoIRStage>& stage) noexcept {return reverbHandoff.publishLatest(stage);}
 void reclaimIRsControl() noexcept {micHandoff.reclaimControl();reverbHandoff.reclaimControl();}
 // Non-audio-thread APIs; no GUI or host message connection yet.
 bool loadMicIRFromFileControl(const std::string& path,std::string& error);
 bool loadReverbIRFromFileControl(const std::string& path,std::string& error);
 tresult PLUGIN_API initialize(FUnknown*) SMTG_OVERRIDE;
 tresult PLUGIN_API notify(Vst::IMessage*) SMTG_OVERRIDE;
 tresult PLUGIN_API setupProcessing(Vst::ProcessSetup&) SMTG_OVERRIDE;
 tresult PLUGIN_API canProcessSampleSize(int32) SMTG_OVERRIDE;
 tresult PLUGIN_API process(Vst::ProcessData&) SMTG_OVERRIDE;
 tresult PLUGIN_API getState(IBStream*) SMTG_OVERRIDE;
 tresult PLUGIN_API setState(IBStream*) SMTG_OVERRIDE;
private:
 float params[kParameterCount]={.5f,.5f,0,0,0,0,0,0,0,0,1,.25f,.3f,.25f,1.f,0.f,0.f};
 vm::VoiceDSP dsp[2];
 // Reserved dual IR stages. Unloaded stages are transparent, and never allocate in process().
 vm::StereoIRStage microphoneIR;
 vm::StereoIRStage reverbIR;
 vm::IRStageHandoff micHandoff,reverbHandoff;
 std::atomic<double> currentSampleRate{0.};
 std::thread irWorker;
 std::mutex irJobMutex;
 std::condition_variable irJobWake;
 bool stopIRWorker=false;
 bool hasIRJob=false;
 bool irJobIsMic=false;
 std::string irJobPath;
 void irWorkerLoop();
 void queueIRJob(std::string path,bool mic);
 void apply() noexcept;
};
}
