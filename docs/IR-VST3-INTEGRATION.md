# IR VST3 integration

Three independent MIC, PREAMP and REVERB WAV stages are implemented. Each loaded or empty stage has 128 samples of aligned latency; the chain reports fixed 384 samples plus spectral-engine latency. MIC/PREAMP preserve source gain. REVERB retains its established normalization and mix law. VOICE BYPASS leaves B1 and all IR stages active.

GUI LOAD/CLEAR uses a background builder. Control-thread retirement continues even without another LOAD. Replacements prewarm and crossfade while the old stage remains audible. Failed loads retain the previous valid IR. Inactive sample-rate setup rebuilds saved IRs before audio can start and requeues unfinished user requests.

State v8 stores the original WAV bytes, lengths and CRCs for all three stages. Restore prepares every IR before accepting the state and publishes one parameter/IR transaction. A corrupted state fails without replacing the previous one. Source paths are provenance; embedded bytes support new-project portability without the source files.

Decoded input is limited to 30 seconds and 128 MiB, with a separate 128-MiB prepared-stage estimate. The estimate includes convolution spectra/history and preparation buffers; accepted durations therefore depend on channel count and rate. Budgeting does not promise freedom from operating-system allocation failures. Such failures are rejected outside the audio callback.

See [REPAIR-REPORT.md](REPAIR-REPORT.md) for measured evidence and outstanding Windows/Studio-One checks.
