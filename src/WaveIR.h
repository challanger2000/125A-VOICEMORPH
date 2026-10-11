#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <limits>
namespace vm {
struct WaveIR {
 std::uint32_t sampleRate=0;
 std::uint16_t channels=0;
 std::vector<float> interleaved;
 std::size_t frames() const noexcept {return channels?interleaved.size()/channels:0;}
};
inline std::uint16_t read16(const std::uint8_t* p) noexcept {return std::uint16_t(p[0]) | (std::uint16_t(p[1])<<8);}
inline std::uint32_t read32(const std::uint8_t* p) noexcept {return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);}
inline bool fourcc(const std::uint8_t* p,const char* s) noexcept {return std::memcmp(p,s,4)==0;}
// Off-audio-thread RIFF WAV decoder. This routine never runs in process().
inline bool decodeWave(const std::uint8_t* bytes,std::size_t size,WaveIR& result,std::string& error,std::size_t maxBytes=256ull*1024*1024) {
 error.clear(); auto fail=[&](const char* msg){error=msg;return false;};
 if(!bytes||size<12||size>maxBytes)return fail("Invalid or oversized WAV");
 if(!fourcc(bytes,"RIFF")||!fourcc(bytes+8,"WAVE"))return fail("Not RIFF/WAVE");
 const auto containerSize=std::size_t(read32(bytes+4))+8;
 if(containerSize>size||containerSize<12)return fail("Truncated RIFF");
 std::uint16_t format=0,channels=0,bits=0,align=0;
 std::uint32_t sampleRate=0,byteRate=0;
 const std::uint8_t* audio=nullptr;std::size_t audioSize=0;
 bool hasFormat=false;
 for(std::size_t pos=12;pos+8<=containerSize;){
  auto chunkBytes=std::size_t(read32(bytes+pos+4));
  auto dataPos=pos+8;
  if(chunkBytes>containerSize-dataPos)return fail("Truncated chunk");
  auto* p=bytes+dataPos;
  if(fourcc(bytes+pos,"fmt ")){
   if(chunkBytes<16)return fail("Invalid fmt chunk");
   format=read16(p);channels=read16(p+2);sampleRate=read32(p+4);
   byteRate=read32(p+8);align=read16(p+12);bits=read16(p+14);
   if(format==0xfffe){
    if(chunkBytes<40||read16(p+16)<22)return fail("Bad extensible fmt");
    const std::uint8_t pcmGuidTail[12]={0,0,0x10,0,0x80,0,0,0xaa,0,0x38,0x9b,0x71};
    if(std::memcmp(p+28,pcmGuidTail,12)!=0)return fail("Unsupported extensible GUID");
    auto sub=read32(p+24);
    if(sub!=1&&sub!=3)return fail("Unsupported extensible codec");
    format=static_cast<std::uint16_t>(sub);
   }
   hasFormat=true;
  } else if(fourcc(bytes+pos,"data")&&!audio) {audio=p;audioSize=chunkBytes;}
  auto advance=8+chunkBytes+(chunkBytes&1);
  if(advance>containerSize-pos)return fail("Invalid chunk padding");
  pos+=advance;
 }
 if(!hasFormat||!audio)return fail("Missing fmt/data");
 if(channels<1||channels>4||sampleRate<8000||sampleRate>384000)return fail("Unsupported channel count or rate");
 if(!((format==1&&(bits==16||bits==24||bits==32))||(format==3&&bits==32)))return fail("Unsupported WAV encoding");
 const std::size_t bytesPerSample=bits/8;
 if(align!=channels*bytesPerSample||byteRate!=sampleRate*align)return fail("Invalid WAV alignment");
 if(audioSize==0||audioSize%align)return fail("Invalid WAV sample size");
 if(audioSize>maxBytes)return fail("IR exceeds memory budget");
 const auto samples=audioSize/bytesPerSample;
 std::vector<float> decoded;decoded.reserve(samples);
 for(std::size_t i=0;i<samples;++i){
  const auto* p=audio+i*bytesPerSample;float v=0;
  if(format==3){std::uint32_t word=read32(p);std::memcpy(&v,&word,4);if(!std::isfinite(v))return fail("Nonfinite float IR");}
  else if(bits==16){v=static_cast<std::int16_t>(read16(p))/32768.f;}
  else if(bits==24){std::int32_t n=std::int32_t(p[0])|(std::int32_t(p[1])<<8)|(std::int32_t(p[2])<<16);if(n&0x800000)n|=~0xffffff;v=n/8388608.f;}
  else {auto raw=read32(p);auto n=static_cast<std::int64_t>(raw);if(raw&0x80000000u)n-=0x100000000ll;v=static_cast<float>(n/2147483648.0);}
  decoded.push_back(v);
 }
 WaveIR next;next.channels=channels;next.sampleRate=sampleRate;next.interleaved=std::move(decoded);
 result=std::move(next);return true;
}
}
