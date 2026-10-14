#include "controller.h"
#include "ids.h"
#include "MeterUnits.h"
#include "IRLoadInfo.h"
#include "VoiceParameterText.h"
#include "HollywoodPreset.h"
#include "pluginterfaces/base/ustring.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "vstgui/lib/cfileselector.h"
#include "vstgui/lib/controls/cbuttons.h"
#include "vstgui/lib/controls/cslider.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/controls/ctextlabel.h"
#include "vstgui/lib/cvstguitimer.h"
#include <cstring>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <string>
namespace Steinberg {
tresult VMController::queryIRStatus(){
 auto message=owned(allocateMessage());
 if(!message)return kResultFalse;
 message->setMessageID("125A.QueryIRStatus");
 return sendMessage(message);
}
const std::string& VMController::irFilename(int slot) const{
 static const std::string empty;
 return slot>=0&&slot<3?irFilenames_[slot]:empty;
}
tresult PLUGIN_API VMController::notify(Vst::IMessage* message){
 if(!message||!message->getMessageID()||
    std::strcmp(message->getMessageID(),"125A.IRStatus")!=0)
  return EditControllerEx1::notify(message);
 auto* attrs=message->getAttributes();
 if(!attrs)return kInvalidArgument;
 for(int i=0;i<3;++i){
  const std::string prefix="ir"+std::to_string(i);
  int64 status=0,error=0;
  if(attrs->getInt((prefix+"Status").c_str(),status)!=kResultOk||
     attrs->getInt((prefix+"Error").c_str(),error)!=kResultOk)return kResultFalse;
  const void* raw=nullptr;uint32 length=0;
  if(attrs->getBinary((prefix+"Name").c_str(),raw,length)==kResultOk&&raw&&length<=4096)
   irFilenames_[i].assign(static_cast<const char*>(raw),length);
  else irFilenames_[i].clear();
  const Vst::ParamID statusId[3]={kMicIRStatus,kPreampIRStatus,kReverbIRStatus};
  const Vst::ParamID errorId[3]={kMicIRError,kPreampIRError,kReverbIRError};
  setParamNormalized(statusId[i],std::clamp(status,static_cast<int64>(0),static_cast<int64>(3))/3.);
  setParamNormalized(errorId[i],std::clamp(error,static_cast<int64>(0),static_cast<int64>(5))/5.);
 }
 return kResultOk;
}
tresult VMController::clearIRFile(int slot){
 if(slot<0||slot>2)return kInvalidArgument;
 auto message=owned(allocateMessage());
 if(!message)return kResultFalse;
 message->setMessageID("125A.ClearIR");
 auto* attributes=message->getAttributes();
 if(!attributes||attributes->setInt("slot",slot)!=kResultOk)
  return kResultFalse;
 return sendMessage(message);
}
tresult VMController::requestIRFile(const std::string& utf8Path,int slot){
 if(slot<0||slot>2||!vm::validIRPath(utf8Path))return kInvalidArgument;
 auto message=owned(allocateMessage());
 if(!message)return kResultFalse;
 message->setMessageID("125A.LoadIR");
 auto* attributes=message->getAttributes();
 if(!attributes)return kResultFalse;
 if(attributes->setInt("slot",slot)!=kResultOk||
    attributes->setBinary("utf8Path",utf8Path.data(),static_cast<uint32>(utf8Path.size()))!=kResultOk)
  return kResultFalse;
 return sendMessage(message);
}

namespace {
// VSTGUI's built-in CSliderBase uses deltaX for horizontal controls.
// Ordinary mouse wheels send deltaY; preserve the existing horizontal
// gesture and map vertical wheel movement only when deltaX is absent.
// Every existing slider parameter ID, visual state and editing gesture
// remains unchanged. This is a GUI-only adjustment.
class IRFilenameLabel final : public VSTGUI::CTextLabel {
 Steinberg::VMController* owner_;
 int slot_;
 VSTGUI::CVSTGUITimer* timer_;
public:
 IRFilenameLabel(const VSTGUI::CTextLabel& source,
                 Steinberg::VMController* owner,int slot)
  :CTextLabel(source),owner_(owner),slot_(slot),timer_(nullptr){
  setTextTruncateMode(kTruncateHead);
  timer_=new VSTGUI::CVSTGUITimer([this](VSTGUI::CVSTGUITimer*){
   if(!owner_)return;
   owner_->queryIRStatus();
   const auto& path=owner_->irFilename(slot_);
   auto p=path.find_last_of("/\\\\");
   const std::string shown=path.empty()?"NO IR":path.substr(p==std::string::npos?0:p+1);
   setText(shown.c_str());
  },250);
 }
 ~IRFilenameLabel() noexcept override {
  if(timer_){timer_->stop();timer_->forget();}
 }
};
class VoiceActiveLabel final : public VSTGUI::CTextLabel {
 Steinberg::VMController* owner_;
 VSTGUI::CVSTGUITimer* timer_;
public:
 VoiceActiveLabel(const VSTGUI::CTextLabel& source, Steinberg::VMController* owner)
  :CTextLabel(source),owner_(owner),timer_(nullptr){
  setText("ON");
  timer_=new VSTGUI::CVSTGUITimer([this](VSTGUI::CVSTGUITimer*){
   if(owner_)setText(owner_->getParamNormalized(Steinberg::kBypass)>=.5?"OFF":"ON");
  },100);
 }
 ~VoiceActiveLabel() noexcept override {
  if(timer_){timer_->stop();timer_->forget();}
 }
};
class VoiceWheelSlider final : public VSTGUI::CSlider {
public:
 explicit VoiceWheelSlider(const VSTGUI::CSlider& source):CSlider(source){}
 void onMouseWheelEvent(VSTGUI::MouseWheelEvent& event) override {
  if(event.deltaX==0. && event.deltaY!=0.)event.deltaX=event.deltaY;
  CSlider::onMouseWheelEvent(event);
 }
};
class VoicePresetButtons final:public VSTGUI::IController {
 Steinberg::VMController* owner;
public:
 explicit VoicePresetButtons(Steinberg::VMController* controller):owner(controller){}
 void valueChanged(VSTGUI::CControl* control) override {
  if(!control||control->getValueNormalized()<=.5f)return;
  if(control->getTag()==1010)owner->applyVoiceQuickPreset(true);
  else if(control->getTag()==1011)owner->applyVoiceQuickPreset(false);
 }
};
class IRLoaderButtons final:public VSTGUI::IController {
 Steinberg::VMController* owner;
 VSTGUI::VST3Editor* editor;
public:
 IRLoaderButtons(Steinberg::VMController* controller,VSTGUI::VST3Editor* e):owner(controller),editor(e){}
 void valueChanged(VSTGUI::CControl* control) override {
  if(!control||control->getValueNormalized()<=.5f)return;
  const int tag=control->getTag();
  if(tag==1002||tag==1003||tag==1005){
   owner->clearIRFile(tag==1002?0:(tag==1005?1:2));return;
  }
  if(tag!=1000&&tag!=1001&&tag!=1004)return;
  auto* frame=editor?editor->getFrame():nullptr;
  if(!frame)return;
  auto* chooser=VSTGUI::CNewFileSelector::create(frame,VSTGUI::CNewFileSelector::kSelectFile);
  if(!chooser)return;
  chooser->setTitle(tag==1000?"Load Microphone IR WAV":
                    (tag==1004?"Load Preamp IR WAV":"Load Reverb IR WAV"));
  chooser->addFileExtension(VSTGUI::CFileExtension("Wave Audio","wav","audio/wav"));
  chooser->setDefaultExtension(VSTGUI::CFileExtension("Wave Audio","wav","audio/wav"));
  if(chooser->runModal()&&chooser->getNumSelectedFiles()>0){
   const auto* selected=chooser->getSelectedFile(0);
   if(selected)owner->requestIRFile(selected,tag==1000?0:(tag==1004?1:2));
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
VSTGUI::CView* VMController::verifyView(VSTGUI::CView* view,
 const VSTGUI::UIAttributes&,const VSTGUI::IUIDescription*,
 VSTGUI::VST3Editor*){
 if(auto* label=dynamic_cast<VSTGUI::CTextLabel*>(view)){
  const int tag=label->getTag();
  if(tag==313){
   auto* adapted=new VoiceActiveLabel(*label,this);
   label->forget();
   return adapted;
  }
  if(tag>=310&&tag<=312){
   auto* adapted=new IRFilenameLabel(*label,this,tag-310);
   label->forget();
   return adapted;
  }
 }
 auto* slider=dynamic_cast<VSTGUI::CSlider*>(view);
 if(!slider)return view;
 const int tag=slider->getTag();
 if(tag<kInput||tag>kPreampPredelay||tag==kBypass||tag==kB1Correction||
    tag==kMicPosition||tag==kMicPredelay||tag==kPreampPredelay)return view;
 auto* adapted=new VoiceWheelSlider(*slider);
 slider->forget();
 return adapted;
}
VSTGUI::IController* VMController::createSubController(VSTGUI::UTF8StringPtr name,
 const VSTGUI::IUIDescription*,VSTGUI::VST3Editor* editor){
 if(name&&std::strcmp(name,"IRLoader")==0)return new IRLoaderButtons(this,editor);
 if(name&&std::strcmp(name,"VoicePresets")==0)return new VoicePresetButtons(this);
 return nullptr;
}
void VMController::applyVoiceQuickPreset(bool hollywood){
 // Host-compatible preset changes: update only existing voice parameters,
 // with matching begin/perform/end edit notifications for Studio One recall.
 // Neither INPUT/OUTPUT, BYPASS nor MIC/REVERB IR state is modified.
 for(const auto& control:vm::hollywoodVoicePreset(hollywood)){
  const auto id=static_cast<Vst::ParamID>(control.id);
  if(!getParameterObject(id))continue;
  if(std::fabs(getParamNormalized(id)-control.normalized)<1.e-7)continue;
  beginEdit(id);
  if(setParamNormalized(id,control.normalized)==kResultTrue)
   performEdit(id,getParamNormalized(id));
  endEdit(id);
 }
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
 parameters.addParameter(STR16("Bypass"),nullptr,1,0,Vst::ParameterInfo::kCanAutomate,kBypass);
 parameters.addParameter(STR16("Mic IR Mix"),STR16("%"),0,1,Vst::ParameterInfo::kCanAutomate,kMicMix);
 parameters.addParameter(STR16("Mic IR Level"),nullptr,0,.25,Vst::ParameterInfo::kCanAutomate,kMicLevel);
 parameters.addParameter(STR16("Reverb IR Mix"),STR16("%"),0,.3,Vst::ParameterInfo::kCanAutomate,kReverbMix);
 parameters.addParameter(STR16("Reverb IR Level"),nullptr,0,.25,Vst::ParameterInfo::kCanAutomate,kReverbLevel);
 parameters.addParameter(STR16("Mic IR Position (legacy inactive)"),nullptr,1,0,0,kMicPosition);
 parameters.addParameter(STR16("Mic IR Predelay (legacy inactive)"),STR16("ms"),0,0,0,kMicPredelay);
 parameters.addParameter(STR16("Reverb IR Predelay"),STR16("ms"),0,0,Vst::ParameterInfo::kCanAutomate,kReverbPredelay);
 // Appended controls only: no legacy parameter IDs are reused.
 parameters.addParameter(STR16("B1 Correction"),nullptr,1,1,Vst::ParameterInfo::kCanAutomate,kB1Correction);
 parameters.addParameter(STR16("Preamp IR Mix"),STR16("%"),0,1,Vst::ParameterInfo::kCanAutomate,kPreampMix);
 parameters.addParameter(STR16("Preamp IR Level"),nullptr,0,.25,Vst::ParameterInfo::kCanAutomate,kPreampLevel);
 parameters.addParameter(STR16("Preamp IR Predelay (legacy inactive)"),STR16("ms"),0,0,0,kPreampPredelay);
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
 parameters.addParameter(STR16("Preamp IR Status"),nullptr,0,0,Vst::ParameterInfo::kIsReadOnly,kPreampIRStatus);
 parameters.addParameter(STR16("Preamp IR Error"),nullptr,0,0,Vst::ParameterInfo::kIsReadOnly,kPreampIRError);
 parameters.addParameter(STR16("Input Clip"),nullptr,1,0,Vst::ParameterInfo::kIsReadOnly,kInputClip);
 parameters.addParameter(STR16("Output Clip"),nullptr,1,0,Vst::ParameterInfo::kIsReadOnly,kOutputClip);
 return kResultOk;
}
tresult PLUGIN_API VMController::setComponentState(IBStream* stream){
 if(!stream)return kResultFalse;
 IBStreamer io(stream,kLittleEndian);int32 version=0;
 if(!io.readInt32(version)||(version<1||version>8))return kResultFalse;
 const int count=version==1?10:(version==2?14:(version==3?15:(version<=5?17:(version<=7?kLegacyParameterCount:kParameterCount))));
 // Fully validate before modifying the visible controller state. Older
 // projects must reset new Pitch/Formant controls (and missing IR params)
 // rather than retaining values from the previously opened project.
 float restored[kParameterCount]={.5f,.5f,0,0,0,0,0,0,0,0,1,.25f,.3f,.25f,1.f,0.f,0.f,2.f/3.f,4.f/7.f,0.f,1.f,.25f,0.f};
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
 if(id==kMicIRError||id==kPreampIRError||id==kReverbIRError){
  if(!std::isfinite(value))value=0.;
  const int code=std::clamp(static_cast<int>(std::lround(value*5.)),0,5);
  UString(string,128).fromAscii(vm::irErrorLabel(code));
  return kResultTrue;
 }
 if(id==kMicIRStatus||id==kPreampIRStatus||id==kReverbIRStatus) {
  if(!std::isfinite(value))value=0.;
  const int idx=std::clamp(static_cast<int>(std::lround(value*3.)),0,3);
  const char* labels[]={"EMPTY","LOADING","READY","ERROR"};
  UString(string,128).fromAscii(labels[idx]);
  return kResultTrue;
 }
 if(id==kInputClip||id==kOutputClip){
  UString(string,128).fromAscii(value>=.5?"CLIP":"OK");
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
 Vst::ParamID id,Vst::TChar* string,Vst::ParamValue& normalized) {
 if(!vm::isVoiceControl(static_cast<int>(id)))
  return EditControllerEx1::getParamValueByString(id,string,normalized);
 if(!string)return kResultFalse;
 // Incoming VST3 strings are UTF-16 and null-terminated. The physical
 // value grammar is strictly ASCII; validate conversion and length here.
 // UString(string,-1) is not valid: its second argument is buffer capacity.
 char text[128]{};
 std::size_t length=0;
 for(;length<127&&string[length]!=0;++length) {
  const auto ch=static_cast<unsigned int>(string[length]);
  if(ch>0x7f)return kResultFalse;
  text[length]=static_cast<char>(ch);
 }
 if(length==127)return kResultFalse;
 double parsed=0.;
 if(!vm::parseVoiceControl(static_cast<int>(id),text,parsed))return kResultFalse;
 normalized=parsed;
 return kResultTrue;
}
}
