#include "controller.h"
#include "ids.h"
#include "MeterUnits.h"
#include "IRLoadInfo.h"
#include "VoiceParameterText.h"
#include "pluginterfaces/base/ustring.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "vstgui/lib/cfileselector.h"
#include "vstgui/lib/controls/cbuttons.h"
#include "vstgui/lib/cframe.h"
#include <cstring>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <string>
namespace Steinberg {
tresult VMController::clearIRFile(bool microphone){
 auto message=owned(allocateMessage());
 if(!message)return kResultFalse;
 message->setMessageID("125A.ClearIR");
 auto* attributes=message->getAttributes();
 if(!attributes||attributes->setInt("microphone",microphone?1:0)!=kResultOk)
  return kResultFalse;
 return sendMessage(message);
}
tresult VMController::requestIRFile(const std::string& utf8Path,bool microphone){
 if(!vm::validIRPath(utf8Path))return kInvalidArgument;
 auto message=owned(allocateMessage());
 if(!message)return kResultFalse;
 message->setMessageID("125A.LoadIR");
 auto* attributes=message->getAttributes();
 if(!attributes)return kResultFalse;
 if(attributes->setInt("microphone",microphone?1:0)!=kResultOk||
    attributes->setBinary("utf8Path",utf8Path.data(),static_cast<uint32>(utf8Path.size()))!=kResultOk)
  return kResultFalse;
 return sendMessage(message);
}

namespace {
class IRLoaderButtons final:public VSTGUI::IController {
 Steinberg::VMController* owner;
 VSTGUI::VST3Editor* editor;
public:
 IRLoaderButtons(Steinberg::VMController* controller,VSTGUI::VST3Editor* e):owner(controller),editor(e){}
 void valueChanged(VSTGUI::CControl* control) override {
  if(!control||control->getValueNormalized()<=.5f)return;
  const int tag=control->getTag();
  if(tag==1002||tag==1003){owner->clearIRFile(tag==1002);return;}
  if(tag!=1000&&tag!=1001)return;
  auto* frame=editor?editor->getFrame():nullptr;
  if(!frame)return;
  auto* chooser=VSTGUI::CNewFileSelector::create(frame,VSTGUI::CNewFileSelector::kSelectFile);
  if(!chooser)return;
  chooser->setTitle(tag==1000?"Load Microphone IR WAV":"Load Reverb IR WAV");
  chooser->addFileExtension(VSTGUI::CFileExtension("Wave Audio","wav","audio/wav"));
  chooser->setDefaultExtension(VSTGUI::CFileExtension("Wave Audio","wav","audio/wav"));
  if(chooser->runModal()&&chooser->getNumSelectedFiles()>0){
   const auto* selected=chooser->getSelectedFile(0);
   if(selected)owner->requestIRFile(selected,tag==1000);
  }
  chooser->forget();
 }
};
}
IPlugView* PLUGIN_API VMController::createView(FIDString name){
 if(name&&std::strcmp(name,Vst::ViewType::kEditor)==0){
  auto* editor=new VSTGUI::VST3Editor(this,"view","voicemorph.uidesc");
  editor->setAllowedZoomFactors({1.,1.5});
  return editor;
 }
 return nullptr;
}
VSTGUI::IController* VMController::createSubController(VSTGUI::UTF8StringPtr name,
 const VSTGUI::IUIDescription*,VSTGUI::VST3Editor* editor){
 if(name&&std::strcmp(name,"IRLoader")==0)return new IRLoaderButtons(this,editor);
 return nullptr;
}
tresult PLUGIN_API VMController::initialize(FUnknown* context){
 auto r=EditControllerEx1::initialize(context);if(r!=kResultOk)return r;
 // The physical units are implemented symmetrically in VoiceParameterText.h.
 parameters.addParameter(STR16("Input Gain"),STR16("dB"),0,.5,Vst::ParameterInfo::kCanAutomate,kInput);
 parameters.addParameter(STR16("Output Gain"),STR16("dB"),0,.5,Vst::ParameterInfo::kCanAutomate,kOutput);
 parameters.addParameter(STR16("De-Nasal"),nullptr,0,0,Vst::ParameterInfo::kCanAutomate,kDeNasal);
 parameters.addParameter(STR16("Chest"),nullptr,0,0,Vst::ParameterInfo::kCanAutomate,kChest);
 parameters.addParameter(STR16("Warmth"),nullptr,0,0,Vst::ParameterInfo::kCanAutomate,kWarmth);
 parameters.addParameter(STR16("Grit"),nullptr,0,0,Vst::ParameterInfo::kCanAutomate,kGrit);
 parameters.addParameter(STR16("Presence"),nullptr,0,0,Vst::ParameterInfo::kCanAutomate,kPresence);
 parameters.addParameter(STR16("Compression"),nullptr,0,0,Vst::ParameterInfo::kCanAutomate,kCompression);
 parameters.addParameter(STR16("Low Cut"),nullptr,0,0,Vst::ParameterInfo::kCanAutomate,kLowCut);
 parameters.addParameter(STR16("Bypass"),nullptr,1,0,Vst::ParameterInfo::kCanAutomate|Vst::ParameterInfo::kIsBypass,kBypass);
 parameters.addParameter(STR16("Mic IR Mix"),STR16("%"),0,1,Vst::ParameterInfo::kCanAutomate,kMicMix);
 parameters.addParameter(STR16("Mic IR Level"),nullptr,0,.25,Vst::ParameterInfo::kCanAutomate,kMicLevel);
 parameters.addParameter(STR16("Reverb IR Mix"),STR16("%"),0,.3,Vst::ParameterInfo::kCanAutomate,kReverbMix);
 parameters.addParameter(STR16("Reverb IR Level"),nullptr,0,.25,Vst::ParameterInfo::kCanAutomate,kReverbLevel);
 parameters.addParameter(STR16("Mic IR Position"),nullptr,1,1,Vst::ParameterInfo::kCanAutomate,kMicPosition);
 parameters.addParameter(STR16("Mic IR Predelay"),STR16("ms"),0,0,Vst::ParameterInfo::kCanAutomate,kMicPredelay);
 parameters.addParameter(STR16("Reverb IR Predelay"),STR16("ms"),0,0,Vst::ParameterInfo::kCanAutomate,kReverbPredelay);
 // Bipolar controls: at their actual defaults, Pitch 0 st and Formant 0 st.
 parameters.addParameter(STR16("Pitch"),STR16("st"),0,2./3.,
   Vst::ParameterInfo::kCanAutomate,kPitch);
 parameters.addParameter(STR16("Formant"),STR16("st"),0,4./7.,
   Vst::ParameterInfo::kCanAutomate,kFormant);
 parameters.addParameter(STR16("Input Peak L"),nullptr,0,0,Vst::ParameterInfo::kIsReadOnly,kInputPeakL);
 parameters.addParameter(STR16("Input Peak R"),nullptr,0,0,Vst::ParameterInfo::kIsReadOnly,kInputPeakR);
 parameters.addParameter(STR16("Output Peak L"),nullptr,0,0,Vst::ParameterInfo::kIsReadOnly,kOutputPeakL);
 parameters.addParameter(STR16("Output Peak R"),nullptr,0,0,Vst::ParameterInfo::kIsReadOnly,kOutputPeakR);
 parameters.addParameter(STR16("Mic IR Status"),nullptr,0,0,Vst::ParameterInfo::kIsReadOnly,kMicIRStatus);
 parameters.addParameter(STR16("Reverb IR Status"),nullptr,0,0,Vst::ParameterInfo::kIsReadOnly,kReverbIRStatus);
 parameters.addParameter(STR16("Mic IR Error"),nullptr,0,0,Vst::ParameterInfo::kIsReadOnly,kMicIRError);
 parameters.addParameter(STR16("Reverb IR Error"),nullptr,0,0,Vst::ParameterInfo::kIsReadOnly,kReverbIRError);
 return kResultOk;
}
tresult PLUGIN_API VMController::setComponentState(IBStream* stream){
 if(!stream)return kResultFalse;
 IBStreamer io(stream,kLittleEndian);int32 version=0;
 if(!io.readInt32(version)||(version<1||version>6))return kResultFalse;
 const int count=version==1?10:(version==2?14:(version==3?15:(version<=5?17:kParameterCount)));
 // Fully validate before modifying the visible controller state. Older
 // projects must reset new Pitch/Formant controls (and missing IR params)
 // rather than retaining values from the previously opened project.
 float restored[kParameterCount]={.5f,.5f,0,0,0,0,0,0,0,0,1,.25f,.3f,.25f,1.f,0.f,0.f,2.f/3.f,4.f/7.f};
 for(int id=0;id<count;++id){
  float value=0;
  if(!io.readFloat(value)||!std::isfinite(value))return kResultFalse;
  restored[id]=std::clamp(value,0.f,1.f);
 }
 for(int id=0;id<kParameterCount;++id)
  setParamNormalized(kInput+id,restored[id]);
 return kResultOk;
}
}

namespace Steinberg {
tresult PLUGIN_API VMController::getParamStringByValue(Vst::ParamID id,Vst::ParamValue value,Vst::String128 string) {
 if(!string)return kResultFalse;
 if(vm::isVoiceControl(static_cast<int>(id))){
  char display[48]{};
  if(!vm::formatVoiceControl(static_cast<int>(id),value,display,sizeof(display)))
   return kResultFalse;
  UString(string,128).fromAscii(display);
  return kResultTrue;
 }
 if(id==kMicIRError||id==kReverbIRError){
  if(!std::isfinite(value))value=0.;
  const int code=std::clamp(static_cast<int>(std::lround(value*5.)),0,5);
  UString(string,128).fromAscii(vm::irErrorLabel(code));
  return kResultTrue;
 }
 if(id==kMicIRStatus||id==kReverbIRStatus) {
  if(!std::isfinite(value))value=0.;
  const int idx=std::clamp(static_cast<int>(std::lround(value*3.)),0,3);
  const char* labels[]={"EMPTY","LOADING","READY","ERROR"};
  UString(string,128).fromAscii(labels[idx]);
  return kResultTrue;
 }
 if(id>=kInputPeakL&&id<=kOutputPeakR) {
  if(!std::isfinite(value))value=0.;
  char display[32];
  const double db=vm::meterToDbfs(value);
  if(db<=-120.)std::snprintf(display,sizeof(display),"-inf dBFS");
  else std::snprintf(display,sizeof(display),"%.1f dBFS",db);
  UString(string,128).fromAscii(display);
  return kResultTrue;
 }
 return EditControllerEx1::getParamStringByValue(id,value,string);
}
}

namespace Steinberg {
tresult PLUGIN_API VMController::getParamValueByString(
 Vst::ParamID id,TChar* string,Vst::ParamValue& normalized) {
 if(!vm::isVoiceControl(static_cast<int>(id)))
  return EditControllerEx1::getParamValueByString(id,string,normalized);
 if(!string)return kResultFalse;
 char text[128]{};
 UString(string,-1).toAscii(text,static_cast<int32>(sizeof(text)));
 double parsed=0.;
 if(!vm::parseVoiceControl(static_cast<int>(id),text,parsed))return kResultFalse;
 normalized=parsed;
 return kResultTrue;
}
}
