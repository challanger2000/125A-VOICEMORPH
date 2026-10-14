# VOICEMORPH — implementation status (2026-10-14)

Active branch: `develop/v1.0.0`. Private Windows-x64 Studio One plugin. Current local repairs preserve the approved stationary sound reference `341fc8b`, version 0.1.0, without changing class IDs or parameter IDs.

Three IR stages, B1, pitch/formant, Voice tone/dynamics, gain, controller mappings, meters, automation and new-project state-v8 recall are implemented. Prepared state transactions, warm IR replacements, smoothing, memory guards and final-package QA replace the defective paths described in [REPAIR-REPORT.md](REPAIR-REPORT.md).

Actual local evidence: 35/35 Release CTest checks and 47/47 SDK validator checks with a real headless Linux VST3; one CTest is an expected negative check. The fixed stationary reference fixture is byte-identical at 44.1/48 kHz. These results do not establish Windows-GUI or Studio-One release readiness.

The updated Windows workflow, native GUI at 100/150 %, authoritative 125A Tester, new Studio-One project portability and real-voice listening remain to be executed. Existing reports from older commits are not evidence for these repairs. No legacy sonic compatibility is required and no commercial distribution/licensing work is in scope.
