#include "HollywoodPreset.h"
#include <array>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iterator>
#include <iostream>
#include <string>
namespace {
bool has(const std::string& text,const std::string& find){
 return text.find(find)!=std::string::npos;
}
}
int main(int argc,char** argv){
 assert(argc==2);
 const auto on=vm::hollywoodVoicePreset(true);
 const auto off=vm::hollywoodVoicePreset(false);
 assert(on.size()==9&&off.size()==on.size());
 for(std::size_t i=0;i<on.size();++i){
  const auto id=on[i].id;
  assert(id>=102&&id<=118&&id!=109&&id!=110&&id!=111&&
         id!=112&&id!=113&&id!=114&&id!=115&&id!=116);
  assert(off[i].id==id);
  assert(on[i].normalized>=0.&&on[i].normalized<=1.);
  assert(off[i].normalized>=0.&&off[i].normalized<=1.);
  for(std::size_t j=i+1;j<on.size();++j)assert(on[j].id!=id);
 }
 assert(on[3].id==105&&on[3].normalized==0.);
 assert(on[7].id==117&&std::fabs(on[7].normalized*18.-12.)<1e-10);
 assert(std::fabs(on[8].normalized*14.-8.+1.)<1e-10);
 assert(on[7].normalized==off[7].normalized);
 assert(on[3].normalized==off[3].normalized);
 assert(std::fabs(off[7].normalized*18.-12.)<1e-10);
 assert(std::fabs(off[8].normalized*14.-8.)<1e-10);
 // No changes to gain, global bypass, existing IR settings or automation IDs.
 std::ifstream file(argv[1],std::ios::binary);
 assert(file.good());
 const std::string xml(std::istreambuf_iterator<char>{file},
                       std::istreambuf_iterator<char>{});
 assert(has(xml,"sub-controller=\"VoicePresets\""));
 assert(has(xml,"name=\"Hollywood\" tag=\"1010\""));
 assert(has(xml,"name=\"OriginalVoice\" tag=\"1011\""));
 assert(has(xml,"control-tag=\"Hollywood\""));
 assert(has(xml,"control-tag=\"OriginalVoice\""));
 assert(has(xml,"title=\"HOLLYWOOD\""));
 assert(has(xml,"title=\"ORIGINAL\""));
 for(int id=100;id<=118;++id){
  const auto tag="name=\"P"+std::to_string(id)+"\" tag=\""+std::to_string(id)+"\"";
  assert(has(xml,tag));
  assert(has(xml,"control-tag=\"P"+std::to_string(id)+"\""));
 }
 // Separate VST3-bound mouse sliders must be present as well as the
 // editable text fields. They must use each parameter's actual neutral
 // default instead of mistakenly assuming normalized 0.5 means 0 st.
 const auto sliderFor=[&](int id){
  const auto needle=std::string("<view class=\"CSlider\"");
  std::size_t pos=0;
  while((pos=xml.find(needle,pos))!=std::string::npos){
   const auto end=xml.find("/>",pos);
   assert(end!=std::string::npos);
   const auto view=xml.substr(pos,end-pos+2);
   if(has(view,"control-tag=\"P"+std::to_string(id)+"\""))
    return view;
   pos=end+2;
  }
  return std::string{};
 };
 // Sub-controller applies to descendants. Keep the IR command listener scoped
 // to the four buttons, never to the surrounding seven VST3 numeric edits.
 assert(!has(xml,"origin=\"632, 85\" size=\"348, 520\" background-color=\"Panel\" transparent=\"false\" sub-controller"));
 const auto count=[&](const std::string& needle){
  std::size_t n=0,at=0;
  while((at=xml.find(needle,at))!=std::string::npos){++n;at+=needle.size();}
  return n;
 };
 assert(count("sub-controller=\"IRLoader\"")==6);
 for(const char* tag:{"ClearMic","LoadMic","ClearRev","LoadRev","ClearPreamp","LoadPreamp"}){
  const auto opening=xml.find("sub-controller=\"IRLoader\"");
  assert(opening!=std::string::npos);
  assert(count(std::string("control-tag=\"")+tag+"\"")==1);
  // Each command has its own isolated listener container.
  const auto pos=xml.find(std::string("control-tag=\"")+tag+"\"");
  const auto parent=xml.rfind("<view class=\"CViewContainer\"",pos);
  assert(parent!=std::string::npos);
  const auto startOfButton=xml.find("<view class=\"CTextButton\"",parent);
  assert(startOfButton!=std::string::npos&&startOfButton<pos);
  const auto parentEnd=xml.find('>',parent);
  assert(parentEnd!=std::string::npos&&parentEnd<pos);
  assert(has(xml.substr(parent,parentEnd-parent),"sub-controller=\"IRLoader\""));
 }
 for(const int id:{110,111,112,113,115,116,120,121,122}){
  const auto pos=xml.find("control-tag=\"P"+std::to_string(id)+"\"");
  assert(pos!=std::string::npos);
  const auto view=xml.rfind("<view class=\"CTextEdit\"",pos);
  assert(view!=std::string::npos&&pos-view<600);
 }
 const auto pitchSlider=sliderFor(117);
 const auto formantSlider=sliderFor(118);
 assert(!pitchSlider.empty()&&!formantSlider.empty());
 for(const auto& slider:{pitchSlider,formantSlider}){
  assert(has(slider,"orientation=\"horizontal\""));
  assert(has(slider,"draw-value=\"true\""));
  assert(has(slider,"draw-value-from-center=\"false\""));
  assert(has(slider,"mode=\"free click\""));
 }
 assert(has(pitchSlider,"default-value=\"0.666666667\""));
 assert(has(formantSlider,"default-value=\"0.571428571\""));
 assert(has(pitchSlider,"origin=\"16, 360\""));
 assert(has(formantSlider,"origin=\"305, 360\""));
 assert(has(xml,"title=\"05  VOICE PROCESSING\""));
 for(const char* tag:{"LoadMic","LoadRev","ClearMic","ClearRev","LoadPreamp","ClearPreamp"})
  assert(has(xml,std::string("control-tag=\"")+tag+"\""));

 // Full audible/control path: new IDs, toggles, 3 IR slots, 4 meters and clipping.
 for(const int id:{119,120,121,122}){
  assert(has(xml,"name=\"P"+std::to_string(id)+"\" tag=\""+std::to_string(id)+"\""));
  assert(has(xml,"control-tag=\"P"+std::to_string(id)+"\""));
 }
 assert(has(xml,"title=\"B1 EQ ON/OFF\" control-tag=\"P119\""));
 assert(has(xml,"title=\"PRE/POST\" control-tag=\"P114\""));
 for(const char* tag:{"InputPeakL","InputPeakR","OutputPeakL","OutputPeakR","InputClip","OutputClip","PreampStatus"})
  assert(has(xml,std::string("control-tag=\"")+tag+"\""));
 for(const char* stage:{"01  INPUT","02  B1 CORRECTION EQ","03  MICROPHONE IR","04  PREAMP IR","05  VOICE PROCESSING","06  REVERB IR","07  OUTPUT"})
  assert(has(xml,stage));
 std::cout<<"Seven-step signal path, B1 toggle, 3 IR slots and meters GUI contract PASS\n";
}
