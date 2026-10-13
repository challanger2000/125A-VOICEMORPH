# VOICEMORPH – Control-to-DSP Traceability Audit (development, 2026-10-13)

## Audit classification
- **STATIC TRACED**: UIDESC tag, Controller registration/event dispatch, Processor
  branch and primary downstream DSP target exist. This does not prove real mouse
  interaction, sound, host parameter delivery or visible state.
- **DSP UNIT COVERED**: specific existing pure-DSP tests exercise at least part
  of the desired behavior. Does not prove full VST3 control/host propagation.
- **HOST NOT VERIFIED**: no captured Studio One or editor-lifecycle evidence.
- **FAIL**: a reproducible contradiction or missing mandatory feature exists.

**Hard rule:** This is NOT a FULL AUDIT/GUI PASS/RELEASE PASS, despite prior
Validator 47/47, 29/29 standalone tests and CI success at head
`f5fe32649eeff05bb80b266b9b1aefff2307aff6`.
After updating UI defaults, the next build must be separately validated.

## Every visible/interactive tag and status
Each parameter below is traced from visible tag to intended Processor/DSP
behavior; all real-host interaction and state-feedback links remain
HOST NOT VERIFIED until an actual UI/controller event+render harness or a
recorded Studio One test confirms them. Every nonmeter slider has a sibling
numeric editor; binary switches have a readable bound status label.

| GUI / ID | User-facing control | Intended event→processor→audio or feedback link | Current evidence and gap |
|---|---|---|---|
| P100 / 100 | INPUT gain | Host parameter → FixedLatencyIRChain input gain before B1 | Source traced; host mouse, automation and recall NOT VERIFIED |
| P101 / 101 | OUTPUT gain | FixedLatencyIRChain output gain after reverb | Source traced; host NOT VERIFIED |
| P119 / 119 | B1 EQ ON/OFF | B1Correction before MIC and PREAMP | Voice bypass DSP regression; host ON/OFF feedback NOT VERIFIED |
| P110 / 110 | MIC MIX | StereoIRStage mic setMix; wet/dry 0…100% | Source traced; loaded IR and host control NOT VERIFIED |
| P111 / 111 | MIC LEVEL | StereoIRStage mic setWetLevel 0…4× | Source traced; host NOT VERIFIED |
| P120 / 120 | PREAMP MIX | StereoIRStage preamp setMix | Source traced; host NOT VERIFIED |
| P121 / 121 | PREAMP LEVEL | StereoIRStage preamp setWetLevel 0…4× | Source traced; host NOT VERIFIED |
| P109 / 109 | VOICE BYPASS | FixedLatencyIRChain voice-only crossfade, aligned dry branch; MIC/PREAMP/B1/REVERB remain | DSP test_voice_bypass; host switch and automated transitions NOT VERIFIED |
| P102 / 102 | DE-NASAL | VoiceDSP adaptive nasal filters | VoiceDSP unit probe; full level sweep/host NOT VERIFIED |
| P103 / 103 | CHEST | VoiceDSP body low-frequency EQ | VoiceDSP basic test; direct control/audio host NOT VERIFIED |
| P104 / 104 | WARMTH | VoiceDSP warm filter | VoiceDSP basic test; direct control/audio host NOT VERIFIED |
| P105 / 105 | GRIT | VoiceDSP nonlinear tanh saturation | VoiceDSP basic test; host/thd/noise sweep NOT VERIFIED |
| P106 / 106 | PRESENCE | VoiceDSP air filter | VoiceDSP basic test; direct control/audio host NOT VERIFIED |
| P107 / 107 | COMPRESSION | VoiceDSP envelope-based gain reduction | VoiceDSP basic test; musical sweep and host NOT VERIFIED |
| P108 / 108 | LOW CUT | VoiceDSP 35–200 Hz HPF (0% OFF) | Unit attenuation probe; host mapped physical value NOT VERIFIED |
| P117 / 117 | PITCH | PitchFormantDSP semitones −12…+6, 0 at normalized 2/3 | Pitch/formant DSP tests; GUI/pitch/formant audio host NOT VERIFIED |
| P118 / 118 | FORMANT | PitchFormantDSP semitones −8…+6, 0 at 4/7 | Pitch/formant DSP tests; GUI/host NOT VERIFIED |
| P112 / 112 | REVERB MIX | Quadratic wet taper reverbWetMix | test_reverb_mix; reset discrepancy fixed; host NOT VERIFIED |
| P113 / 113 | REVERB LEVEL | StereoIRStage reverb wet gain 0…4× | Source traced; host NOT VERIFIED |
| P116 / 116 | REVERB PREDELAY | StereoIRStage reverb 0…2000 ms | IR stage predelay unit test; host NOT VERIFIED |
| LoadMic / 1000 | LOAD MIC | IRLoaderButtons → requestIRFile(slot=0) → worker → micHandoff | File/wave IR tests; click dialog and native host NOT VERIFIED |
| ClearMic / 1002 | CLEAR MIC | IRLoaderButtons → clearIRFile(slot=0) → worker | Worker handoff test; GUI event NOT VERIFIED |
| LoadPreamp / 1004 | LOAD PREAMP | IRLoaderButtons → requestIRFile(slot=1) → preampHandoff | File/wave IR tests; GUI event NOT VERIFIED |
| ClearPreamp / 1005 | CLEAR PREAMP | IRLoaderButtons → clearIRFile(slot=1) → worker | Worker handoff test; GUI event NOT VERIFIED |
| LoadRev / 1001 | LOAD REVERB | IRLoaderButtons → requestIRFile(slot=2) → reverbHandoff | Reverb IR tests; GUI event NOT VERIFIED |
| ClearRev / 1003 | CLEAR REVERB | IRLoaderButtons → clearIRFile(slot=2) → worker | Worker handoff test; GUI event NOT VERIFIED |
| Hollywood / 1010 | HOLLYWOOD | VoicePresetButtons → 9 parameter edit gestures | Preset array + GUI contract tests; actual VST3 host edit NOT VERIFIED |
| OriginalVoice / 1011 | ORIGINAL | VoicePresetButtons → neutral voice values via host edit gestures | Preset array + GUI contract tests; host edit NOT VERIFIED |
| InputPeakL / 200 | INPUT peak L bar & dBFS | Processor dryPeak L → outputParameterChanges (read-only) | Buffer measurement traced; dynamic host display NOT VERIFIED |
| InputPeakR / 201 | INPUT peak R bar & dBFS | Processor dryPeak R → outputParameterChanges (read-only) | Buffer measurement traced; dynamic host display NOT VERIFIED |
| OutputPeakL / 202 | OUTPUT peak L bar & dBFS | Rendered output L → outputParameterChanges (read-only) | Buffer measurement traced; dynamic host display NOT VERIFIED |
| OutputPeakR / 203 | OUTPUT peak R bar & dBFS | Rendered output R → outputParameterChanges (read-only) | Buffer measurement traced; dynamic host display NOT VERIFIED |
| InputClip / 210 | INPUT clip | Processor >0dBFS input detection, 1s held flag → outputParameterChanges | Source traced; visual host NOT VERIFIED |
| OutputClip / 211 | OUTPUT clip | Processor >0dBFS output detection, 1s held flag → outputParameterChanges | Source traced; visual host NOT VERIFIED |
| MicStatus / 204 | MIC STATUS | Worker empty/loading/ready/error → readonly parameter updates | Source traced; host UI and error feedback NOT VERIFIED |
| PreampStatus / 208 | PREAMP STATUS | Worker empty/loading/ready/error → readonly parameter updates | Source traced; host UI and error feedback NOT VERIFIED |
| RevStatus / 205 | REVERB STATUS | Worker empty/loading/ready/error → readonly parameter updates | Source traced; host UI and error feedback NOT VERIFIED |

## Retired VST3 parameters – NOT GUI controls
Legacy IDs `114 MIC POSITION`, `115 MIC PREDELAY`, and `122 PREAMP PREDELAY`
continue in old state blobs for compatibility. Their active DSP pathways were
intentionally removed in the approved seven-stage concept. They no longer
advertise VST3 automation in this audit patch. Old DAW sessions that automated
these controls may not reproduce their historical audio. Full migration
acceptance remains NOT VERIFIED.

## Confirmed findings (no inference from green CI)
1. **GUI reset default contradiction – FIXED IN SOURCE, CI PENDING.**
   Reverb MIX P112 had `default-value="0"` on CSlider, but the Processor
   `params` and Controller addParameter use 0.3. Set GUI default to 0.3,
   and added a 20-control reset-default XML regression, including binary
   buttons P109=0 and P119=1. This is NOT a real Ctrl+click host test.
2. **Host-visible obsolete automation – FIXED IN SOURCE, CI PENDING.**
   Legacy positions 114/115/122 were still advertised as automatable even
   though the DSP ignores them. Keep IDs, remove automation flags, label
   deprecated. Backward project recall still needs real-host testing.
3. **VST3 tail reporting – FAIL / RELEASE BLOCKER.**
   `VMProcessor` does not override `IAudioProcessor::getTailSamples()`,
   therefore inherits `AudioEffect::getTailSamples() = kNoTail`.
   A loaded REVERB IR may produce a long audible tail after input stops.
   Without correct tail reporting, hosts can truncate offline processing or
   make wrong process-optimization decisions. Must derive a verified
   sample-rate-aware finite policy accounting for IR duration and predelay,
   and test load/clear/state-recall plus offline silence/postroll.
   Steinberg primary source:
   https://steinbergmedia.github.io/vst3_doc/vstinterfaces/classSteinberg_1_1Vst_1_1IAudioProcessor.html
4. **IR filename feedback – FAIL / UNIMPLEMENTED UX.**
   The approved plan requested loaded WAV filename display. The Editor
   currently shows only EMPTY/LOADING/READY/ERROR and no filename.
   Individual error codes 206/207/209 exist but are not visible either.
5. **Direct GUI and host integration – NOT VERIFIED.**
   CTextButton/CSlider and READONLY meters are connected by tags; actual
   clicks, native file dialogs, Controller→AudioProcessor dispatch,
   simultaneous editor reopen+automation, state reload and 100/150% Studio
   One zoom have not been exercised by the existing test suite.
6. **Per-control sound consequence – INCOMPLETE.**
   Unit tests cover some voice/DSP scenarios, but no complete independent
   programme-level A/B for all 9 voice controls at 0/25/50/75/100%, nor
   formal project-recall and audio-torture evidence.

## Completion gate (NOT VERIFIED / RELEASE BLOCKED)
- Native VST3 editor attach/detach/reopen ≥20 cycles, 100/150% zoom.
- Each of 18 continuous controls: mouse drag, numeric editing, Ctrl+left
  reset, host automation, save+reload, sonic consequence under isolated
  fixture; disable/dead-zone checks and sample-rate coverage.
- B1 toggle and VOICE BYPASS: ON/OFF/reopen/recall; prove separate IR
  stages keep processing and host output remains correctly latency-aligned.
- All 6 file buttons: native file chooser, load, clear, invalid WAV,
  status and error feedback, WAV persistence and reloading at 44.1/48 kHz.
- 4 peak meters + 2 clips: verify Controller read-only UI refresh,
  peak-hold/release, silence recovery and independent L/R behavior.
- Two presets: complete gesture/automation semantics and project reload,
  not merely const-array contents.
- Reverb tail in offline processing; Plugin Tester, I/O/event, offline/
  lifecycle/audio torture, explicit save-state and automation probes,
  NaN/denormal stress and level-matched real spoken-word listening.

## Release status
**NOT FINAL.** No user should be asked to infer successful wiring from the
GUI or from a Validator green check. Engineering must produce direct
test evidence for each remaining gate; Studio One user hearing checks
are supplemental, never a substitute for the structural audit.

## Audit expansion: control-by-control regression (2026-10-13)

Two additional CTest targets are being introduced without modifying the
existing DSP, frequencies, signal path, parameter IDs, GUI geometry, or preset
sound:

- `all_gui_controller_processor_bindings`: enumerates **all 20 active
  automatable GUI parameters** and their intended code destination, checks
  all 18 visible sliders and numeric edits, all 2 switches, all 6 IR
  load/clear actions, both preset button callbacks, and all 9 read-only
  visible metering/status identifiers, plus exact retirement of 3 inactive
  parameters. Requires the real parameter token, Controller registration and
  Processor binding rather than existence of an arbitrary XML tag.
- `isolated_voice_knob_audio_response`: drives independent sine-based
  fixtures through each of the 7 VoiceDSP controls plus two I/O trims at
  both 44.1 and 48 kHz. Requires a measurable result when each knob changes
  and 0% neutrality relative to a matched reference; also asserts reverb
  MIX law endpoints. Existing tests cover B1, reverb convolution, Voice
  Bypass, pitch/formants and IR signal stages separately.
- Updated state mailbox concurrency probe from the old 19 entries to all
  **23 current parameter-state entries** (including B1/preamp controls).

**Evidence level**: automated static/DSP regression only. It does **not**
exercise native Studio One mouse gestures, VSTGUI attachment, DAW automation,
complete real-project recall, host output telemetry, offline tail behavior
or human listening. These remain release blockers until independently tested.

## Verified audit build evidence — 2026-10-13

**Commit:** `3e6470f7e795d2dce0f66aa9e37fdcfcb7bddc30`
**Windows CI:** https://github.com/challanger2000/125A-VOICEMORPH/actions/runs/38433581288
- Steinberg SDK Validator: **47/47 passed**.
- Release CTest suite: **31/31 passed**, including newly added
  `all_gui_controller_processor_bindings` and
  `isolated_voice_knob_audio_response` (7 VoiceDSP musical controls,
  direct I/O trims; 44.1 and 48 kHz).
- State mailbox test now protects all 23 entries rather than 19.
- Windows VST3 built, C++ callback allocation probe passed, bundle staged.
- **Zero DSP, preset, GUI layout or sonic changes in this audit commit.**

### Remaining host-facing gaps, not yet a release PASS
- Input-status feedback: the Processor currently publishes IR READY/ERROR
  through `outputParameterChanges` after positive-length audio blocks.
  Its `numSamples<=0` early-return branch handles parameter flush but
  does not publish status telemetry. A host with no active audio stream
  may therefore fail to refresh IR status after LOAD/CLEAR until
  processing resumes. Verify empirically; implement a compliant
  GUI-message/status path without introducing audio-thread allocations.
- No test instantiates the real GUI and simulates 20 controller gestures
  (including defaults), 6 native file buttons, both presets, zoom or
  editor attach/detach. The CTest UI audit is a **static binding proof**,
  not GUI functionality proof.
- Loaded IR filenames and meaningful load-error detail are still absent
  from the editor. The coded status shows only EMPTY/LOADING/READY/ERROR.
- The Processor still omits VST3 `getTailSamples()`; Steinberg documents
  the need to report reverb tails for offline processing. This remains
  an explicit release blocker; latency samples are separate and MUST
  NOT be included in the tail estimate.
- Full VST3 parameter→host automation→processor→audio→project/save state
  roundtrip and lifecycle tests remain **NOT VERIFIED**.
- The user's already approved voice timbre must remain unchanged while
  completing QA and UI/host reliability repairs.
