# 125A VOICEMORPH — Cinematic Voice Processor

Windows x64 VST3 speech processor. **Development / not release-ready.**

## Product objective

Transform an already low-pitched but somewhat nasal speaking voice toward a natural, deep, mature, cinematic narrator sound. Preserve articulation, intelligibility and spoken timing. No mandatory cloud or AI. Optional pitch lowering must remain independent of formant control.

## Planned audio chain

Input → optional pitch/formant speech engine → adaptive de-nasal / resonances → chest / texture / presence → microphone IR → preamp IR → speech dynamics → reverb IR wet path → output.

Three independent convolution engines:
- B1 CORRECTION: fixed Studio Projects B1 inverse-high-frequency EQ, binary ON/OFF, before all IRs.
- MIC IR: optional microphone-coloration WAV, PRE by default; legacy POST still available.
- PREAMP IR: separate linear preamp-colour WAV convolution between MIC and voice processing.
- REVERB IR: mono, stereo, and (when supplied) 4-channel true stereo; pre-delay, wet/dry, level and tail.

All algorithms and parameter defaults are **provisional until measured**. Pitch and formant shifting should be evaluated against representative speech before selecting an implementation. Genuine speaker-identity conversion is out of scope for V1.

## Development

- Active branch: `develop/v1.0.0`; `main` stays stable.
- Steinberg VST3 SDK: **3.8.1 (2026-08-11)**, official source `steinbergmedia/vst3sdk`, MIT license. Dependency should be pinned to the exact release tag/commit during build integration; do not silently track HEAD.
- Follow [125A Engineering START-HERE](https://github.com/challanger2000/125A-Engineering/blob/main/START-HERE.md) and the current [125A Branding](https://github.com/challanger2000/125A-Branding).
- All new GUI work: static design and approved master assets first, then VSTGUI.
- Technical scope and test gates: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), [docs/QA-PLAN.md](docs/QA-PLAN.md).

**Status (2026-10-12):** Active VST3 development build; automated Windows CI compiles the plugin and runs 47 Steinberg Validator checks plus DSP/functional and allocation/CPU stress probes. The processor contains spectral pitch/formant shifting, voice-tone/dynamics, microphone, preamp and reverb IR load/clear, partitioned convolution, gain/peak telemetry and a development VSTGUI with 100/150% zoom. This is **not** the final approved GUI or a release. Host GUI attach/reopen, level-matched real-speech listening, long-term DAW operation, IR state portability and the final 125A release audit are not yet signed off. Check the latest GitHub Actions run for exact passing-test counts. 
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

## Simple personal quick-voice buttons

The development editor now has two one-click voice choices in its header:
**HOLLYWOOD** for a deliberately mild initial combination of existing
De-Nasal, Chest, Warmth, Presence, Compression and Low Cut, with 0 st Pitch,
-1 st Formant and no extra Grit; **ORIGINAL** returns just these nine parameters to their
neutral defaults. This is an *empirical starting preset*, not an
acoustically verified Hollywood voice-identity converter.

Both buttons edit the existing automatable parameter IDs using normal VST3
begin/perform/end notifications. They never change INPUT/OUTPUT levels,
bypass, selected WAV IRs, IR mix, levels or predelays. The individual text
fields remain accessible for small manual changes. No Hollywood parameter,
separate macro state, additional DSP, new license functionality or state
migration is added; the actual settings are recalled with the existing
project state. Studio One click and editor reopen verification is still
required.

### User-calibrated voice (private)

The speaker found -2 semitone PITCH audibly artificial and manually returned it
to 0. HOLLYWOOD therefore preserves the original pitch and disables Grit,
which added unintended gain at its previous 8% setting. These adjustments
are starting points for level-matched listening, not certified mic modelling.

A separate private B1/U87/V311 tone IR was prepared from the user's
commercially obtained IR files. Those source files and derivative WAVs are
**not** stored in this GitHub repository.

## VOICEMORPH GUI and signal-chain revision (2026-10-13)

**Private development; not final.** The fixed audible chain is
INPUT -> Studio Projects B1 EQ -> MIC IR -> PREAMP IR ->
VOICE PROCESSING [voice-only bypass] -> REVERB IR -> OUTPUT.
BYPASS affects **only** VOICE (pitch/formant, De-Nasal, Chest, Warmth,
Grit, Presence, Compression and Low Cut), never earlier mic correction,
either character IR, REVERB or OUTPUT. The prevoice bypass tap is delayed
by the exact active spectral latency and crossfaded over 128 samples;
the voice processors continue running internally while bypassed.

The three optional IR engines retain fixed 128-sample convolution
latency each, even if empty. 384 fixed + active spectral latency
is still declared to the host. The GUI shows seven sequential numbered
stages, with INPUT/MIC/PREAMP/B1 on the upper row, VOICE in the middle,
REVERB/OUTPUT on the lower row. B1 and VOICE bypass are click switches
with visible state; all active continuous controls use mouse sliders
plus direct physical-unit number edit. The two stereo L/R metering
sections use 650 ms peak hold and sample-rate-correct 350 ms release,
with 1 second clip indicators. Low Cut now supports up to 200 Hz and
0% still means OFF.

The MIC PRE/POST control (old ID 114) and MIC/PREAMP IR predelays
(old IDs 115 and 122) are **retired** as user-facing options. These IDs
remain in VST3 state for compatibility but no longer affect DSP.
Consequently an old development session using MIC POST or a non-zero
preamp/mic predelay will change sonically upon upgrade. This is an
intentional pre-release routing correction. REVERB predelay stays
available and functional. Existing WAV source bytes, user audio
and old parameter IDs remain unmodified.

The official source logo is
`vst/resource/125A_Logo_Master_FINAL.svg`, an exact copy from
`challanger2000/125A-Branding`. The bitmap
`vst/resource/125A_Logo_Master.png` is a geometry-preserving
rasterization of that master, not a letterfont or redrawn logo.
The 1000x615 GUI retains 100/150% zoom.

The new mouse/GUI layout needs **real Studio One verification**;
CI Validator/CTest PASS alone will not prove correct rendering,
knob gestures, project recall or user-level sonic acceptance.

## B1 correction and explicit signal flow (development)

The editor now numbers the live chain 01 INPUT → 02 B1 CORRECTION → 03 MIC IR → 04 PREAMP IR → 05 VOICE → 06 REVERB IR → 07 OUTPUT, with L/R input/output peak readouts and per-block clipping indication. Input and output gain are at their corresponding ends. Optional MIC POST explicitly moves MIC behind VOICE for compatibility with prior projects. B1 EQ is **documented-shape and empirically approximated** from the Studio Projects B1 response described by Bonedo, not a calibrated measurement. See [signal-chain evidence](docs/B1-SIGNAL-CHAIN.md).

Project-state v8 adds an independently embedded preamp WAV IR without renumbering existing VST3 parameter IDs. Old saved sessions leave the new B1 correction OFF. Fixed three-slot IR latency is 384 samples plus spectral engine delay; DAW PDC and v1-v7 recall remain subject to real-host verification. Build/CTests do not constitute final plugin QA.

## Reverb MIX sensitivity correction (development)

A reported 1% Reverb MIX was already very prominent with some uncalibrated
third-party WAV room IRs. Previously the FFT convolver used the original
IR amplitude verbatim, and the linear 0..100% MIX had no low-range
resolution. **The original IR is still stored unchanged in portable project
state.** Only the prepared REVERB convolution taps receive a bounded
energy-only attenuation, measured in the actual host sample-rate domain.
No boost is applied, and MIC/PREAMP coloration IRs are untouched.

MIX now uses `normalized^2` as a musically gradual wet law, with the
endpoints 0% = strictly dry, 100% = all wet. For 1% position, the wet
coefficient is 0.0001 (-80 dB relative to standardized wet before LEVEL).
The 1% control value is not 1% linear wet amplitude. LEVEL still has
its separate 0..400% scalar. Earlier development-version sessions can
sound less wet when loaded in this update: this is an intentional
correction to the existing parameter's behavior, not a new parameter ID.
The change is verified by deterministic high-energy WAV fixtures and
requires fresh Studio One listening with the actual user IR.
