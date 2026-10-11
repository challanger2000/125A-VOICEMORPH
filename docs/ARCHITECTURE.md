# Architecture / DSP contracts (V1 design)

## Voice processing
- Preserve pitch as independent parameter in **semitones**, nominal proposal -12..+6, 0 semitones neutral. Never treat displayed 0% as a default pitch offset.
- Formant modification independent from pitch, with intelligibility checks and no forced voice-identity mimicry.
- Algorithm evaluation: pitch-synchronous overlap/add vs complementary spectral/formant approaches. Test voiced/unvoiced transition, plosives, sibilants and breath before selecting.
- De-nasal: adaptive targeted resonances where evidenced by speech spectrum. Compare against unprocessed controls; avoid spectral overcorrection and claims of anatomical correction.
- Chest: dynamic low-mid spectral shaping, not fabricated subharmonics by default.
- Texture: conservative nonlinear harmonic generation; oversampling only if alias metrics justify it.
- Dynamics: calibrated detector, time constants and gain structure, true bypass semantics.
- Latency: report exact processing latency via SDK contract; measure actual roundtrip on representative audio and host configurations.

## Convolution
- Separate MIC and REVERB IR processors; support independent bypass and mix.
- Decode WAV off the audio thread. Initially target PCM 16/24/32 and 32-bit float; validate formats, channels, length, memory consumption.
- Internally process floating point, resample IR once off the audio thread to project rate using measured high-quality converter; retain original loaded content in state or deterministic embedded representation.
- MIC: preserve impulse shape and phase. Do not assume an IR's filename reflects measured hardware characteristics.
- REVERB: stereo and true-stereo routing requires clearly defined channel mapping and regression fixtures. Dry path must remain coherent; provide latency-aligned mix where required.
- Pre-delay is wet-only; decay shortening must not increase tail energy, but is not assumed possible for every IR without audible compromises.
- Partitioned convolution for long IRs, measured CPU vs latency tradeoff. No heavy transforms, loads, FFT planning or allocations inside audio callback.
- Parameter updates click-free; IR swaps prepared on worker thread with bounded click-free transition.
- Failed load / missing project asset must preserve usable operation and convey failure.

## VST3 / UI
- Processor/controller separation, consistent parameter IDs, state serialization and DAW automation.
- Mono input; intentional mono/stereo outputs (including mono-to-stereo reverb).
- Support 32/64-bit host sample data only as verified; sample rates and all legal block sizes.
- Ensure plugin state recalls both IRs without relying on external paths.
- Zoom 100% / 150%, Ctrl+click reset to true defaults, compliant editor lifecycle.
- Existing 125A master logo is the sole branding source.
