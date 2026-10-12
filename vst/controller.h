#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include <string>
namespace Steinberg {
class VMController final:public Vst::EditControllerEx1,public VSTGUI::VST3EditorDelegate {
public:
 static FUnknown* createInstance(void*) {return static_cast<Vst::IEditController*>(new VMController);}
 tresult PLUGIN_API initialize(FUnknown*) SMTG_OVERRIDE;
 IPlugView* PLUGIN_API createView(FIDString name) SMTG_OVERRIDE;
 VSTGUI::IController* createSubController(VSTGUI::UTF8StringPtr name,const VSTGUI::IUIDescription*,VSTGUI::VST3Editor*) override;
 // UI-thread command: transmit selected WAV path to the processor.
 tresult requestIRFile(const std::string& utf8Path,bool microphone);
 tresult clearIRFile(bool microphone);
 tresult PLUGIN_API setComponentState(IBStream*) SMTG_OVERRIDE;
 tresult PLUGIN_API getParamStringByValue(Vst::ParamID,Vst::ParamValue,Vst::String128) SMTG_OVERRIDE;
 tresult PLUGIN_API getParamValueByString(Vst::ParamID,TChar*,Vst::ParamValue&) SMTG_OVERRIDE;
};
}
