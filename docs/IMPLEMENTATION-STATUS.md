# VOICEMORPH – implementation status (2026-10-12)

**Development branch:** `develop/v1.0.0`. **Purpose:** Private Studio One VST3 for Windows x64; not a commercial release.

Working source areas:
- Windows x64 VST3 processor and controller, pinned Steinberg SDK 3.8.1.
- Stereo spectral pitch (-12 to +6 semitones) and independent formant shift (-8 to +6 st), plus vocal filters, dynamics and gain.
- Stereo/true-stereo WAV microphone and reverb IR loaders on a background thread; FFT partitioned convolution with 128-sample per-IR delay.
- Lock-free parameter state mailbox, v1-v6 state migration, MIDI-independent host automation at sample offsets, physical-value text editing.
- 32/64-bit floating-point host audio buffers and 100/150% development editor.

Verified before the new target-rate changes: Windows Actions run 38293668019 had 47/47 Steinberg Validator checks and 24/24 standalone tests, with allocation-free C++ callback diagnostics. The 44.1/48 kHz rate-switch regression and worker requeue correction require a new successful CI run.

**Personal-use completion checks:**
1. Validated GUI master visuals and Studio One attach/open/close/reopen/zoom, host text edits and IR file selection.
2. Natural speaking-voice listening and acoustic metrics (sibilants, breath, formant coloration, consonants, noisy/unvoiced portions).
3. Full 125A Plugin Tester and host torture (PDC, offline render, latency, small buffers, channel routing, sample rate changes).
4. IR project portability: new state v7 embeds original WAV bytes, validates lengths and CRC32, and re-prepares IRs on the background thread. Implementation requires CI confirmation and practical Studio One save/move/reopen testing. Legacy v5-v6 path-only sessions still depend on their source WAVs.
5. Safe audible transitions during IR swaps and automation, long IR memory limits, error recovery and full preset lifetime tests.
6. Keep the Windows x64 VST3 development artifact available for manual installation; demo, licensing and Gumroad packaging are deliberately out of scope.

The earlier first-commit status statement referring to a bare research DSP core is obsolete.

**Actual usage rates:** Studio One 44,100 Hz (primary); 48,000 Hz for video. The 96-kHz dense-IR benchmark is no longer a priority. Earlier 96-kHz compatibility remains in the source code.
