# 125A VOICEMORPH – Non-AI spectral pitch and formant stage

Status: integrated development implementation; not release-ready.

Provenance: Signalsmith Stretch v1.3.2 (commit
a670068d9aeb64913331d5cc29337b19a457a7df) and Signalsmith Linear
(commit 146b26f91b48835d665fa0498d64d9617818d27a) are included under
their MIT licenses in src/third_party. The original copyright and license
texts must remain included in distributed source packages.

## Signal path

Mic IR PRE (128 samples) → spectral pitch/formant → voice tone/dynamics →
Mic IR POST (if selected) → reverb IR (128 samples). Exactly one of PRE
or POST is active; the disabled Mic placement uses the same 128-sample slot.

The optional spectral stage processes the stereo audio continuously to maintain
history, while a separately aligned dry path is selected at neutral settings.
Pitch changes are smoothed between processed and unprocessed audio over 128
samples. The entire bypass has the identical delay as the wet path.

The plugin reports 256 + spectral inputLatency() + outputLatency() samples,
recomputed during host setup for the current sample rate. IR stages always
consume 256 samples combined regardless of which IRs are active.

Bipolar plugin parameters:
- PITCH: normalized [0,1] → [-12,+6] semitones; neutral = 2/3.
- FORMANT: normalized [0,1] → [-8,+6] semitones; neutral = 4/7.
Exact near-zero floating-point defaults snap to zero semitones so nominal
neutral can remain a transparent delayed signal.
These are added at stable IDs 117 and 118, with defaults in state v6. State
v1–v5 migration leaves pitch and formants neutral; IR paths are still read
for v5+.

## Remaining quality gates (NOT VERIFIED)

- Objective end-to-end p95/p99/max and deadline misses under realistic
  speech and long dense IRs. Vendor DSP must be audited for allocations in
  its audio process path; passing isolated DSP tests is not proof of realtime safety.
- Exact input/output latency measurement on the active spectral path,
  transients and automation and corresponding Studio One PDC behaviour.
- Level-matched consonant/voice tests: de-nasal, breathing texture,
  voiced/unvoiced transitions, formant stability, metallic/phasiness artifacts,
  loudness, channel correlation, silence/tails.
- VSTGUI attach/open/close/reopen/zoom and full knob/edit text automation,
  save-reopen behaviour, 125A Release QA.

## Host sample-rate fallback (validator regression)

The Steinberg validator also calls `setupProcessing()` with 1234.568 Hz and
1,234,567.8 Hz. These rates are outside the supported spectral analysis domain
of 8,000–384,000 Hz. The plugin must still accept lifecycle setup: the spectral
processor is disabled, host PDC reports the remaining 256-sample dual-IR delay,
and the neutral/IR-only audio path stays valid. A later supported setup fully
reinitialises the pitch processor and restores the measured spectral latency.
This fallback does not claim pitch/formant processing works at those unusual rates.

## Realtime allocation and CPU probe (Windows CI)

`test_pitch_realtime` simulates 128-sample callback blocks, including the
spectral stage, stereo routing, voice tone/dynamics, two inactive IR inserts,
bypass crossfading and pitch/formant automation. Profiles at 44.1/48/96 kHz
warm up FFT/STFT state before measurement. The probe monitors C++ `operator
new` / `operator new[]` in the timed audio region and fails on any allocation.
It does **not** intercept direct `malloc` or OS allocation APIs.

Per-rate mean/p95/p99/max, audio callback deadlines and overrun counts are
printed in the Actions log. Shared GitHub runner timing is *diagnostic* and
not a release gate. The numbers include instrumented callback overhead.
Measurements with loaded long dense IRs and live Studio One speech are
still required for release.

## Dense dual-IR CPU stress fixture

The existing realtime probe now also includes dual-IR cases at 48/96 kHz:
700 ms deterministic, *fully dense* 4-channel microphone IR (true stereo)
and 700 ms 2-channel reverb IR, running at 128 samples/block. Both are
prebuilt before timing; audio callback has no IR preparation or file I/O.
The signal path includes pitch/formant, tone/dynamics, two convolutions,
automated pitch/formant and bypass changes. The profile records mean,
p95/p99/max, deadline misses, and dynamic C++ allocation count.

This fixture addresses the missing loaded-IR CPU case. Timing on shared
CI remains diagnostic; real Windows DAW p99/deadline QA is still required.
The synthesized densely populated IRs are *not* a substitute for licensed
real microphone, reverb and vocal recordings in the final listening tests.

## Host block-size burst-load diagnostic

The dense 700 ms dual-IR profile now runs with DAW-style block sizes
**16, 32, 64, 128, 256 and 512 samples at both 48 and 96 kHz**.
Because the internal uniform FFT partition is 128 samples, the computation
cost is expected to be **unevenly distributed** across very short callbacks.
Per-configuration p95/p99/max and measured callbacks exceeding the physical
block deadline are recorded. These are **diagnostics rather than a PASS/FAIL
CPU gate on shared CI**. C++ allocations in the callback remain a hard FAIL.
Before claiming realtime safety, run paced host/audio torture and address
small-block deadline misses without introducing audible timing errors or
unreported latency.
