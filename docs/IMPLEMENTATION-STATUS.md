# VOICEMORPH – implementation status (2026-10-12)

**Development branch:** `develop/v1.0.0`. **Release status:** NOT FINAL.

Working source areas:
- Windows x64 VST3 processor and controller, pinned Steinberg SDK 3.8.1.
- Stereo spectral pitch (-12 to +6 semitones) and independent formant shift (-8 to +6 st), plus vocal filters, dynamics and gain.
- Stereo/true-stereo WAV microphone and reverb IR loaders on a background thread; FFT partitioned convolution with 128-sample per-IR delay.
- Lock-free parameter state mailbox, v1-v6 state migration, MIDI-independent host automation at sample offsets, physical-value text editing.
- 32/64-bit floating-point host audio buffers and 100/150% development editor.

Verified as of Windows Actions run 38288695940: 47/47 VST3 Validator tests, 22/22 standalone tests, allocation-free C++ callback diagnostics with synthetic loaded IRs. The latest bus-routing and mono-to-stereo test changes require a new CI result; do not count them as PASS prematurely.

**Release blockers:**
1. Validated GUI master visuals and Studio One attach/open/close/reopen/zoom, host text edits and IR file selection.
2. Natural speaking-voice listening and acoustic metrics (sibilants, breath, formant coloration, consonants, noisy/unvoiced portions).
3. Full 125A Plugin Tester and host torture (PDC, offline render, latency, small buffers, channel routing, sample rate changes).
4. IR project portability: v6 saves the IR **file path**, not self-contained audio data; unavailable files cannot be recalled on another machine.
5. Safe audible transitions during IR swaps and automation, long IR memory limits, error recovery and full preset lifetime tests.
6. Full release packaging, license/demo implementation where required, installer and QA handoff.

The earlier first-commit status statement referring to a bare research DSP core is obsolete.
