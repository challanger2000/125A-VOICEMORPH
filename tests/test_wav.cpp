#include "WaveIR.h"
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include <cmath>
#include <iostream>
using namespace vm;
static void add16(std::vector<std::uint8_t>& a,std::uint16_t x){a.push_back(x&255);a.push_back(x>>8);}
static void add32(std::vector<std::uint8_t>& a,std::uint32_t x){for(int i=0;i<4;++i)a.push_back((x>>(8*i))&255);}
static std::vector<std::uint8_t> make(std::uint16_t bit,std::uint16_t codec,int channels=1){
 std::vector<std::uint8_t> a={'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' '};
 add32(a,16);add16(a,codec);add16(a,channels);add32(a,48000);add32(a,48000*channels*(bit/8));add16(a,channels*(bit/8));add16(a,bit);
 a.insert(a.end(),{'d','a','t','a'});add32(a,channels*(bit/8)*2);
 for(int frame=0;frame<2;++frame)for(int ch=0;ch<channels;++ch){
  if(codec==3){float x=frame==0?.5f:-.5f;std::uint32_t y;std::memcpy(&y,&x,4);add32(a,y);}
  else if(bit==16)add16(a,frame==0?16384:49152);
  else if(bit==24){std::uint32_t x=frame==0?0x400000:0xc00000;for(int j=0;j<3;++j)a.push_back((x>>(j*8))&255);}
  else add32(a,frame==0?0x40000000u:0xc0000000u);
 }
 auto n=std::uint32_t(a.size()-8);for(int i=0;i<4;++i)a[4+i]=(n>>(i*8))&255;
 return a;
}
int main(){
 for(auto pair:{std::pair<int,int>{16,1},{24,1},{32,1},{32,3}})for(int ch:{1,2,4}){
  auto bytes=make(pair.first,pair.second,ch);
  WaveIR out;std::string error;
  assert(decodeWave(bytes.data(),bytes.size(),out,error));
  assert(out.sampleRate==48000&&out.channels==ch&&out.frames()==2);
  assert(std::fabs(out.interleaved[0]-.5f)<.0001f);
  assert(std::fabs(out.interleaved[ch]+.5f)<.0001f);
  auto old=out;bytes.pop_back();
  assert(!decodeWave(bytes.data(),bytes.size(),out,error));
  assert(out.interleaved==old.interleaved);
 }
 auto b=make(24,1);WaveIR out;std::string e;
 b[0]='X';assert(!decodeWave(b.data(),b.size(),out,e));
 std::cout<<"WAV IR decode PASS\n";
}
