#include "controller.h"
#include "ids.h"
#include "MeterUnits.h"
#include "pluginterfaces/base/ustring.h"
#include "base/source/fstreamer.h"
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <string>
namespace Steinberg {
tresult PLUGIN_API VMController::initialize(FUnknown* context){
 auto r=EditControllerEx1::initialize(context);if(r!=kResultOk)return r;
 struct Def {const char16* label;Vst::ParamID id;double value;};
 // Units are deliberately defined as generic normalized values until final display mapping is implemented.
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
 parameters.addParameter(STR16("Input Peak L"),nullptr,0,0,Vst::ParameterInfo::kIsReadOnly,kInputPeakL);
 parameters.addParameter(STR16("Input Peak R"),nullptr,0,0,Vst::ParameterInfo::kIsReadOnly,kInputPeakR);
 parameters.addParameter(STR16("Output Peak L"),nullptr,0,0,Vst::ParameterInfo::kIsReadOnly,kOutputPeakL);
 parameters.addParameter(STR16("Output Peak R"),nullptr,0,0,Vst::ParameterInfo::kIsReadOnly,kOutputPeakR);
 return kResultOk;
}
tresult PLUGIN_API VMController::setComponentState(IBStream* stream){
 if(!stream)return kResultFalse;
 IBStreamer io(stream,kLittleEndian);int32 version=0;
 if(!io.readInt32(version)||(version<1||version>4))return kResultFalse;
 const int count=version==1?10:(version==2?14:(version==3?15:kParameterCount));
 for(int id=0;id<count;++id){
  float value=0;if(!io.readFloat(value)||!std::isfinite(value))return kResultFalse;
  setParamNormalized(kInput+id,value);
 }
 return kResultOk;
}
}

namespace Steinberg {
tresult PLUGIN_API VMController::getParamStringByValue(Vst::ParamID id,Vst::ParamValue value,Vst::String128 string) {
 if(!string)return kResultFalse;
 if(id==kInput||id==kOutput||id==kMicMix||id==kReverbMix||
    id==kMicLevel||id==kReverbLevel||id==kMicPredelay||
    id==kReverbPredelay||id==kMicPosition||id==kLowCut){
  if(!std::isfinite(value))value=0.;
  value=std::max(0.,std::min(1.,value));
  char display[40];
  if(id==kInput||id==kOutput)
   std::snprintf(display,sizeof(display),"%.1f dB",(value-.5)*24.);
  else if(id==kMicMix||id==kReverbMix)
   std::snprintf(display,sizeof(display),"%.0f %%",value*100.);
  else if(id==kMicLevel||id==kReverbLevel)
   std::snprintf(display,sizeof(display),"%.0f %%",value*400.);
  else if(id==kLowCut)
   std::snprintf(display,sizeof(display),"%s",value<=0.?"OFF":(std::to_string(static_cast<int>(std::lround(35+145*value)))+" Hz").c_str());
  else if(id==kMicPredelay||id==kReverbPredelay)
   std::snprintf(display,sizeof(display),"%.0f ms",value*2000.);
  else std::snprintf(display,sizeof(display),"%s",value<.5?"PRE":"POST");
  UString(string,128).fromAscii(display);
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
