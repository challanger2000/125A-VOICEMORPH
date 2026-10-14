#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
#ifndef VM_WITH_GUI
#define VM_WITH_GUI 1
#endif
#if VM_WITH_GUI
#include "vstgui/plugin-bindings/vst3editor.h"
#endif
#include <string>
#include <array>
namespace Steinberg {
class VMController final:public Vst::EditControllerEx1
#if VM_WITH_GUI
,public VSTGUI::VST3EditorDelegate
#endif
{
private:
 std::array<std::string,3> irFilenames_{};
public:
 static FUnknown* createInstance(void*) {return static_cast<Vst::IEditController*>(new VMController);}
 tresult PLUGIN_API initialize(FUnknown*) SMTG_OVERRIDE;
 IPlugView* PLUGIN_API createView(FIDString name) SMTG_OVERRIDE;
#if VM_WITH_GUI
 VSTGUI::CView* verifyView(VSTGUI::CView*,const VSTGUI::UIAttributes&,
   const VSTGUI::IUIDescription*,VSTGUI::VST3Editor*) override;
 VSTGUI::IController* createSubController(VSTGUI::UTF8StringPtr name,const VSTGUI::IUIDescription*,VSTGUI::VST3Editor*) override;
#endif
 tresult PLUGIN_API notify(Vst::IMessage*) SMTG_OVERRIDE;
 tresult queryIRStatus();
 const std::string& irFilename(int slot) const;
 // UI-thread command: transmit selected WAV path to the processor.
 tresult requestIRFile(const std::string& utf8Path,int slot);
 tresult clearIRFile(int slot);
 // One-click convenience preset; uses existing automatable parameters only.
 void applyVoiceQuickPreset(bool hollywood);
 tresult PLUGIN_API setComponentState(IBStream*) SMTG_OVERRIDE;
 tresult PLUGIN_API getParamStringByValue(Vst::ParamID,Vst::ParamValue,Vst::String128) SMTG_OVERRIDE;
 tresult PLUGIN_API getParamValueByString(Vst::ParamID,Vst::TChar*,Vst::ParamValue&) SMTG_OVERRIDE;
 Vst::ParamValue PLUGIN_API normalizedParamToPlain(Vst::ParamID,Vst::ParamValue) SMTG_OVERRIDE;
 Vst::ParamValue PLUGIN_API plainParamToNormalized(Vst::ParamID,Vst::ParamValue) SMTG_OVERRIDE;
};
}
