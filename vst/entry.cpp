#include "processor.h"
#include "controller.h"
#include "ids.h"
#include "public.sdk/source/main/pluginfactory.h"
using namespace Steinberg;
using namespace Steinberg::Vst;
BEGIN_FACTORY_DEF("125A Systems","https://github.com/challanger2000","")
 DEF_CLASS2(INLINE_UID_FROM_FUID(kVMProcessorUID),PClassInfo::kManyInstances,kVstAudioEffectClass,
 "125A VoiceMorph",Vst::kDistributable,"Fx|Dynamics","0.1.0",kVstVersionString,VMProcessor::createInstance)
 DEF_CLASS2(INLINE_UID_FROM_FUID(kVMControllerUID),PClassInfo::kManyInstances,kVstComponentControllerClass,
 "125A VoiceMorph Controller",0,"","0.1.0",kVstVersionString,VMController::createInstance)
END_FACTORY
