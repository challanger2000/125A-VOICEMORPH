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
