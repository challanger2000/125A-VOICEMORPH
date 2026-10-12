#pragma once
// Private-project, one-click voice presets. No extra DSP parameter, state
// version, dependencies or background processing; only existing controls.
// These values are initial listening presets, not calibrated voice models.
#include <array>
#include <cstddef>
namespace vm {
struct VoicePresetValue {int id;double normalized;};
constexpr std::array<VoicePresetValue,9> hollywoodVoicePreset(bool enabled) noexcept {
 return enabled ? std::array<VoicePresetValue,9>{{
  {102,.25}, // De-Nasal, conservative resonance reduction
  {103,.42}, // Chest
  {104,.35}, // Warmth
  {105,.08}, // Grit, subtle
  {106,.23}, // Presence / articulation
  {107,.32}, // Compression
  {108,.08}, // Low Cut, very low for deep voice
  {117,10./18.}, // Pitch -2 st
  {118,7./14.}  // Formant -1 st
 }} : std::array<VoicePresetValue,9>{{
  {102,0.},{103,0.},{104,0.},{105,0.},{106,0.},
  {107,0.},{108,0.},{117,2./3.},{118,4./7.}
 }};
}
}
