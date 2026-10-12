# Pitch/Formant engine experiment (non-AI)

This is a deliberately isolated DSP prototype, not yet connected to the VST3 controller,
GUI or audio routing. It must not be advertised as a finished voice changer.

The pitch/formant implementation uses the MIT-licensed
[Signalsmith Stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch)
(version 1.3.2, pinned upstream commit a670068d9aeb64913331d5cc29337b19a457a7df)
and its [Signalsmith Linear](https://github.com/Signalsmith-Audio/linear)
dependency (commit 146b26f91b48835d665fa0498d64d9617818d27a).
The original license notices are preserved in src/third_party/.

Parameters tested: pitch -12..+6 semitones; independent spectral-formant movement
-8..+6 semitones, with formant compensation enabled. The spectral algorithm is
not a PSOLA replacement; consonants, breathing, timbre and transients still
require measurement and listening. Tonal tests do not establish speech quality.

The analysis window is approximately 50 ms with one-quarter hop, and split
computation is enabled. Exact input + output latency is queried at runtime.
Before connecting this to the plug-in, adjust host PDC, align dry/processed
paths, measure callback p95/p99/max, add real-voice fixtures and test state/GUI.
