#pragma once
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "VoiceDSP.h"
#include "IRStage.h"
#include "IRStageHandoff.h"
#include "IRFileBuilder.h"
#include "IRStateBlob.h"
#include "IRLoadInfo.h"
#include "FixedLatencyIRChain.h"
#include "B1Correction.h"
#include "IRTailAccounting.h"
#include "PitchFormantDSP.h"
#include "ParameterStateMailbox.h"
#include <string>
#include <memory>
#include <cstdint>
#include <atomic>
#include <array>
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
 void reclaimIRsControl() noexcept {micHandoff.reclaimControl();preampHandoff.reclaimControl();reverbHandoff.reclaimControl();}
 tresult PLUGIN_API initialize(FUnknown*) SMTG_OVERRIDE;
 tresult PLUGIN_API notify(Vst::IMessage*) SMTG_OVERRIDE;
 tresult PLUGIN_API setupProcessing(Vst::ProcessSetup&) SMTG_OVERRIDE;
 tresult PLUGIN_API setProcessing(TBool state) SMTG_OVERRIDE;
 tresult PLUGIN_API canProcessSampleSize(int32) SMTG_OVERRIDE;
 tresult PLUGIN_API setBusArrangements(
  Vst::SpeakerArrangement* inputs,int32 numIns,
  Vst::SpeakerArrangement* outputs,int32 numOuts) SMTG_OVERRIDE;
 uint32 PLUGIN_API getLatencySamples() SMTG_OVERRIDE;
 uint32 PLUGIN_API getTailSamples() SMTG_OVERRIDE;
 tresult PLUGIN_API process(Vst::ProcessData&) SMTG_OVERRIDE;
 tresult PLUGIN_API getState(IBStream*) SMTG_OVERRIDE;
 tresult PLUGIN_API setState(IBStream*) SMTG_OVERRIDE;
private:
 float params[kParameterCount]={.5f,.5f,0,0,0,0,0,0,0,0,1,.25f,.3f,.25f,0.f,0.f,0.f,0.6666666667f,0.5714285714f,1.f,1.f,.25f,0.f};
 vm::ParameterStateMailbox<kParameterCount> stateMailbox;
 vm::VoiceDSP dsp[2];
 // Reserved dual IR stages. Unloaded stages are transparent, and never allocate in process().
 vm::StereoIRStage microphoneIR;
 vm::StereoIRStage preampIR;
 vm::StereoIRStage reverbIR;
 vm::IRStageHandoff micHandoff,preampHandoff,reverbHandoff;
 vm::B1Correction b1Correction;
 vm::FixedLatencyIRChain fixedIRChain;
 vm::PitchFormantDSP pitchEngine;
 std::atomic<uint32> totalLatencySamples{384};
 std::array<std::atomic<uint32>,3> irResponseFrames{};
 std::array<std::atomic<uint32>,3> lastSuccessfulIRFrames{};
 std::atomic<bool> voiceFiltersActive{false};
 float meterDisplay[4]{},meterHeld[4]{};
 double meterHoldSeconds[4]{},clipHoldSeconds[2]{};
 std::atomic<double> currentSampleRate{0.};
 std::thread irWorker;
 std::mutex irJobMutex;
 std::condition_variable irJobWake;
 bool stopIRWorker=false;
 bool hasMicJob=false,hasPreampJob=false,hasReverbJob=false;
 std::string micJobPath,preampJobPath,reverbJobPath;
 std::string micStatePath,preampStatePath,reverbStatePath;
 vm::SharedIRSource micJobSource,preampJobSource,reverbJobSource;
 vm::SharedIRSource micStateSource,preampStateSource,reverbStateSource;
 int lastIRSlot=2;
 std::atomic<int32_t> micIRStatus{0},preampIRStatus{0},reverbIRStatus{0};
 std::atomic<int32_t> micIRError{0},preampIRError{0},reverbIRError{0};
 std::uint64_t micRevision=0,preampRevision=0,reverbRevision=0;
 void irWorkerLoop();
 void queueIRJob(std::string path,int slot);
 void apply() noexcept;
};
}
