# Meter telemetry note

VST3 block telemetry now uses actual post-chain output buffers, including loaded FFT IR processing and bypass.
Normal processing input peak is measured after input trim in the DSP; bypass uses dry source.
The current input path must be verified for in-place processing and should not infer unprocessed source samples after overwriting.
Read-only host meter parameters use normalized linear peaks, not calibrated dBFS display; GUI needs explicit log conversion.
