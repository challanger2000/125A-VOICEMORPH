#pragma once
#include <algorithm>
#include <cmath>
namespace vm {
inline double meterToDbfs(double normalized) noexcept {
 if(!std::isfinite(normalized)||normalized<=0)return -120.0;
 return std::max(-120.,20.*std::log10(std::min(1.,normalized)));
}
inline double dbfsToMeter(double db) noexcept {
 if(!std::isfinite(db)||db<=-120)return 0.;
 return std::clamp(std::pow(10.,db/20.),0.,1.);
}
}