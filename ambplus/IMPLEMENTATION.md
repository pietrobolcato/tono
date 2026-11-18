# AMB+ implementation ledger

Latest: v0.2 performance feedback development candidate, branch ambplus/performance-feedback.
NOT HARDWARE-VALIDATED. FLASH STATUS: NOT AUTHORISED. No physical device actions were run.

Implementation source: f87a437b800fecaa7e97f43fcd5b79db95b7aaab.
Package SHA256: 9a214073a3090515ba411d4bf3fb72b7b42e04c690922a8b505f4bbd60792294.
Pinned source/dependencies/licenses: SOURCES_LOCK.json. Previous v0.1 package/source/evidence are preserved in ../../release/ambplus-0.1-development/.

## Implemented software

- Four active layers, seven roles, five HOME targets, MASTER pickup and retained target, macros, panic/mute/solo.
- Native Phosphor six EDIT/five GLO pages and action LEDs; 160-frame layout audit with no findings.
- 48 distinct audible recipes, six scenes, two original bundled PCM sources.
- Harmony/voice leading/evolution, 22 deterministic policies, literal/manual locks and recording.
- Shared real effects, sends, master EQ/protection; strict acceptance at 240 MHz remains failed.
- Eight existing CRC/A-B save objects; 1400-byte v3 payload, migration from v1/v2 and guarded overwrite/recovery. Puya separate-process custom-pool recall passes; UC25 remains refused.
- Feedback: wider HOME macro spans, whole-output warmth, high-sustain/activity Pad/Drone defaults, immediate sustained startup, HOME SUSTAIN including recorded melodies, generated capture before overdub, detailed engine/filter/play controls, visible LEDs/pool cues, live DSP-phase LFO, labelled FX diagrams, guarded per-track restore and editable shared HOME note pool.
- Queued sustained harmony keeps envelopes/common tones and glides changed pitches over about 1.2 s.

## Final checks

- PASS build and byte-identical rebuild; targeted native tests, both Linux ASan/UBSan suites, 160 UI frames, Puya v3 save/recall and final exact-package 20-minute fast functional soak.
- 240 MHz strict heavy stress: FAIL, peak 101%, 4 late / 5 shed. 30-second strict performance: 1 late / 1 shed. No audio_lost or full-scale output samples in the captures. Fuller sustained playback increases the importance of unresolved headroom.
- Additional strict 320 MHz + timed LCD test: peak 73%, zero late/shed/audio_lost. Pinned JIANT supports an optional 320 MHz path, but the handoff specifies 240 MHz. Both results remain explicit; no physical clock claim/change is made.
- Full upstream suite rerun: FAIL overall, 157 unchanged golden renders, 22 CPU-budget entries and two web-editor schema mismatches. Corrected pinned Python resolves the initial Pillow/manual infrastructure failure. No golden/budget rewrites.
- Human sound/interaction acceptance and all physical acceptance: PENDING. Owner JEDEC/backups/recovery/signoff: absent. NO FLASH.

The HAL/emulator expose quadrature KNOBS 1–4 and only MASTER/battery ADCs, resolving the handoff's contrary wiring claim. Generic absolute choice pickup/hysteresis is separately tested, not wired to invented ADC inputs. Retired STRING is replaced with functioning PHASE/FM/analog recipes. External microphone capture and fullscreen VISUAL alone remain intentionally deferred. AMB-1 helper imports retain MPL notices; no Shrub sources/assets were used.

See FEEDBACK_VALIDATION_REPORT.md, CONTROLS.md and ../../release/ambplus-0.2-feedback/ for logs, exact package, source archive, native screens, A/B WAVs, filled human card and software-only reproduction. The root handoff HTML remains a visual specification, not a playable firmware implementation.
