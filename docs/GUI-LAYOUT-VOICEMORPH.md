# VOICEMORPH static GUI master plan — 2026-10-13

## Source
The authoritative branding master is `challanger2000/125A-Branding/125A_Logo_Master_FINAL.svg`.
The 156x78 PNG derivative has the same viewbox aspect, path geometry,
colors and order. Keep its SVG source in this plugin repository as a
byte-identical asset; do not redraw the logo.

## Static layout, 1000×615 at 100% (1500×922.5 at 150%)
Top identity band y=0..80: 125A master logo at 12,2, centered
VOICEMORPH text x=316..684, preset buttons right.
Signal overview at y=81..105, visually read LEFT to RIGHT.
Upper signal modules at y=108..270: INPUT x=14..236,
B1 EQ x=244..394, MIC x=402..690, PREAMP x=698..986.
Full-width VOICE at x=14..986, y=278..482.
REVERB x=14..659 and OUTPUT x=667..986 at y=490..601.
Dark navy instrument panels; shared font families, colors and
consistent 8–14 px module padding; no DAW chrome, empty titles,
or unrelated controls in the voice grid.

## Interaction truth table
- INPUT: gain slider+text, independent real L/R dBFS meter, 1s clip latch
- B1: fixed ON/OFF toggle, no other correction knobs
- MIC/PREAMP: individual LOAD/CLEAR WAV + status, MIX slider+text,
  LEVEL slider+text; no route/predelay fields
- VOICE: voice-only BYPASS switch (not a host-global bypass), 9 controls
  De-Nasal/Chest/Warmth/Grit/Presence/Compression/Low Cut/Pitch/Formant,
  slider plus editable physical-unit readout
- REVERB: separate LOAD/CLEAR, MIX, LEVEL, PREDELAY
- OUTPUT: trim slider+text, real L/R dBFS meter, clip latch

## Signal and QA
Audio remains active through B1, MIC, PREAMP, REVERB and OUTPUT when
VOICE BYPASS is ON. The engine runs the bypassed voice DSP anyway so
state remains warm; dry bypass and active VOICE path are delay-aligned.
At 0% B1 correction is OFF; 100% REVERB MIX remains all wet.
The deprecated IDs 114/115/122 remain present in state but have
no visible GUI control and no longer change DSP. Previous development
projects with nonzero legacy values can sound different — disclose it.

User-facing file names are not yet persisted as individual labels;
existing IR READY/EMPTY/LOADING statuses remain visible. Do not
represent file-name recall as tested. Real Studio One GUI attachment,
Ctrl+click reset, automation and editor lifecycle must be verified
separately before release.
