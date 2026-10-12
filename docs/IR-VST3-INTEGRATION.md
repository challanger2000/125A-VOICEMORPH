# IR VST3 routing integration (work in progress)

The VST3 processor now contains independent microphone and reverb FFT convolution instances. An unloaded instance is transparent. The processing path passes left/right processed voice audio through MIC then REVERB when either instance is loaded. Global bypass remains absolute.

This change **does not yet expose a loader**, wet/dry mix or saved IR resources to the GUI/DAW. The two convolution instances therefore remain unloaded during normal use and no IR is audible. FFT latency and tail accounting must be added before any loaded IR mode can be enabled in production. This is not yet an installable end-user IR feature.
