#pragma once
// Non-realtime, validated WAV IR state blobs for portable VST3 projects.
// v7 stores the ORIGINAL source WAV bytes (not a resampled IR) so recalling
// at a different host sample rate re-prepares the same recorded impulse.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <vector>
namespace vm {
using IRSourceBytes=std::vector<std::uint8_t>;
using SharedIRSource=std::shared_ptr<const IRSourceBytes>;
constexpr std::size_t kMaxEmbeddedIRBytes=256ull*1024*1024;
struct IRBlobHeader {
 std::uint32_t size=0;
 std::uint32_t checksum=0;
};
inline std::uint32_t irCRC32(const std::uint8_t* bytes,std::size_t size) noexcept {
 std::uint32_t crc=0xffffffffu;
 for(std::size_t i=0;i<size;++i){
  crc^=bytes[i];
  for(int k=0;k<8;++k)
   crc=(crc>>1)^(0xedb88320u&static_cast<std::uint32_t>(-(static_cast<std::int32_t>(crc&1u))));
 }
 return ~crc;
}
inline bool isWavEnvelope(const std::uint8_t* data,std::size_t size) noexcept {
 return data&&size>=12&&
  std::memcmp(data,"RIFF",4)==0&&std::memcmp(data+8,"WAVE",4)==0;
}
inline bool prepareIRBlobHeader(const SharedIRSource& bytes,IRBlobHeader& h) noexcept {
 h={};
 if(!bytes||bytes->empty())return true;
 if(bytes->size()>kMaxEmbeddedIRBytes||!isWavEnvelope(bytes->data(),bytes->size()))
  return false;
 h.size=static_cast<std::uint32_t>(bytes->size());
 h.checksum=irCRC32(bytes->data(),bytes->size());
 return true;
}
// An embedded IR remains valid after its original filename is removed.
// A rate change must therefore requeue work even when the source path is empty.
inline bool needsIRRateRebuild(const std::string& path,const SharedIRSource& embedded,
                               double preparedRate,double currentRate) noexcept {
 return (!path.empty()||static_cast<bool>(embedded))&&preparedRate!=currentRate;
}
inline bool validateIRBlob(const IRBlobHeader& h,const IRSourceBytes& bytes) noexcept {
 if(h.size>kMaxEmbeddedIRBytes||h.size!=bytes.size())return false;
 if(h.size==0)return h.checksum==0;
 return isWavEnvelope(bytes.data(),bytes.size())&&
        irCRC32(bytes.data(),bytes.size())==h.checksum;
}
}
