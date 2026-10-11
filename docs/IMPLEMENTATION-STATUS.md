# Status of first source commit

This commit creates a standalone native C++17 **voice-DSP research core**, not a VST3 plugin.

Implemented: input/output trims -12..+12 dB, atomic peak telemetry, optional low-cut, de-nasal test peak filter, chest, warmth, presence, harmonic texture and simple dynamics.

**Not implemented:** pitch shifter, formant shifting, WAV IR loading, MIC and REVERB convolution, true-stereo routing, VST3 factory/processor/controller, editor and DAW recall.

All acoustic centers and gain curves are **EMPIRICALLY TUNED placeholders** pending analysis of representative speech, NOT validated narrator-style presets. Meters are peak-since-reset only, not yet ballistic GUI meters.

No code here is approved for release. Do not report a VST3 or Steinberg Validator PASS.

The C++ core has been locally compiled with CMake / GCC and its initial tests passed. Windows GitHub Actions status requires verification after commit.
