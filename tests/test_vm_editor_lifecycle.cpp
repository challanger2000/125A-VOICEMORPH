// Independent native Win32 VST3 editor lifecycle probe. Does not link to
// the plugin's C++ classes: loads the exact staged Windows VST3 bundle.
// Conceptually modelled on 125A Plugin Tester Final's own editor probe.
#include <windows.h>
#include <objbase.h>
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "base/source/fobject.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "public.sdk/source/common/memorystream.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace VST3::Hosting;
namespace {
constexpr int kCycles=20;
constexpr wchar_t kClassName[]=L"125A_VoiceMorph_Editor_Lifecycle_Probe";
LRESULT CALLBACK windowProc(HWND hwnd,UINT msg,WPARAM w,LPARAM l){
 if(msg==WM_GETMINMAXINFO&&l){
  // Windows' default MAXTRACKSIZE constrains a resizable top-level host
  // window to approx 1028x749 on GitHub's 1024x728 desktop, even when
  // VST3 requests a 1500x923 view. A real DAW is expected to permit
  // plugin windows larger than its test desktop; keep the requested
  // logical/physical client region valid and pan the window for each
  // native control hit test.
  auto* mm=reinterpret_cast<MINMAXINFO*>(l);
  const auto result=DefWindowProcW(hwnd,msg,w,l);
  mm->ptMaxTrackSize.x=4096;
  mm->ptMaxTrackSize.y=4096;
  return result;
 }
 return DefWindowProcW(hwnd,msg,w,l);
}
void pump(){
 MSG m{};
 for(int pass=0;pass<10;++pass){
  while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){
   TranslateMessage(&m);DispatchMessageW(&m);
  }
  Sleep(8);
 }
}
class HostFrame final:public FObject,public IPlugFrame {
 HWND hwnd_;
public:
 explicit HostFrame(HWND hwnd):hwnd_(hwnd){}
 tresult PLUGIN_API resizeView(IPlugView* view,ViewRect* size) override {
  if(!view||!size||!hwnd_)return kInvalidArgument;
  const int w=std::max<int>(1,size->right-size->left);
  const int h=std::max<int>(1,size->bottom-size->top);
  RECT rect{0,0,w,h};
  const DWORD style=static_cast<DWORD>(GetWindowLongPtrW(hwnd_,GWL_STYLE));
  const DWORD ex=static_cast<DWORD>(GetWindowLongPtrW(hwnd_,GWL_EXSTYLE));
  if(AdjustWindowRectEx(&rect,style,FALSE,ex))
   SetWindowPos(hwnd_,nullptr,0,0,rect.right-rect.left,rect.bottom-rect.top,
                SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
  return view->onSize(size);
 }
 OBJ_METHODS(HostFrame,FObject)
 DEFINE_INTERFACES
  DEF_INTERFACE(IPlugFrame)
 END_DEFINE_INTERFACES(FObject)
 REFCOUNT_METHODS(FObject)
};

struct RequiredParam {
 ParamID id;
 ParamValue defaultValue;
};
int checkRealControllerParameters(IEditController* controller){
 if(!controller)return 70;
 const std::array<RequiredParam,20> params{{
  {100,.5},{101,.5},{102,0.},{103,0.},{104,0.},{105,0.},
  {106,0.},{107,0.},{108,0.},{109,0.},{110,1.},
  {111,.25},{112,.3},{113,.25},{116,0.},
  {117,2./3.},{118,4./7.},{119,1.},{120,1.},{121,.25}
 }};
 auto locate=[&](ParamID id,ParameterInfo& found)->bool{
  const auto total=controller->getParameterCount();
  for(int32 idx=0;idx<total;++idx){
   ParameterInfo candidate{};
   if(controller->getParameterInfo(idx,candidate)!=kResultTrue)return false;
   if(candidate.id==id){found=candidate;return true;}
  }
  return false;
 };
 for(const auto& spec:params){
  ParameterInfo info{};
  if(!locate(spec.id,info) ||
     !(info.flags&ParameterInfo::kCanAutomate) ||
     (info.flags&ParameterInfo::kIsReadOnly)){
   std::cerr<<"Active parameter missing/disabled ID "<<spec.id<<"\n";return 71;
  }
  if(std::fabs(info.defaultNormalizedValue-spec.defaultValue)>1e-5 ||
     std::fabs(controller->getParamNormalized(spec.id)-spec.defaultValue)>1e-5){
   std::cerr<<"Parameter default mismatch ID "<<spec.id<<"\n";return 72;
  }
  for(const double v:{0.,.25,.5,.75,1.}){
   if((spec.id==109||spec.id==119) && v!=0. && v!=1.)continue;
   if(controller->setParamNormalized(spec.id,v)!=kResultTrue ||
      std::fabs(controller->getParamNormalized(spec.id)-v)>1e-6){
    std::cerr<<"Controller value roundtrip FAIL ID "<<spec.id<<" at "<<v<<"\n";
    return 73;
   }
   TChar display[128]{};
   if(controller->getParamStringByValue(spec.id,v,display)!=kResultTrue ||
      display[0]==0){
    std::cerr<<"VST3 display text FAIL ID "<<spec.id<<"\n";return 74;
   }
   ParamValue decoded=-1.;
   if(controller->getParamValueByString(spec.id,display,decoded)!=kResultTrue ||
      !std::isfinite(decoded) ||
      std::fabs(decoded-v)>.015){
    std::cerr<<"VST3 display/parse mismatch ID "<<spec.id
             <<" at normalized "<<v<<", returned "<<decoded<<"\n";
    return 75;
   }
  }
  if(controller->setParamNormalized(spec.id,spec.defaultValue)!=kResultTrue)
   return 76;
 }
 for(ParamID id:{114u,115u,122u}){
  ParameterInfo legacy{};
  if(!locate(id,legacy)||(legacy.flags&ParameterInfo::kCanAutomate)){
   std::cerr<<"Retired parameter incorrectly automatable ID "<<id<<"\n";
   return 77;
  }
 }
 for(ParamID id:{200u,201u,202u,203u,204u,205u,206u,207u,208u,209u,210u,211u}){
  ParameterInfo info{};
  if(!locate(id,info)||!(info.flags&ParameterInfo::kIsReadOnly) ||
     (info.flags&ParameterInfo::kCanAutomate)){
   std::cerr<<"Read-only meter or IR status flag FAIL ID "<<id<<"\n";
   return 78;
  }
 }
 std::cout<<"Actual VST3 Controller: 18 continuous x5 plus 2 binary x2,"
          <<" text display/parse, 3 legacy and 12 read-only flags PASS\n";
 return 0;
}


int checkRealProcessingLifecycle(IComponent* component){
 if(!component)return 79;
 IAudioProcessor* processor=nullptr;
 if(component->queryInterface(IAudioProcessor::iid,
       reinterpret_cast<void**>(&processor))!=kResultTrue||!processor){
  std::cerr<<"No IAudioProcessor in real VST3\n";return 80;
 }
 for(int sampleFormat:{kSample32,kSample64}){
  ProcessSetup setup{};
  setup.processMode=kRealtime;
  setup.sampleRate=48000.;
  setup.maxSamplesPerBlock=512;
  setup.symbolicSampleSize=sampleFormat;
  if(processor->setupProcessing(setup)!=kResultOk){
   std::cerr<<"setupProcessing FAIL sample format "<<sampleFormat<<"\n";
   processor->release();return 81;
  }
  if(component->setActive(true)!=kResultOk){
   std::cerr<<"setActive(true) FAIL\n";processor->release();return 82;
  }
  if(processor->setProcessing(true)!=kResultOk){
   std::cerr<<"setProcessing(true) FAIL sample format "<<sampleFormat<<"\n";
   component->setActive(false);processor->release();return 83;
  }
  // Steinberg allows false immediately after true with no process() block.
  if(processor->setProcessing(false)!=kResultOk ||
     processor->setProcessing(true)!=kResultOk ||
     processor->setProcessing(false)!=kResultOk){
   std::cerr<<"setProcessing transport toggle FAIL\n";
   component->setActive(false);processor->release();return 84;
  }
  if(component->setActive(false)!=kResultOk){
   std::cerr<<"setActive(false) FAIL\n";processor->release();return 85;
  }
 }
 processor->release();
 std::cout<<"Real VST3 setProcessing start/stop/restart in 32/64-bit PASS\n";
 return 0;
}


// Native interaction gate: dispatch real Win32 button/motion events to
// the VSTGUI HWND, then read actual VST3 controller parameter state.
// Coordinates are global *inside the 1000x615 view* and were independently
// traced from the authoritative UIDESC nested-container positions.
// This is not a simulated controller->setParamNormalized() test.
struct Widget {ParamID id;int x,y,w,h;};
constexpr std::array<Widget,18> kGuiSliders{{
 {100,65,148,83,17},{101,861,501,51,17},
 {102,28,348,200,17},{103,344,348,200,17},
 {104,660,348,200,17},{105,28,396,200,17},
 {106,344,396,200,17},{107,660,396,200,17},
 {108,28,444,200,17},{110,455,193,153,17},
 {111,455,227,153,17},{112,28,558,112,17},
 {113,235,558,112,17},{116,442,558,112,17},
 {117,344,444,200,17},{118,660,444,200,17},
 {120,751,193,153,17},{121,751,227,153,17}
}};

void positionWidgetInsideMonitor(HWND host,int x,int y) {
 // GitHub's desktop is sometimes only 1024x768, while a 150% 1000x615
 // editor is 1500x923 plus Win32 title/decorations. A real OS pointer
 // cannot click parts beyond screen bounds; translate the hosting window
 // rather than falsely declaring a zoom hit-region defect.
 POINT p{x,y};
 if(!ClientToScreen(host,&p))return;
 MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);
 if(!GetMonitorInfoW(MonitorFromWindow(host,MONITOR_DEFAULTTONEAREST),&monitor))return;
 const RECT& area=monitor.rcWork;
 const int cx=(area.left+area.right)/2;
 const int cy=(area.top+area.bottom)/2;
 const int margin=32;
 const int dx=p.x<area.left+margin?cx-p.x:
              (p.x>area.right-margin?cx-p.x:0);
 const int dy=p.y<area.top+margin?cy-p.y:
              (p.y>area.bottom-margin?cy-p.y:0);
 if(dx||dy){
  RECT frame{};
  GetWindowRect(host,&frame);
  SetWindowPos(host,nullptr,frame.left+dx,frame.top+dy,0,0,
               SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
  pump();
  POINT check{x,y};ClientToScreen(host,&check);
  std::cout<<"Native viewport relocation for 150% testing: "<<dx<<","<<dy
           <<" client ("<<x<<","<<y<<") -> desktop ("
           <<check.x<<","<<check.y<<") work area "
           <<area.right-area.left<<"x"<<area.bottom-area.top<<"\n";
 }
}

HWND hitTarget(HWND host,int& x,int& y) {
 POINT client{x,y};
 const HWND target=ChildWindowFromPointEx(host,client,CWP_SKIPINVISIBLE|CWP_SKIPDISABLED);
 if(target && target!=host){
  POINT screen=client;ClientToScreen(host,&screen);
  ScreenToClient(target,&screen);
  x=screen.x;y=screen.y;
  return target;
 }
 return host;
}
void mouseAction(HWND host,int x,int y,bool down,WPARAM modifiers=0) {
 positionWidgetInsideMonitor(host,x,y);
 HWND target=hitTarget(host,x,y);
 SendMessageW(target,down?WM_LBUTTONDOWN:WM_LBUTTONUP,
              (down?MK_LBUTTON:0)|modifiers,MAKELPARAM(x,y));
 pump();
}
void dragNative(HWND host,int x0,int y0,int x1,int y1) {
 positionWidgetInsideMonitor(host,(x0+x1)/2,(y0+y1)/2);
 int x=x0,y=y0;
 HWND target=hitTarget(host,x,y);
 SetForegroundWindow(host);SetFocus(target);
 POINT startScreen{x0,y0};ClientToScreen(host,&startScreen);
 SetCursorPos(startScreen.x,startScreen.y);pump();
 SendMessageW(target,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(x,y));
 pump();
 POINT endScreen{x1,y1};ClientToScreen(host,&endScreen);
 SetCursorPos(endScreen.x,endScreen.y);pump();
 POINT end{x1,y1};ClientToScreen(host,&end);ScreenToClient(target,&end);
 SendMessageW(target,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(end.x,end.y));
 pump();
 SendMessageW(target,WM_LBUTTONUP,0,MAKELPARAM(end.x,end.y));
 pump();
}

constexpr std::array<Widget,18> kGuiTextFields{{
 {100,157,142,68,25},{101,918,495,59,25},
 {102,235,340,77,25},{103,551,340,77,25},
 {104,867,340,77,25},{105,235,388,77,25},
 {106,551,388,77,25},{107,867,388,77,25},
 {108,235,436,77,25},{110,615,187,65,25},
 {111,615,221,65,25},{112,147,551,68,25},
 {113,354,551,68,25},{116,561,551,68,25},
 {117,551,436,77,25},{118,867,436,77,25},
 {120,911,187,65,25},{121,911,221,65,25}
}};
int exerciseNativeWheelAndText(HWND host,IEditController* controller,double scale) {
 const auto scaled=[&](int x){return int(std::lround(x*scale));};
 for(const Widget& w:kGuiSliders){
  if(controller->setParamNormalized(w.id,.4)!=kResultTrue)return 110;
  const auto before=controller->getParamNormalized(w.id);
  const int x=scaled(w.x+w.w/2),y=scaled(w.y+w.h/2);
  positionWidgetInsideMonitor(host,x,y);
  POINT screen{x,y};ClientToScreen(host,&screen);
  SetCursorPos(screen.x,screen.y);pump();
  int hitX=x,hitY=y;
  HWND target=hitTarget(host,hitX,hitY);
  SendMessageW(target,WM_MOUSEWHEEL,MAKEWPARAM(0,WHEEL_DELTA),
               MAKELPARAM(screen.x,screen.y));
  pump();
  const auto after=controller->getParamNormalized(w.id);
  if(!std::isfinite(after)||std::fabs(after-before)<1.e-6){
   std::cerr<<"Native mouse wheel FAIL parameter "<<w.id
            <<" zoom "<<scale<<" before "<<before<<" after "<<after<<"\n";
   return 111;
  }
  std::cout<<"Native wheel "<<w.id<<" @ "<<scale<<"x PASS\n";
 }
 // Keyboard-edit a user-visible CTextEdit for every slider. Use the actual
 // physical display string as the input, then compare the Controller result.
 // This is real native WM_CHAR typing on a focused Win32 EDIT child.
 for(const Widget& w:kGuiTextFields){
  if(controller->setParamNormalized(w.id,.12)!=kResultTrue)return 112;
  constexpr double wanted=.6;
  TChar formatted[128]{};
  if(controller->getParamStringByValue(w.id,wanted,formatted)!=kResultTrue)
   return 113;
  const int x=scaled(w.x+w.w/2),y=scaled(w.y+w.h/2);
  mouseAction(host,x,y,true);mouseAction(host,x,y,false);
  pump();
  HWND edit=GetFocus();
  wchar_t klass[64]{};
  if(edit)GetClassNameW(edit,klass,64);
  if(!edit||_wcsicmp(klass,L"Edit")!=0){
   std::cerr<<"CTextEdit did not create native editable text field for "
            <<w.id<<" @ "<<scale<<"x\n";
   return 114;
  }
  SendMessageW(edit,EM_SETSEL,0,-1);
  for(int i=0;i<127&&formatted[i];++i)
   SendMessageW(edit,WM_CHAR,static_cast<WPARAM>(formatted[i]),1);
  pump();
  SendMessageW(edit,WM_KEYDOWN,VK_RETURN,1);
  pump();
  const double after=controller->getParamNormalized(w.id);
  if(!std::isfinite(after)||std::fabs(after-wanted)>.015){
   std::cerr<<"Native typed text parse FAIL "<<w.id<<" @ "<<scale
            <<"x expected "<<wanted<<" returned "<<after<<"\n";
   return 115;
  }
  std::cout<<"Native typed numeric edit "<<w.id<<" @ "<<scale<<"x PASS\n";
 }
 return 0;
}

int exerciseNativeMouse(HWND hwnd,IEditController* controller,double scale){
 // A native SetContentScaleFactor must adjust both editor geometry and
 // the mouse hit coordinates, not merely paint a magnified screenshot.
 for(const auto& w:kGuiSliders){
  if(controller->setParamNormalized(w.id,.14)!=kResultTrue)return 90;
  const auto before=controller->getParamNormalized(w.id);
  const auto px=[&](int a){return int(std::lround(a*scale));};
  const int y=px(w.y+w.h/2),xa=px(w.x+w.w/5),xb=px(w.x+4*w.w/5);
  dragNative(hwnd,xa,y,xb,y);
  const auto after=controller->getParamNormalized(w.id);
  if(!(std::isfinite(after)&&after>=0.&&after<=1.&&std::fabs(after-before)>.05)){
   RECT hostClient{},childClient{};GetClientRect(hwnd,&hostClient);
   int startX=xa,startY=y;
   HWND target=hitTarget(hwnd,startX,startY);
   GetClientRect(target,&childClient);
   POINT screen{xa,y};ClientToScreen(hwnd,&screen);
   POINT actualMouse{};GetCursorPos(&actualMouse);
   std::cerr<<"Native VSTGUI drag FAIL parameter "<<w.id
            <<" scale "<<scale<<" before "<<before<<" after "<<after
            <<" hostClient "<<hostClient.right<<"x"<<hostClient.bottom
            <<" targetClient "<<childClient.right<<"x"<<childClient.bottom
            <<" screen "<<screen.x<<","<<screen.y
            <<" cursor "<<actualMouse.x<<","<<actualMouse.y<<"\n";
   // Diagnose a native zoom transform mismatch: unscaled coordinates
   // are NOT accepted as a 150% PASS, but are useful for identifying a
   // plug-in that visually resizes without transforming hit regions.
   if(scale>1.){
    const int unscaledY=w.y+w.h/2;
    dragNative(hwnd,w.x+w.w/5,unscaledY,w.x+4*w.w/5,unscaledY);
    std::cerr<<"Native 1.5x diagnostic, same control with unscaled hit coords: "
             <<controller->getParamNormalized(w.id)<<"\n";
   }
   return 91;
  }
  // VSTGUI's documented Ctrl + left click resets to the parameter's
  // actual default, not necessarily the visible slider's last position.
  double expectedDefault=-1.;
  for(int32 index=0;index<controller->getParameterCount();++index){
   ParameterInfo info{};
   if(controller->getParameterInfo(index,info)==kResultTrue&&info.id==w.id){
    expectedDefault=info.defaultNormalizedValue;break;
   }
  }
  if(expectedDefault<0.)return 98;
  const int centerX=px(w.x+w.w/2);
  mouseAction(hwnd,centerX,y,true,MK_CONTROL);
  mouseAction(hwnd,centerX,y,false,MK_CONTROL);
  const double reset=controller->getParamNormalized(w.id);
  if(!std::isfinite(reset)||std::fabs(reset-expectedDefault)>1.e-4){
   std::cerr<<"Native Ctrl+left reset FAIL "<<w.id<<" @ "<<scale
            <<"x expected "<<expectedDefault<<" actual "<<reset<<"\n";
   return 99;
  }
  std::cout<<"Native mouse drag + Ctrl+left reset "<<w.id<<" @ "<<scale<<"x PASS\n";
 }
 // Both GUI one-click presets must invoke the real VST3 Controller callback.
 controller->setParamNormalized(103,0.);
 const int hwX=int(std::lround(801*scale)),hwY=int(std::lround(34*scale));
 mouseAction(hwnd,hwX,hwY,true);mouseAction(hwnd,hwX,hwY,false);
 if(std::fabs(controller->getParamNormalized(103)-.42)>1e-3){
  std::cerr<<"Native HOLLYWOOD button FAIL @ "<<scale<<"x\n";return 100;
 }
 const int orX=int(std::lround(920*scale)),orY=hwY;
 mouseAction(hwnd,orX,orY,true);mouseAction(hwnd,orX,orY,false);
 if(std::fabs(controller->getParamNormalized(103))>1e-4){
  std::cerr<<"Native ORIGINAL button FAIL @ "<<scale<<"x\n";return 101;
 }
 // The VOICE BYPASS switch must not be globally mapped to the plugin output.
 for(const auto& b:std::array<Widget,2>{{{109,797,286,171,28},{119,256,199,126,29}}}){
  const double before=controller->getParamNormalized(b.id);
  const int x=int(std::lround((b.x+b.w/2)*scale));
  const int y=int(std::lround((b.y+b.h/2)*scale));
  mouseAction(hwnd,x,y,true);mouseAction(hwnd,x,y,false);
  const double after=controller->getParamNormalized(b.id);
  if(!(std::fabs(after-before)>.5)){
   std::cerr<<"Native VSTGUI switch FAIL "<<b.id<<" scale "<<scale
            <<" before "<<before<<" after "<<after<<"\n";return 92;
  }
 }
 return 0;
}
int exerciseNativeScalingAndControls(IPlugView* view,HWND hwnd,IEditController* controller){
 // Before any scale change, the GUI must respond to physical input.
 if(const int fail=exerciseNativeMouse(hwnd,controller,1.))return fail;
 if(const int fail=exerciseNativeWheelAndText(hwnd,controller,1.))return fail;
 IPlugViewContentScaleSupport* support=nullptr;
 const bool available=view->queryInterface(IPlugViewContentScaleSupport::iid,
     reinterpret_cast<void**>(&support))==kResultTrue&&support;
 if(!available){std::cerr<<"VST3 content-scale interface missing\n";return 93;}
 ViewRect before{};
 if(view->getSize(&before)!=kResultTrue){support->release();return 94;}
 if(support->setContentScaleFactor(1.5f)!=kResultTrue){
  support->release();std::cerr<<"150 percent scaling rejected\n";return 95;
 }
 pump();
 ViewRect after{};
 if(view->getSize(&after)!=kResultTrue ||
    after.right-after.left<int((before.right-before.left)*1.4) ||
    after.bottom-after.top<int((before.bottom-before.top)*1.4)){
  support->release();
  std::cerr<<"150 percent scaling geometry NOT applied\n";return 96;
 }
 // A host view resize request is not satisfied until the *actual native
 // HWND client* is big enough. Without the WM_GETMINMAXINFO override, a
 // synthetic pointer beyond a clipped client never tests VSTGUI at all.
 RECT actualClient{};GetClientRect(hwnd,&actualClient);
 const int wantW=after.right-after.left,wantH=after.bottom-after.top;
 if(actualClient.right<int(wantW*.95)||actualClient.bottom<int(wantH*.95)){
  support->release();
  std::cerr<<"TEST HOST resize still clipped requested native GUI: "
           <<actualClient.right<<"x"<<actualClient.bottom
           <<" vs VST3 "<<wantW<<"x"<<wantH<<"\n";
  return 98;
 }
 std::cout<<"Native 150% HWND and VST3 view agree: "<<actualClient.right
          <<"x"<<actualClient.bottom<<" requested "<<wantW<<"x"<<wantH<<"\n";
 if(const int result=exerciseNativeMouse(hwnd,controller,1.5)){
  support->release();return result;
 }
 if(const int result=exerciseNativeWheelAndText(hwnd,controller,1.5)){
  support->release();return result;
 }
 if(support->setContentScaleFactor(1.f)!=kResultTrue){
  support->release();return 97;
 }
 pump();
 support->release();
 std::cout<<"Native 100/150/100 percent scaling and 18 controls + switches PASS\n";
 return 0;
}


// Real compiled VST3 processor + controller project-state integration.
// Build a version-8 state with ALL 23 normalized values and three original
// embedded WAVs; verify exact binary export, replay and controller recall.
// The resulting source WAVs never need their original disk paths.
int checkRealProjectRecall(IComponent* component,IEditController* controller){
 if(!component||!controller)return 120;
 const std::vector<std::uint8_t> wav{
  'R','I','F','F',38,0,0,0,'W','A','V','E',
  'f','m','t',' ',16,0,0,0,1,0,1,0,0x80,0xbb,0,0,0,0x77,1,0,2,0,16,0,
  'd','a','t','a',2,0,0,0,0xff,0x7f
 };
 auto checksum=[&](const std::vector<std::uint8_t>& b){
  std::uint32_t crc=0xffffffffu;
  for(auto c:b){
   crc^=c;
   for(int i=0;i<8;++i)
    crc=(crc>>1)^(0xedb88320u&static_cast<std::uint32_t>(-static_cast<std::int32_t>(crc&1u)));
  }
  return ~crc;
 };
 std::vector<char> bytes;
 auto add32=[&](std::uint32_t v){
  for(int i=0;i<4;++i)bytes.push_back(static_cast<char>((v>>(i*8))&255));
 };
 auto addFloat=[&](float f){
  std::uint32_t v=0;std::memcpy(&v,&f,sizeof(v));add32(v);
 };
 add32(8);
 std::array<float,23> expected{};
 for(int i=0;i<23;++i){
  expected[i]=.08f+static_cast<float>(i)*.035f;
  // Preserve proper discrete flags while exercising OFF/ON project recall.
  if(i==9)expected[i]=1.f; // Voice bypass
  if(i==19)expected[i]=0.f; // B1 EQ
  addFloat(expected[i]);
 }
 auto addPath=[&](){add32(0);}; // source filename deliberately absent
 auto addWav=[&](){
  add32(static_cast<std::uint32_t>(wav.size()));
  add32(checksum(wav));
  bytes.insert(bytes.end(),wav.begin(),wav.end());
 };
 addPath();addPath(); // v5 MIC and REVERB legacy path fields
 addWav();addWav();   // v7 full original source WAV payloads
 addPath();addWav();  // v8 PREAMP path and original WAV payload
 Steinberg::MemoryStream componentInput(bytes.data(),static_cast<Steinberg::TSize>(bytes.size()));
 if(component->setState(&componentInput)!=kResultOk){
  std::cerr<<"VST3 v8 embedded three-IR component setState FAIL\n";return 121;
 }
 Steinberg::MemoryStream controllerInput(bytes.data(),static_cast<Steinberg::TSize>(bytes.size()));
 if(controller->setComponentState(&controllerInput)!=kResultOk){
  std::cerr<<"VST3 v8 controller setComponentState FAIL\n";return 122;
 }
 for(int i=0;i<23;++i){
  if(std::fabs(controller->getParamNormalized(static_cast<ParamID>(100+i))-expected[i])>1e-6){
   std::cerr<<"VST3 v8 controller recall differs at parameter "<<100+i<<"\n";
   return 123;
  }
 }
 Steinberg::MemoryStream saved;
 if(component->getState(&saved)!=kResultOk||
    saved.getSize()!=static_cast<Steinberg::TSize>(bytes.size())||
    std::memcmp(saved.getData(),bytes.data(),bytes.size())!=0){
  std::cerr<<"VST3 v8 full source WAV + 23-parameter binary save differs\n";
  return 124;
 }
 // A new "project open" restores after an unrelated previous state,
 // including serialized originals that are not present as physical files.
 Steinberg::MemoryStream restored(saved.getData(),saved.getSize());
 if(component->setState(&restored)!=kResultOk)return 125;
 Steinberg::MemoryStream controllerRestored(saved.getData(),saved.getSize());
 if(controller->setComponentState(&controllerRestored)!=kResultOk)return 126;
 Steinberg::MemoryStream savedAgain;
 if(component->getState(&savedAgain)!=kResultOk||
    savedAgain.getSize()!=saved.getSize()||
    std::memcmp(savedAgain.getData(),saved.getData(),saved.getSize())!=0){
  std::cerr<<"VST3 v8 second save after project reload FAIL\n";
  return 127;
 }
 std::cout<<"Actual VST3 v8: all 23 parameters + three embedded original WAVs"
          <<" saved, recalled and binary-equivalent resaved PASS\n";
 return 0;
}


// IR GUI command test: use three already-embedded source WAVs, click the
// actual CLEAR buttons and verify their processor state physically shrinks.
// This tests GUI -> subcontroller -> VST3 message -> worker -> saved state,
// even while no process() callback or transport is running.
int exerciseNativeIRClears(HWND hwnd,IComponent* component){
 if(!component)return 128;
 Steinberg::MemoryStream initial;
 if(component->getState(&initial)!=kResultOk)return 129;
 const auto start=initial.getSize();
 if(start<3*46){std::cerr<<"Missing three embedded fixture IRs\n";return 130;}
 const std::array<Widget,3> buttons{{
  {1002,519,147,100,27}, // MIC CLEAR
  {1005,815,147,100,27}, // PREAMP CLEAR
  {1003,283,495,85,27}  // REVERB CLEAR
 }};
 auto previous=start;
 for(const auto& b:buttons){
  const int x=b.x+b.w/2,y=b.y+b.h/2;
  mouseAction(hwnd,x,y,true);mouseAction(hwnd,x,y,false);
  bool cleared=false;
  for(int retry=0;retry<100;++retry){
   pump();
   Steinberg::MemoryStream state;
   if(component->getState(&state)!=kResultOk)return 131;
   const auto now=state.getSize();
   if(now==previous-46){previous=now;cleared=true;break;}
   if(now<previous-46){
    std::cerr<<"CLEAR modified unexpected number of IRs for tag "<<b.id<<"\n";
    return 132;
   }
   Sleep(10);
  }
  if(!cleared){
   std::cerr<<"Real CLEAR button failed GUI->DSP state "
            <<b.id<<" (transport stopped)\n";return 133;
  }
  std::cout<<"Native IR CLEAR button "<<b.id
           <<" released exactly one original WAV with stopped transport PASS\n";
 }
 if(previous!=start-138)return 134;
 return 0;
}

int probe(const std::filesystem::path& path){
 std::string error;
 auto module=Module::create(path.u8string(),error);
 if(!module){std::cerr<<"Module load FAIL "<<error<<"\n";return 2;}
 HostApplication host;
 auto factory=module->getFactory();
 factory.setHostContext(&host);
 WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.lpfnWndProc=windowProc;
 wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=kClassName;
 wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);
 if(!RegisterClassExW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)
  return 3;
 int editors=0;
 for(const auto& info:factory.classInfos()){
  if(info.category()!=kVstAudioEffectClass)continue;
  auto component=factory.createInstance<IComponent>(info.ID());
  if(!component||component->initialize(&host)!=kResultOk){
   std::cerr<<"Component creation/initialization FAIL\n";return 4;
  }
  TUID cid{};
  if(component->getControllerClassId(cid)!=kResultTrue){
   component->terminate();std::cerr<<"Missing controller class FAIL\n";return 5;
  }
  auto created=factory.createInstance<IEditController>(VST3::UID(cid));
  if(!created){component->terminate();return 6;}
  IEditController* controller=created.take();
  if(controller->initialize(&host)!=kResultOk){
   controller->release();component->terminate();return 7;
  }
  if(const int result=checkRealControllerParameters(controller)){
   std::cerr<<"Real compiled VST3 controller audit failed: "<<result<<"\n";
   controller->terminate();controller->release();component->terminate();
   return result;
  }
  if(const int result=checkRealProcessingLifecycle(component.get())){
   std::cerr<<"Real compiled VST3 processor lifecycle audit failed: "<<result<<"\n";
   controller->terminate();controller->release();component->terminate();
   return result;
  }
  IConnectionPoint* cp=nullptr;
  IConnectionPoint* cc=nullptr;
  const bool componentCP=component->queryInterface(IConnectionPoint::iid,
    reinterpret_cast<void**>(&cp))==kResultTrue&&cp;
  const bool controllerCP=controller->queryInterface(IConnectionPoint::iid,
    reinterpret_cast<void**>(&cc))==kResultTrue&&cc;
  if(!componentCP||!controllerCP||cp->connect(cc)!=kResultTrue||
     cc->connect(cp)!=kResultTrue){
   std::cerr<<"Bidirectional controller/component connection FAIL\n";
   if(cp)cp->release();if(cc)cc->release();
   controller->terminate();controller->release();component->terminate();return 8;
  }
  // Seed three portable WAV sources before opening the editor, so the
  // actual CLEAR buttons can be tested against real worker-owned IR data.
  if(const int result=checkRealProjectRecall(component.get(),controller)){
   std::cerr<<"Real VST3 embedded project recall failed "<<result<<"\n";
   return result;
  }
  for(int cycle=0;cycle<kCycles;++cycle){
   auto* view=controller->createView(ViewType::kEditor);
   if(!view){std::cerr<<"Editor createView FAIL cycle "<<cycle<<"\n";return 9;}
   if(view->isPlatformTypeSupported(kPlatformTypeHWND)!=kResultTrue){
    std::cerr<<"Editor HWND support FAIL\n";view->release();return 10;
   }
   ViewRect size{};
   if(view->getSize(&size)!=kResultTrue||size.right<=size.left||size.bottom<=size.top){
    size.left=size.top=0;size.right=1000;size.bottom=615;
   }
   RECT rect{0,0,size.right-size.left,size.bottom-size.top};
   AdjustWindowRectEx(&rect,WS_OVERLAPPEDWINDOW,FALSE,WS_EX_TOOLWINDOW);
   HWND window=CreateWindowExW(WS_EX_TOOLWINDOW,kClassName,
    L"VoiceMorph QA",WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT,CW_USEDEFAULT,rect.right-rect.left,rect.bottom-rect.top,
    nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
   if(!window){std::cerr<<"Host HWND creation FAIL cycle "<<cycle<<"\n";view->release();return 11;}
   auto* frame=new HostFrame(window);
   if(view->setFrame(frame)!=kResultTrue){
    std::cerr<<"setFrame FAIL cycle "<<cycle<<"\n";return 12;
   }
   if(view->attached(reinterpret_cast<void*>(window),kPlatformTypeHWND)!=kResultTrue){
    std::cerr<<"attached(HWND) FAIL cycle "<<cycle<<"\n";return 13;
   }
   ShowWindow(window,SW_SHOWNA);UpdateWindow(window);pump();
   view->onFocus(true);view->onFocus(false);pump();
   if(cycle==0){
    if(const int result=exerciseNativeScalingAndControls(view,window,controller)){
     std::cerr<<"GUI actual mouse/scale test failure: "<<result<<"\n";
     return result;
    }
    if(const int result=exerciseNativeIRClears(window,component.get())){
     std::cerr<<"Native GUI IR clear test failure "<<result<<"\n";
     return result;
    }
   }
   if(view->removed()!=kResultTrue||view->setFrame(nullptr)!=kResultTrue){
    std::cerr<<"remove/setFrame cleanup FAIL cycle "<<cycle<<"\n";return 14;
   }
   frame->release();
   DestroyWindow(window);pump();
   view->release();
   if(IsWindow(window)){std::cerr<<"Window remained after destroy\n";return 15;}
   std::cout<<"Editor attach/detach cycle "<<(cycle+1)<<"/"<<kCycles<<" PASS\n";
  }
  cc->disconnect(cp);cp->disconnect(cc);
  cp->release();cc->release();
  if(controller->terminate()!=kResultOk||component->terminate()!=kResultOk)
   return 16;
  controller->release();
  ++editors;
 }
 if(editors==0){std::cerr<<"No VST3 Audio Effect editor to probe\n";return 17;}
 std::cout<<"Real packaged VST3 editor: "<<kCycles
          <<" attach/focus/detach/reopen cycles PASS\n";
 return 0;
}
}
int main(int argc,char** argv){
 if(argc!=2){std::cerr<<"Usage: probe <path-to-vst3>\n";return 1;}
 const auto path=std::filesystem::u8path(argv[1]);
 if(!std::filesystem::exists(path)){std::cerr<<"Bundle missing\n";return 1;}
 const HRESULT com=CoInitialize(nullptr);
 if(FAILED(com)){std::cerr<<"COM init FAIL\n";return 1;}
 const int result=probe(path);
 CoUninitialize();
 return result;
}
