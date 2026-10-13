# VOICEMORPH B1 corrective EQ and explicit signal route (development)
## Scope
Studio Projects B1 with -10/-20 dB PAD and 75/150 Hz low-cut switch,
NOT a Behringer B-1. The reported capsule response from Bonedo's 2009 test
rises mildly from ~2 kHz and peaks near +5 dB around 10 kHz.
Reference: https://www.bonedo.de/artikel/studio-projects-b1/

## Correction evidence
B1 Correction ON uses a fixed broad reciprocal shaping filter with
center 8500 Hz, Q=0.6 and peak gain -5.0 dB. This is a **documented shape**
and **empirically tuned approximation** only, not measured B1 sample response.
OFF is intended to be exactly neutral. Independent correction before any
optional microphone/emulation IR. No global treble cut, no strength dial.

## Default signal path
INPUT gain -> B1 correction -> MIC IR -> PREAMP IR -> PITCH/VOICE DSP ->
REVERB IR -> OUTPUT gain.
PREAMP WAV IR is a **linear** preamp frequency/phase response, not
harmonic distortion, saturation or actual electronic circuit modelling.
Three IR slots independently load/save/clear original WAVs. Empty slots
retain the same internal delay as loaded slots. Fixed three-IR delay:
384 samples + separate spectral latency. Host PDC/realtime measurements
must be redone.

Existing Mic Position (parameter 114) is retained. Default of new
sessions is PRE. POST routes MIC IR after VOICE and before REVERB; this
exception is explicitly shown in the editor so the diagram is accurate.
An older project retains its stored parameter 114 value; the B1 correction
defaults to OFF for state versions 1..7 so old projects are not
silently given a new tonality. Cross-component timbre and PDC for
old sessions with non-neutral gain need explicit host comparison before
backward-compatibility can be declared.

## Parameter/state contract
Pre-existing parameter IDs 100..118 unchanged.
New automatable IDs: 119 B1 Correction (ON/OFF), 120 Preamp IR Mix,
121 Preamp IR Level, 122 Preamp IR Predelay.
New read-only IDs: 208/209 Preamp status/error, 210/211 input/output clipping.
Processor state v8 appends independent preamp WAV path and original WAV
source bytes with existing CRC checks after the complete previous v7 format.
Old v1..v7 restores must be tested in both Processor and Controller.

## Metering and GUI contract
A numbered left-to-right SIGNAL route appears in the title strip.
Panels use those same 01..07 numbers. Input and output show independent
L/R peak bars, dBFS readouts and CLIP status from the real host buffer
rather than parameter positions. Input is before trim; output is after
the last DSP stage including bypass. Numeric readouts/meter bars use
existing read-only output parameters 200..203.

Release QA gates pending: correct real Studio One meter dispatch at
44.1/48 kHz, state/migration, PDC, IR load/clear/reload for all 3 slots,
click and automation of B1 ON/OFF and PRE/POST, visible zoom 100/150,
20 open/reopen cycles, realtime tail latency and level-matched voice
listening. A passing build does not establish a final release.
