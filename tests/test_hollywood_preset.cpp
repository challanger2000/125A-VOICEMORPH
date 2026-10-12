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
 assert(std::fabs(on[7].normalized*18.-12.+2.)<1e-10);
 assert(std::fabs(on[8].normalized*14.-8.+1.)<1e-10);
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
 for(const char* tag:{"LoadMic","LoadRev","ClearMic","ClearRev"})
  assert(has(xml,std::string("control-tag=\"")+tag+"\""));
 std::cout<<"Two one-click voice presets, 19 existing parameter bindings and IR controls PASS\n";
}
