#include "IRFileBuilder.h"
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <string>
int main(){
 const std::string path="builder_test_ir.wav";
 const unsigned char bytes[]={'R','I','F','F',38,0,0,0,'W','A','V','E',
 'f','m','t',' ',16,0,0,0,1,0,1,0,0x80,0xbb,0,0,
 0,0x77,1,0,2,0,16,0,'d','a','t','a',2,0,0,0,0,0x40};
 {std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<const char*>(bytes),sizeof(bytes));}
 std::string error;
 auto valid=vm::IRFileBuilder::load(path,48000,0,error);
 assert(valid&&valid->ready()&&error.empty());
 float left=0,right=0;
 for(int n=0;n<140;++n){
  valid->process(n==0?1.f:0.f,0.f,left,right);
  if(n==128)assert(left>.49f&&left<.51f);
 }
 auto bad=vm::IRFileBuilder::load(path,4000,0,error);
 assert(!bad&&!error.empty());
 assert(valid->ready());
 std::remove(path.c_str());
 auto missing=vm::IRFileBuilder::load(path,48000,0,error);
 assert(!missing&&!error.empty());
 assert(valid->ready());
}
