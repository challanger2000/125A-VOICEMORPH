#include "VoiceParameterText.h"
#include <array>
#include <cassert>
#include <cmath>
#include <clocale>
#include <iostream>
#include <string>
int main(){
 constexpr std::array<double,8> values{{0.,.1,.25,.33,.5,.6666666667,.75,1.}};
 for(int id=100;id<=118;++id){
  for(double n:values){
   char label[48]{};
   assert(vm::formatVoiceControl(id,n,label,sizeof(label)));
   double restored=-99.;
   assert(vm::parseVoiceControl(id,label,restored));
   if(id==109||id==114)assert(restored==(n<.5?0.:1.));
   else assert(std::fabs(n-restored)<.015);
  }
 }
 const auto valid=[](int id,const char* label,double expected,double tolerance=1.e-5){
  double n=0.;assert(vm::parseVoiceControl(id,label,n));
  assert(std::fabs(n-expected)<tolerance);
 };
 const auto displays=[&](int id,double n,const char* expected){
  char text[48]{};
  assert(vm::formatVoiceControl(id,n,text,sizeof(text)));
  assert(std::string(text)==expected);
 };
 displays(117,2./3.,"0.0 st");
 displays(117,.6666666865348816,"0.0 st");
 displays(118,4./7.,"0.0 st");
 displays(118,.5714285969734192,"0.0 st");
 displays(117,10./18.,"-2.0 st");
 displays(118,7./14.,"-1.0 st");
 valid(117,"-12 st",0.);
 valid(117,"+0.0 st",2./3.);
 valid(117,"+6 st",1.);
 valid(118,"0 st",4./7.);
 valid(118,"-8 st",0.);
 valid(100,"-6,0 dB",.25);
 valid(100,"-6.0 dB",.25);
 valid(117,"+2,5 st",(2.5+12.)/18.);
 valid(117,"+2.5 st",(2.5+12.)/18.);
 // The same inputs must parse even when the numeric C locale is German,
 // since Steinberg hosts can run under any Windows regional settings.
 const char* locale=std::setlocale(LC_NUMERIC,nullptr);
 const std::string originalLocale=locale?locale:"C";
 const char* german=std::setlocale(LC_NUMERIC,"German_Germany.1252");
 if(!german)german=std::setlocale(LC_NUMERIC,"de_DE.UTF-8");
 if(german){
  valid(117,"+2.5 st",(2.5+12.)/18.);
  valid(117,"+2,5 st",(2.5+12.)/18.);
  valid(100,"-6.0 dB",.25);
  valid(100,"-6,0 dB",.25);
  std::setlocale(LC_NUMERIC,originalLocale.c_str());
 }
 valid(108,"OFF",0.);
 valid(108,"180 Hz",1.);
 valid(110,"35 %",.35);
 valid(111,"200 %",.5);
 valid(116,"125 ms",.0625);
 valid(114,"POST",1.);
 valid(109,"on",1.);
 valid(109,"OFF",0.);
 const auto invalid=[](int id,const char* s){
  double out=.12345;
  assert(!vm::parseVoiceControl(id,s,out));
  assert(out==.12345);
 };
 invalid(117,"7 st");
 invalid(117,"-13 st");
 invalid(118,"-9 st");
 invalid(100,"24 dB");
 invalid(102,"101 %");
 invalid(111,"401%");
 invalid(108,"33 Hz");
 invalid(110,"35 dB");
 invalid(115,"-1 ms");
 invalid(116,"1 000 ms");
 invalid(117,"abc");
 invalid(117,"NaN");
 invalid(117,"inf");
 invalid(117,"3 st junk");
 invalid(109,"MAYBE");
 invalid(114,"BACK");
 assert(!vm::formatVoiceControl(205,.5,nullptr,0));
 std::cout<<"19 editable controls: format, parse, unit and range validation PASS\n";
}
