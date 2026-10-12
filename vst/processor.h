#pragma once
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "VoiceDSP.h"
#include "IRStage.h"
#include "ids.h"
namespace Steinberg {
class VMProcessor final:public Vst::AudioEffect {
public:
 static FUnknown* createInstance(void*) {return static_cast<Vst::IAudioProcessor*>(new VMProcessor);}
 VMProcessor();
 tresult PLUGIN_API initialize(FUnknown*) SMTG_OVERRIDE;
 tresult PLUGIN_API setupProcessing(Vst::ProcessSetup&) SMTG_OVERRIDE;
 tresult PLUGIN_API canProcessSampleSize(int32) SMTG_OVERRIDE;
 tresult PLUGIN_API process(Vst::ProcessData&) SMTG_OVERRIDE;
 tresult PLUGIN_API getState(IBStream*) SMTG_OVERRIDE;
 tresult PLUGIN_API setState(IBStream*) SMTG_OVERRIDE;
private:
 float params[kParameterCount]={.5f,.5f,0,0,0,0,0,0,0,0,1,.25f,.3f,.25f,1.f};
 vm::VoiceDSP dsp[2];
 // Reserved dual IR stages. Unloaded stages are transparent, and never allocate in process().
 vm::StereoIRStage microphoneIR;
 vm::StereoIRStage reverbIR;
 void apply() noexcept;
};
}
