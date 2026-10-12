#pragma once
// VST3-neutral supported host bus combinations and mono-to-stereo routing.
// Do not infer output count from input count: a one-channel spoken-word
// recording may intentionally produce two wet output channels.
namespace vm {
inline constexpr bool supportedVoiceBusLayout(int inputChannels,int outputChannels) noexcept {
 return (inputChannels==1&&outputChannels==1)||
        (inputChannels==1&&outputChannels==2)||
        (inputChannels==2&&outputChannels==2);
}
inline float rightVoiceInput(float left,float right,int inputChannels,int outputChannels) noexcept {
 return inputChannels==2?right:(outputChannels==2?left:0.f);
}
}
