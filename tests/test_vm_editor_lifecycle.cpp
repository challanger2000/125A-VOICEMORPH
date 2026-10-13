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
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/vsttypes.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace VST3::Hosting;
namespace {
constexpr int kCycles=20;
constexpr wchar_t kClassName[]=L"125A_VoiceMorph_Editor_Lifecycle_Probe";
LRESULT CALLBACK windowProc(HWND hwnd,UINT msg,WPARAM w,LPARAM l){
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
  if(std::fabs(info.defaultNormalized-spec.defaultValue)>1e-5 ||
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
