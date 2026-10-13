#include <array>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <string>
#include <vector>
#include <utility>

// Static VSTGUI -> VST3 Controller registration -> Processor/DSP trace.
// This must be combined with a real editor/DAW test before claiming full QA.
static std::string read(const char* filename){
 std::ifstream stream(filename,std::ios::binary);
 assert(stream.good());
 return {std::istreambuf_iterator<char>{stream},std::istreambuf_iterator<char>{}};
}
static int count(const std::string& s,const std::string& term){
 int n=0;std::size_t at=0;
 while((at=s.find(term,at))!=std::string::npos){++n;at+=term.size();}
 return n;
}
static bool inParamDeclaration(const std::string& ctrl,const std::string& id,
                               bool automatable){
 std::size_t at=0;
 while((at=ctrl.find("parameters.addParameter(",at))!=std::string::npos){
  const auto end=ctrl.find(");",at);
  assert(end!=std::string::npos);
  const std::string item=ctrl.substr(at,end-at+2);
  const bool hasId=item.find(","+id+");")!=std::string::npos;
  if(hasId)return (item.find("kCanAutomate")!=std::string::npos)==automatable;
  at=end+2;
 }
 return false;
}
struct Binding {
 int id;
 const char* symbol;
 const char* signalPath;
};
int main(int argc,char** argv){
 assert(argc==6);
 const std::string ui=read(argv[1]),ids=read(argv[2]),controller=read(argv[3]);
 const std::string processor=read(argv[4]),chain=read(argv[5]);
 // Distinct signal-path identifiers come from independently reviewed DSP sources,
 // not from the GUI file itself.
 constexpr std::array<Binding,20> controls{{
 {100,"kInput","fixedIRChain.setInputOutputGains((params[0]-.5f)*24.f"},
 {101,"kOutput","(params[1]-.5f)*24.f"},
 {102,"kDeNasal","p.denasal=params[2]"},
 {103,"kChest","p.chest=params[3]"},
 {104,"kWarmth","p.warmth=params[4]"},
 {105,"kGrit","p.grit=params[5]"},
 {106,"kPresence","p.presence=params[6]"},
 {107,"kCompression","p.compression=params[7]"},
 {108,"kLowCut","p.lowCut=params[8]"},
 {109,"kBypass","params[kBypass-kInput]>=.5f"},
 {110,"kMicMix","mic->setMix(params[kMicMix-kInput])"},
 {111,"kMicLevel","mic->setWetLevel(params[kMicLevel-kInput]*4.f)"},
 {112,"kReverbMix","rev->setMix(vm::reverbWetMix(params[kReverbMix-kInput]))"},
 {113,"kReverbLevel","rev->setWetLevel(params[kReverbLevel-kInput]*4.f)"},
 {116,"kReverbPredelay","rev->setPredelayNormalized(params[kReverbPredelay-kInput])"},
 {117,"kPitch","pitchEngine.setPitchSemitones("},
 {118,"kFormant","pitchEngine.setFormantSemitones("},
 {119,"kB1Correction","b1Correction.setEnabled(params[kB1Correction-kInput]>=.5f)"},
 {120,"kPreampMix","preamp->setMix(params[kPreampMix-kInput])"},
 {121,"kPreampLevel","preamp->setWetLevel(params[kPreampLevel-kInput]*4.f)"}
 }};
 constexpr std::array<int,18> sliders{{
  100,101,102,103,104,105,106,107,108,110,111,112,113,116,
  117,118,120,121
 }};
 for(const auto& c:controls){
  const std::string tag="P"+std::to_string(c.id);
  const std::string idName=c.symbol;
  assert(ids.find(idName+"="+std::to_string(c.id))!=std::string::npos);
  assert(count(ui,"control-tag=\""+tag+"\"")==2);
  assert(inParamDeclaration(controller,idName,true));
  assert(processor.find(c.signalPath)!=std::string::npos);
  bool slider=false;
  for(int id:sliders)if(id==c.id)slider=true;
  if(slider){
   bool foundEdit=false,foundSlider=false;
   for(const std::string& classname:{"CSlider","CTextEdit"}){
    std::size_t at=0;
    while((at=ui.find("<view class=\""+classname+"\"",at))!=std::string::npos){
     const auto end=ui.find("/>",at);
     assert(end!=std::string::npos);
     const auto element=ui.substr(at,end-at+2);
     if(element.find("control-tag=\""+tag+"\"")!=std::string::npos){
      if(classname=="CSlider"){foundSlider=true;assert(element.find("default-value=\"")!=std::string::npos);}
      else foundEdit=true;
     }
     at=end+2;
    }
   }
   assert(foundEdit&&foundSlider);
  }else{
   assert(c.id==109||c.id==119);
   assert(ui.find("control-tag=\""+tag+"\" kick-style=\"false\"")!=std::string::npos||
          ui.find("control-tag=\""+tag+"\" default-value=")!=std::string::npos);
   assert(ui.find("class=\"CTextLabel\"")!=std::string::npos);
  }
 }
 // 3 retired parameters retain their historical IDs/state slots but must
 // NOT be presented as editable, automatable, functional GUI controls.
 for(const auto& deprecated:std::array<std::pair<int,const char*>,3>{{
  {114,"kMicPosition"},{115,"kMicPredelay"},{122,"kPreampPredelay"}
 }}){
  assert(ids.find(std::string(deprecated.second)+"="+std::to_string(deprecated.first))!=std::string::npos);
  assert(count(ui,"control-tag=\"P"+std::to_string(deprecated.first)+"\"")==0);
  assert(inParamDeclaration(controller,deprecated.second,false));
 }
 // Six file commands MUST have specific event dispatch and separated slots.
 for(const char* tag:{"LoadMic","ClearMic","LoadPreamp","ClearPreamp","LoadRev","ClearRev"})
  assert(count(ui,std::string("control-tag=\"")+tag+"\"")==1);
 for(const char* required:{"tag==1002","tag==1003","tag==1005",
                          "tag!=1000&&tag!=1001&&tag!=1004",
                          "owner->requestIRFile(selected,tag==1000?0:(tag==1004?1:2))",
                          "owner->clearIRFile(tag==1002?0:(tag==1005?1:2))",
                          "125A.LoadIR","125A.ClearIR"})
  assert(controller.find(required)!=std::string::npos||
         processor.find(required)!=std::string::npos);
 // Both preset buttons are real callbacks to controller-edit lifecycle.
 for(const char* tag:{"Hollywood","OriginalVoice"})assert(count(ui,std::string("control-tag=\"")+tag+"\"")==1);
 for(const char* entry:{"applyVoiceQuickPreset(true)","applyVoiceQuickPreset(false)",
                       "beginEdit(id);","performEdit(id,getParamNormalized(id));","endEdit(id);"})
  assert(controller.find(entry)!=std::string::npos);
 // All 4 realtime read-only meter values, 2 clips and 3 IR statuses
 // MUST be present in UI, registered read-only and emitted by processor.
 constexpr std::array<const char*,9> monitorIds{{
  "kInputPeakL","kInputPeakR","kOutputPeakL","kOutputPeakR",
  "kInputClip","kOutputClip","kMicIRStatus","kPreampIRStatus","kReverbIRStatus"
 }};
 constexpr std::array<const char*,9> monitorTags{{
  "InputPeakL","InputPeakR","OutputPeakL","OutputPeakR",
  "InputClip","OutputClip","MicStatus","PreampStatus","RevStatus"
 }};
 for(std::size_t i=0;i<monitorIds.size();++i){
  assert(ui.find(std::string("control-tag=\"")+monitorTags[i]+"\"")!=std::string::npos);
  assert(inParamDeclaration(controller,monitorIds[i],false));
  assert(controller.find(std::string("kIsReadOnly,")+monitorIds[i])!=std::string::npos);
  assert(processor.find(monitorIds[i])!=std::string::npos);
 }
 // GUI should NOT advertise 'OFF' as a change to a global plugin bypass.
 assert(chain.find("stage(mic,micEmpty_,l,r);")!=std::string::npos);
 assert(chain.find("stage(preamp,preampEmpty_,l,r);")!=std::string::npos);
 assert(chain.find("stage(reverb,reverbEmpty_,l,r);")!=std::string::npos);
 assert(chain.find("dryVoiceL*bypassAmount_")!=std::string::npos);
 // Non-audio state migration and sample accurate automation must cover
 // the complete parameter set rather than the earlier 19-item suite.
 assert(processor.find("float next[kParameterCount]")!=std::string::npos);
 assert(processor.find("stateMailbox.storeAutomationAudio")!=std::string::npos);
 std::cout<<"20 automatable GUI parameters, 3 retired controls, 6 file buttons,"
             " 2 presets, 9 status/meter tags statically traceable PASS\n";
}
