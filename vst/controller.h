#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
namespace Steinberg {
class VMController final:public Vst::EditControllerEx1 {
public:
 static FUnknown* createInstance(void*) {return static_cast<Vst::IEditController*>(new VMController);}
 tresult PLUGIN_API initialize(FUnknown*) SMTG_OVERRIDE;
 tresult PLUGIN_API setComponentState(IBStream*) SMTG_OVERRIDE;
 tresult PLUGIN_API getParamStringByValue(Vst::ParamID,Vst::ParamValue,Vst::String128) SMTG_OVERRIDE;
};
}
