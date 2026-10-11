# Release-quality acceptance gates (planned, none yet PASS)

1. Algorithm baselines: deterministic speech samples (male low pitch, differing nasal resonances; vowels, fricatives, plosives, breaths); pitch tracking, formant deviation, intelligibility, artifacts.
2. IR fixtures: delta IR null test; measured frequency/phase against offline reference convolution; 44.1/48/96/192 kHz; WAV PCM/float; mono/stereo/true stereo; project recall; invalid/missing files.
3. DSP: 0% neutral, bypass timing, gain-matched A/B, clipping/headroom, NaN/Inf, silence/denormal, parameter extremes, time-varying transitions, alias spectrum/oversampling.
4. Realtime: 32/64/128/256/512/1024 sample blocks; measure mean/p95/p99/max callback runtime and deadline overruns on realistic 11th-generation i9 CPU hardware. No disk I/O or allocations in callback.
5. VST3: Steinberg Validator **all applicable tests**; 125A Plugin Tester and event/I-O, lifecycle, offline/state/automation probes.
6. Studio One host testing: microphone monitoring, recorded and rendered speech, latency, bypass, automation, GUI reopen, two independent IR loads and saved-session restore.
7. Human listening gate: level-matched blind comparison of unprocessed and processed natural speech. Accept only if cinema/narration improvement comes without unacceptable robotic artifacts. User subjective approval required.

No synthetic test, compiled binary, Validator outcome or sonic result exists yet.

8. Gain/meter tests: +12/-12 dB transfer with tolerances, 0 dB unity, real mono input and stereo L/R output meters, peak hold/clip reset, gain placement, automation and editor lifecycle. Meter update must not block the audio thread.
