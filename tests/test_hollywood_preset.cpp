#include "HollywoodPreset.h"
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <set>
#include <array>
#include <utility>
#include <cstdlib>
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
 assert(has(ui,"title=\"VOICE BYPASS\" control-tag=\"P109\" default-value=\"0\" kick-style=\"false\""));
 assert(has(ui,"title=\"B1 EQ\" control-tag=\"P119\" default-value=\"1\" kick-style=\"false\""));
 assert(count(ui,"control-tag=\"P109\"")==2);
 assert(count(ui,"control-tag=\"P119\"")==2);
 assert(has(ui,"sub-controller=\"VoicePresets\""));
 assert(count(ui,"control-tag=\"Hollywood\"")==1);
 assert(count(ui,"control-tag=\"OriginalVoice\"")==1);
 // Every editable slider reset value must match its actual VST3 default.
 const std::array<std::pair<int,double>,18> sliderDefaults{{
   {100,.5},{101,.5},{102,0.},{103,0.},{104,0.},{105,0.},
   {106,0.},{107,0.},{108,0.},{110,1.},{111,.25},
   {112,.3},{113,.25},{116,0.},{117,2./3.},{118,4./7.},
   {120,1.},{121,.25}
 }};
 for(const auto& item:sliderDefaults){
  const auto tag="control-tag=\"P"+std::to_string(item.first)+"\"";
  bool found=false;
  std::size_t at=0;
  while((at=ui.find("<view class=\"CSlider\"",at))!=std::string::npos){
   const auto end=ui.find("/>",at);assert(end!=std::string::npos);
   const auto view=ui.substr(at,end-at+2);
   if(has(view,tag)){
    const auto key=view.find("default-value=\"");
    assert(key!=std::string::npos);
    const auto start=key+15,close=view.find('"',start);
    assert(close!=std::string::npos);
    const double actual=std::strtod(view.substr(start,close-start).c_str(),nullptr);
    assert(std::fabs(actual-item.second)<1.e-6);
    found=true;break;
   }
   at=end+2;
  }
  assert(found);
 }
 for(const auto& item:std::array<std::pair<int,int>,2>{{{109,0},{119,1}}}){
  const auto tag="control-tag=\"P"+std::to_string(item.first)+"\"";
  bool found=false;std::size_t at=0;
  while((at=ui.find("<view class=\"CTextButton\"",at))!=std::string::npos){
   const auto end=ui.find("/>",at);assert(end!=std::string::npos);
   const auto view=ui.substr(at,end-at+2);
   if(has(view,tag)){
    assert(has(view,"default-value=\""+std::to_string(item.second)+"\""));
    found=true;break;
   }
   at=end+2;
  }
  assert(found);
 }
 std::cout<<"20 visible parameter defaults agree with reset controls PASS\n";
 std::cout<<"Master logo, ordered stage layout, 18 mouse sliders + text edits, 6 IR commands, voice-only bypass GUI PASS\n";
}
