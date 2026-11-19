# Required feature acceptance matrix

Latest v0.6 checkpoint: PLAYING_CHECKPOINT_VALIDATION_REPORT.md. Per-feature PASS means the stated software test level; human musical acceptance remains pending.

PASS below means exercised in the stated software test level. It never means hardware certification. The overall stable release gate is BLOCKED. Only microphone capture and fullscreen VISUAL were intentionally deferred.

| Requirement | Result | Evidence / remaining scope |
|---|---|---|
| Four active layers, seven roles, five HOME targets | PASS in native C | Full fixture assigns every role in every slot, exercises routing and sample/drum DSP; exact package boots and plays |
| MASTER pickup, retained HOME target, panic, mute/solo | PASS in native C; PARTIAL acceptance | Pickup/retarget and solo persistence tests; manual physical feel and emergency low-volume test outstanding |
| Macros preserve deep baselines | PASS in native C | Parameter serialization before/after macro and generation events; shared underlying destinations |
| 48 recipes, six factory pieces | PASS signal generation; PARTIAL musical review | All 48 isolated renders have nonzero energy/distinct PCM hashes; six 12-second scene renders; human curation/listening outstanding |
| Engine-specific PATCH/DETAIL, envelope, LFO | PASS in native C | Page/role/parameter fixtures and real DSP; one control-rate LFO/layer |
| SAMPLE playback | PASS software | Two original 16000-sample companded sources and five acoustic recordings, four playback styles, looping/trim/pitch; all slots tested; read-only playback with no writable PCM sectors |
| Harmony preview/REC/auto and voice leading | PASS in native C | Chord validity, deterministic helper tests, staged bar recall and harmony scheduling |
| 8 rhythm / 8 phrase / 6 interplay policies | PASS in native C | All 22 distinct/repeatable traces; four-bar queuing/cancel; 1000-event local-state immutability comparison |
| Absolute Knobs 1–3 | BLOCKED as written; encoder equivalent implemented | Verified HAL/emulator have quadrature KNOBS 1–4, no corresponding ADCs. Separate absolute decoder tested; MASTER is actual absolute pot |
| Pitched/drum recording, off-scale notes, manual locks | PASS in native C | Step/lane/REC fixtures and serialized preservation; real GPIO scripted note/role controls in package soak |
| Shared FX, layer sends, EQ, output protection | PARTIAL | Real JIANT DSP plus master EQ, unclipped captured output; heavy-FX strict timing gate fails |
| Smooth tails / no jumps across all combinations | PARTIAL | Normal harmony/style envelopes; selected-duration release of short/long active synths before patch/role replacement, no parallel synth. Shared FX retained. Default 3 s duration is configurable. Human listening acceptance pending |
| Native Phosphor seven EDIT / five GLO pages | PASS renderer; PARTIAL interactive acceptance | 191-frame native bitmap audit at 240×240 with zero bounds/overlap findings; package LCD captures; desktop UI control was not comprehensively exercised manually |
| Contextual LEDs / scale cues | PASS in native fixture | Real HAL key map used; REC inert on GENERATE, active during recording; physical LED acceptance outstanding |
| PRESETS recall at bar / SAVE overwrite guards | PASS in native C | Staged complete recall, cancellation, selected slot overwrite and failed-write tests |
| Eight saves, CRC and versioning | PASS Puya software; BLOCKED UC25 | 1420 B vs existing 3840 B capacity; all eight A/B slots tested; separate-process Puya payload equality. UC25 has zero writes and SAVE REFUSED |
| Resource layout / no reserved-sector changes | PASS static/build | App/RAM budgets pass; HAL flash driver, linker layout, loader and package builder unchanged |
| Dense voice bound | PASS in native C | 40-note fixture enforces global cap of four |
| DSP golden / full host regression | PARTIAL / FAIL overall | 157 unchanged golden renders; 22 CPU-budget entries and two web-editor mock failures remain in baseline suite |
| Strict worst-case timing | FAIL at 240 MHz | Current canonical peak 98%, 5 late / 8 sheds, max 2856 µs; 320 MHz 74%, 0 late/shed. No physical clock inferred |
| 20-minute exact-package soak | Deferred for this checkpoint | Owner-approved short-check cadence; prior v0.5 soak is historical. Current candidate has short native/exact-package evidence and pending human playing |
| Shared HOME pitch-class pool / track restore / overdub | PASS native + exact-package coverage | Editable pool drives voicing and persists; per-track restore/cancel; REC captures generated melody and keeps other slots; v3 migration |
| Physical FM1 / chip / backups / recovery | BLOCKED | No owner JEDEC, two verified backups, exported settings, stock image or recovery/signoff evidence supplied |
| External microphone/voice capture | DEFERRED by scope | UI states unavailable; no simulated capture |
| Fullscreen VISUAL | DEFERRED by scope | Page ID reserved |

AUTO progression, 6 Pad pairs/8 motifs, natural envelopes/retained shared tails and reversible MANUAL/AUTO mode: PASS native behavioral tests and exact-package coverage; human musical acceptance pending. The source fade controller is removed. See the current report for strict timing; retained 240 MHz gate fails. Guidance is consolidated in a three-line footer with 191 passing native UI audit frames.

Current bounded release/coalescing, silent selected-step entry/divided playhead and independent persisted layer EQ: PASS native/ASan/exact-package tests. v4 migration passes in host and real-package emulator. 20-minute functional run: one boot, peak 33%, zero late/shed/audio_lost. Stronger strict 320 MHz: peak 78%, zero late/shed; retained 240 MHz gate FAIL. Human musical acceptance pending.

Current v0.6: timed GLO-style and whole-song changes, temporary HOME selected-layer level, steady noise Texture defaults and manual ARP clear/rest pass short checks. Old saved PHASE textures retain parameters. New Noise recipe IDs 2/4 require this checkpoint; older v0.5 may refuse them. No new 20-minute soak.
