#include "HollywoodPreset.h"
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <set>
namespace {
bool has(const std::string& s,const std::string& needle){return s.find(needle)!=std::string::npos;}
int count(const std::string& s,const std::string& needle){
 int n=0;std::size_t at=0;while((at=s.find(needle,at))!=std::string::npos){++n;at+=needle.size();}return n;
}
}
int main(int argc,char** argv){
 assert(argc==2);
 const auto hollywood=vm::hollywoodVoicePreset(true),original=vm::hollywoodVoicePreset(false);
 assert(hollywood.size()==9&&original.size()==9);
 for(std::size_t i=0;i<hollywood.size();++i){
  assert(hollywood[i].id==original[i].id);
  assert(hollywood[i].id>=102&&hollywood[i].id<=118);
  assert(hollywood[i].id!=109&&hollywood[i].id!=114);
  assert(hollywood[i].normalized>=0&&hollywood[i].normalized<=1.);
 }
 assert(hollywood[3].id==105&&hollywood[3].normalized==0.);
 std::ifstream f(argv[1],std::ios::binary);assert(f.good());
 const std::string ui{std::istreambuf_iterator<char>{f},std::istreambuf_iterator<char>{}};
 // Unique authoritative logo, centered product title and explicit 7-step ordering.
 assert(has(ui,"<bitmap name=\"MasterLogo\" path=\"125A_Logo_Master.png\"/>"));
 assert(has(ui,"title=\"VOICEMORPH\" font=\"Title\""));
 assert(has(ui,"origin=\"316, 13\" size=\"368, 39\""));
 assert(has(ui,"size=\"1000, 615\""));
 for(const char* text:{"01  INPUT","02  B1 EQ","03  MICROPHONE IR","04  PREAMP IR",
                       "05  VOICE PROCESSING","06  REVERB IR","07  OUTPUT"})
  assert(has(ui,text));
 // Every active control has a mouse slider plus direct numeric edit:
 // input/output, six voice tone controls, low-cut, pitch, formant, all IR
 // MIX/LEVEL, and reverb predelay.
 for(const int id:{100,101,102,103,104,105,106,107,108,110,111,112,113,116,
                   117,118,120,121}){
  const std::string tag="control-tag=\"P"+std::to_string(id)+"\"";
  assert(count(ui,"class=\"CSlider\"")>=18);
  const auto slider=ui.find("class=\"CSlider\"");
  assert(slider!=std::string::npos);
  std::size_t at=0;bool sliderFound=false,editFound=false;
  while((at=ui.find(tag,at))!=std::string::npos){
   const auto view=ui.rfind("<view class=",at);
   const auto type=ui.substr(view,at-view);
   if(has(type,"CSlider"))sliderFound=true;
   if(has(type,"CTextEdit"))editFound=true;
   at+=tag.size();
  }
  assert(sliderFound&&editFound);
 }
 // Only explicitly agreed controls are visible: no MIC position, no MIC
 // predelay and no PREAMP predelay; legacy VST3 IDs are still declared.
 for(const int id:{114,115,122}){
  assert(has(ui,"name=\"P"+std::to_string(id)+"\""));
  assert(!has(ui,"control-tag=\"P"+std::to_string(id)+"\""));
 }
 assert(count(ui,"sub-controller=\"IRLoader\"")==6);
 for(const char* tag:{"LoadMic","ClearMic","LoadPreamp","ClearPreamp","LoadRev","ClearRev"})
  assert(count(ui,std::string("control-tag=\"")+tag+"\"")==1);
 for(const char* tag:{"InputPeakL","InputPeakR","OutputPeakL","OutputPeakR","InputClip","OutputClip",
                      "MicStatus","PreampStatus","RevStatus"})
  assert(has(ui,std::string("control-tag=\"")+tag+"\""));
 // Binary controls are clickable and separately expose a readable state.
 assert(has(ui,"title=\"VOICE BYPASS\" control-tag=\"P109\" kick-style=\"false\""));
 assert(has(ui,"title=\"B1 EQ\" control-tag=\"P119\" kick-style=\"false\""));
 assert(count(ui,"control-tag=\"P109\"")==2);
 assert(count(ui,"control-tag=\"P119\"")==2);
 assert(has(ui,"sub-controller=\"VoicePresets\""));
 assert(count(ui,"control-tag=\"Hollywood\"")==1);
 assert(count(ui,"control-tag=\"OriginalVoice\"")==1);
 std::cout<<"Master logo, ordered stage layout, 18 mouse sliders + text edits, 6 IR commands, voice-only bypass GUI PASS\n";
}
