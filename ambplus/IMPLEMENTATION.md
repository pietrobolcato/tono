# AMB+ implementation ledger

Latest: v0.3 tails, harmonic variety and AUTO return; branch ambplus/transitions-and-harmony.
Implementation c810c57093f7b151b6871dfc701ac7c005754967.
Package SHA256 90e258ec820d445a8a0b38a5b4203bc0ca1ee4c288e374761ade5b4ca7447817.
FLASH STATUS: NOT AUTHORISED. NOT HARDWARE-VALIDATED.

The previous complete native instrument/performance feedback remains implemented. v0.3 fixes AUTO's erroneous 0↔3 successor cycle with a seeded weighted anti-repeat degree bag, adds six Pad voicing pairs/eight phrase motifs/slow register movement, and prevents reattacks of a surviving Pad tone under pressure.

Committed chords and GENERATE changes now use a 1.2 s outgoing-source fade, untouched existing wet tails and a 2.4 s incoming-source fade. Voices are retired only at zero source gain, preserving the hard four-voice budget rather than doubling DSP. Dry parts are not forced wet. Global transitions leave locked manual layers alone. On PATTERN, EDIT or SEQ again visibly switches MANUAL/AUTO while retaining the manual pattern.

PASS: native controls/policies/music/transition/feedback/DSP/storage tests, four Linux sanitizer suites, 176 native UI frames, separate-process Puya v3 recall and exact-package 20-minute functional soak (one boot, no late/shed/audio_lost/full-scale samples). Same strict stress: 320 MHz PASS (75%, no late/shed), 240 MHz FAIL (96%, 3 late / 2 shed). Full upstream regression remains failed with 157 unchanged golden renders, 22 CPU-budget entries and two editor schema mismatches. No golden or budget rewrites.

Build: app 376500 B, static RAM 75852 B, RAM code 22024 B; runtime pool 303552 B / spare 15936 B. Two builds compare byte-for-byte equal. v3 1400-byte save payload and eight A/B objects unchanged; no loader/HAL/linker/flash driver changes. UC25 persistence remains refused. Owner JEDEC, two backups, exported settings, recovery and signoff remain absent. No physical device actions were run.

See TRANSITIONS_VALIDATION_REPORT.md, CONTROLS.md and ../../release/ambplus-0.3-transitions/ for current evidence, source archive, A/B WAVs, native screenshots and the filled human card. Earlier releases/evidence are preserved. External capture and fullscreen VISUAL remain deferred. Source/dependency/license pins are retained in SOURCES_LOCK.json; no Shrub materials were used. The source-confirmed encoder/absolute-pot and clock-model discrepancies remain explicit, never guessed as physical hardware identity.

Human listening and interaction acceptance remains pending. No emulator result authorizes installation.
