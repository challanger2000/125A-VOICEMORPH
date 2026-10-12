#pragma once
#include "WaveIRFile.h"
#include "IRStage.h"
#include <memory>
#include <cmath>
#include <string>
namespace vm {
// Non-realtime construction API for future GUI/controller file selection.
// Prepares the complete effect privately so failures never change an active stage.
// Never call load() from VST3 process() or access stage across threads without handoff.
class IRFileBuilder {
public:
 static std::unique_ptr<StereoIRStage> load(
  const std::string& path,double sampleRate,std::size_t predelay,
  std::string& error) {
  if(!std::isfinite(sampleRate)||sampleRate<8000.||sampleRate>384000.){
   error="Unsupported host sample rate";return {};
  }
  if(predelay>static_cast<std::size_t>(sampleRate*2.)){
   error="Predelay exceeds two seconds";return {};
  }
  WaveIR wave;
  if(!loadWaveIRFile(path,wave,error))return {};
  auto stage=std::make_unique<StereoIRStage>();
  if(!stage->prepare(wave,sampleRate,predelay)){
   error="Cannot prepare IR for host sample rate";return {};
  }
  error.clear();return stage;
 }
};
}
