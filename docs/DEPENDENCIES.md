# Toolchain

- Official Steinberg VST3 SDK release: **3.8.1** (August 11, 2026), upstream https://github.com/steinbergmedia/vst3sdk
- Source of version information: https://steinbergmedia.github.io/vst3_dev_portal/pages/Versions/Index.html
- Use official SDK license and a pinned tag / resolved SHA, rather than a moving master, when wiring build.
- MSVC/Visual Studio and CMake versions should be determined from the SDK's actual release and CI runner compatibility before first build.
- No third-party voice model/FFT/resampler dependency is selected yet. Evaluate license, latency, quality and maintainability before importing source.
- Native VST3 plugin must compile and pass minimal lifecycle smoke tests before GUI implementation.
