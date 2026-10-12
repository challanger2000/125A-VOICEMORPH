# 125A VOICEMORPH — Cinematic Voice Processor

Windows x64 VST3 speech processor. **Development / not release-ready.**

## Product objective

Transform an already low-pitched but somewhat nasal speaking voice toward a natural, deep, mature, cinematic narrator sound. Preserve articulation, intelligibility and spoken timing. No mandatory cloud or AI. Optional pitch lowering must remain independent of formant control.

## Planned audio chain

Input → optional pitch/formant speech engine → adaptive de-nasal / resonances → chest / texture / presence → microphone IR → speech dynamics → reverb IR wet path → output.

Two separate convolution engines:
- MIC IR: short/long mono/stereo microphone coloration WAV IRs; optional PRE/POST voice section.
- REVERB IR: mono, stereo, and (when supplied) 4-channel true stereo; pre-delay, wet/dry, level and tail.

All algorithms and parameter defaults are **provisional until measured**. Pitch and formant shifting should be evaluated against representative speech before selecting an implementation. Genuine speaker-identity conversion is out of scope for V1.

## Development

- Active branch: `develop/v1.0.0`; `main` stays stable.
- Steinberg VST3 SDK: **3.8.1 (2026-08-11)**, official source `steinbergmedia/vst3sdk`, MIT license. Dependency should be pinned to the exact release tag/commit during build integration; do not silently track HEAD.
- Follow [125A Engineering START-HERE](https://github.com/challanger2000/125A-Engineering/blob/main/START-HERE.md) and the current [125A Branding](https://github.com/challanger2000/125A-Branding).
- All new GUI work: static design and approved master assets first, then VSTGUI.
- Technical scope and test gates: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), [docs/QA-PLAN.md](docs/QA-PLAN.md).

**Status (2026-10-12):** Active VST3 development build; automated Windows CI compiles the plugin and runs 47 Steinberg Validator checks plus DSP/functional and allocation/CPU stress probes. The processor contains spectral pitch/formant shifting, voice-tone/dynamics, microphone and reverb IR load/clear, partitioned convolution, gain/peak telemetry and a development VSTGUI with 100/150% zoom. This is **not** the final approved GUI or a release. Host GUI attach/reopen, level-matched real-speech listening, long-term DAW operation, IR state portability and the final 125A release audit are not yet signed off. Check the latest GitHub Actions run for exact passing-test counts. 
- Normal audio arrangements: Mono→Mono, Mono→Stereo, Stereo→Stereo. Host buffers: 32-bit and 64-bit floating point (internal DSP remains float).
- Project state v7 embeds the **original WAV bytes** for both IR slots, while retaining source paths as optional provenance/fallback for v5-v6 projects. Older state v1-v6 remains readable. The embedded recall and corruption guards require successful CI and Studio One restore tests before claiming portable DAW release behavior.

## Personal use / target rates

VOICEMORPH is a **private Windows x64 Studio One project**, not a Gumroad or
commercial release. **44,100 Hz** is the primary session rate and **48,000 Hz**
is the secondary rate for video compatibility. Other host rates retain their
existing compatibility logic, but development and CPU diagnostics prioritize
44.1/48 kHz. No demo timer, license check, store packaging or macOS build is
required for the personal use case.

The `studio_one_44100_48000_rate_switch` regression checks exact compensated
neutral dual-IR impulse timing at both rates with host blocks from 16 to
512 frames, a full-octave pitch change with formant shift, and original IR
resampling in both directions. It does not replace an actual Studio One
session reopen/PDC/GUI test.
