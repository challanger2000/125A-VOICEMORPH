#pragma once
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "VoiceDSP.h"
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
 float params[kParameterCount]={.5f,.5f,0,0,0,0,0,0,0,0};
 vm::VoiceDSP dsp[2];
 void apply() noexcept;
};
}
