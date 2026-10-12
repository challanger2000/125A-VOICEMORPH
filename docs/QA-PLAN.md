# Release-quality acceptance gates (status as of 2026-10-12)

1. Algorithm baselines: deterministic speech samples (male low pitch, differing nasal resonances; vowels, fricatives, plosives, breaths); pitch tracking, formant deviation, intelligibility, artifacts.
2. IR fixtures: delta IR null test; measured frequency/phase against offline reference convolution; 44.1/48/96/192 kHz; WAV PCM/float; mono/stereo/true stereo; project recall; invalid/missing files.
3. DSP: 0% neutral, bypass timing, gain-matched A/B, clipping/headroom, NaN/Inf, silence/denormal, parameter extremes, time-varying transitions, alias spectrum/oversampling.
4. Realtime: 32/64/128/256/512/1024 sample blocks; measure mean/p95/p99/max callback runtime and deadline overruns on realistic 11th-generation i9 CPU hardware. No disk I/O or allocations in callback.
5. VST3: Steinberg Validator **all applicable tests**; 125A Plugin Tester and event/I-O, lifecycle, offline/state/automation probes.
6. Studio One host testing: microphone monitoring, recorded and rendered speech, latency, bypass, automation, GUI reopen, two independent IR loads and saved-session restore.
7. Human listening gate: level-matched blind comparison of unprocessed and processed natural speech. Accept only if cinema/narration improvement comes without unacceptable robotic artifacts. User subjective approval required.

**Automated evidence:** Windows x64 VST3 compiled on the pinned Steinberg SDK 3.8.1 and 47/47 Validator checks plus 22/22 standalone tests passed in GitHub Actions run 38288695940. CPU/allocation measurements use synthetic audio and IR fixtures; GitHub runner deadline timings are diagnostics, not a release gate. Mono/stereo bus routing and full VST3 host interaction remain to be checked in the next CI runs. **No full 125A Release QA, editor lifecycle smoke, Studio One PDC/restore, long-running offline bounce or natural-voice listening approval yet.** The GUI is a development UI, not an approved final master.

8. Gain/meter tests: +12/-12 dB transfer with tolerances, 0 dB unity, real mono input and stereo L/R output meters, peak hold/clip reset, gain placement, automation and editor lifecycle. Meter update must not block the audio thread.
