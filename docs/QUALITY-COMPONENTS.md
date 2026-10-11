# VoiceMorph: engineering-quality component selection

Status: Design choices subject to **measured** evaluation; not yet implemented or acoustically approved.

## Current verified source versions
- Steinberg VST3 SDK **3.8.1**, released 2026-08-11. GUI source: SDK VSTGUI 4.15.1. Keep pinned to verified SHA `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96` already used by the successful GitHub build.
- Signalsmith Stretch is a modern C++ pitch/time candidate with formant shifting/compensation and permissive MIT licensing. Benchmark speech-specific quality, objective latency, CPU tail deadlines and voiced/unvoiced artifacts before selecting or integrating it.
- Rubber Band is an alternative *research comparison*, with GPL/commercial license implications; do not copy GPL code into a closed-source product.
- Dedicated pitch-synchronous speech methods (PSOLA-type) remain a comparison candidate for monophonic narration. Evaluate independently on natural lower-register speech.
- High-quality sample-rate conversion candidate: r8brain-free-src (MIT). Evaluate filter response, latency and IR alignment before inclusion.

## Required topology
Speech input gain → low-cut → vocal pitch + formant (independent) → evidence-based adaptive de-nasal → chest/texture/presence → MIC convolution → speech dynamics → parallel REVERB convolution → output gain → true input/output meters.

- PITCH in semitones, 0 st neutral; FORMANT independently adjustable; never substitute pitch-synchronous conversion with a naïve resampling speed change.
- De-nasal cannot assume every voice has an unwanted resonance at exactly 920 Hz. Adaptive filtering requires tracked, bounded candidates and before/after measurements on nasalized *and* clean voice.
- Dynamic voice processing should use calibrated detection/time constants and avoid obvious pumping/breath exaggeration. Nonlinear harmonics must be screened for aliasing.
- MIC IR: accurate full-length convolution and optional PRE/POST routing, preserve exact impulse response except intentional rate adaptation.
- REVERB IR: uniform/non-uniform partitioned convolution, proper latency accounting, mono/stereo and true-stereo with four routing filters where material supports it. Wet-only pre-delay and phase-coherent dry alignment.
- Load WAV and prepare FFT plans and resampling exclusively outside audio callbacks. Ensure fully self-contained state/preset IR recall rather than paths that may break.
- No arbitrary IR truncation: any memory/length limit must be surfaced as a documented tested engineering constraint.
- Proper mono-to-stereo host routing, reporting, sample-accurate/musical automation and sample-rate change behavior.
- GUI: VSTGUI 4.15.1, authoritative 125A master assets, 100/150% verified layouts, clearly calibrated dBFS peak meters and reset.

## Release acceptance
1. Pitch step/preservation and formant shift measured separately, time-domain transient and intelligibility checks.
2. Conv FFT output vs direct reference for impulse/white noise/real WAV IR, across block sizes, 44.1/48/96/192 kHz and mono/stereo/true-stereo.
3. No allocations, locks, filesystem operations, long unpredictable transforms or sleeps in `process()`; CPU deadline p95/p99/max benchmarked on actual target Windows CPU.
4. Correct latency/tail/bypass, state recall, silence, denormals and GUI lifecycle under validator, 125A tester and Studio One.
5. Final level-matched real-voice listening gate: distinct audible cinematic improvement without unacceptable robotic/metallic/nasal artifacts.

Sources:
- https://steinbergmedia.github.io/vst3_dev_portal/pages/Versions/Version%2B3.8.1.html
- https://github.com/Signalsmith-Audio/signalsmith-stretch
- https://signalsmith-audio.com/code/stretch/
- https://github.com/syshlted/timepitch-bench

Existing `src/VoiceDSP.h` and `src/Convolution.h` are **research baselines only**. Their fixed de-nasal EQ and short direct FIR are not substitutes for the release architecture above.
