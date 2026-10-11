#pragma once
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"
namespace Steinberg {
static const FUID kVMProcessorUID(0xFB9F1301,0x5F2946D1,0xA1F90017,0x0B11A009);
static const FUID kVMControllerUID(0xFB9F1302,0x5F2946D1,0xA1F90017,0x0B11A009);
enum VMParameter : Vst::ParamID {
 kInput=100,kOutput=101,kDeNasal=102,kChest=103,kWarmth=104,
 kGrit=105,kPresence=106,kCompression=107,kLowCut=108,kBypass=109
};
constexpr int kParameterCount=10;
}
