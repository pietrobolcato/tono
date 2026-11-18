# AMB+ 0.2 performance feedback — 2026-11-18

DEVELOPMENT CANDIDATE. NOT HARDWARE-VALIDATED. FLASH STATUS: NOT AUTHORISED.

All requested feedback areas are implemented in the native firmware and exercised in software. Human musical/interaction acceptance remains pending. Timing/release acceptance remains failed; the fuller sustained bed does not remove the existing CPU limitation.

## Source and artifact

- Base: 885e385ba377a53f600ba31eecd3887f463f54ce; dedicated branch ambplus/performance-feedback.
- Implementation checkpoints: c4dbb2c0 (controls, editing, overdub, pool, glide), 7ac04790 (prompt sustained startup / mute handling), final f87a437b800fecaa7e97f43fcd5b79db95b7aaab (recorded Pad/Drone holds).
- Package SHA256: 9a214073a3090515ba411d4bf3fb72b7b42e04c690922a8b505f4bbd60792294. Package size 610113 bytes. Final rebuild compared byte-for-byte equal.
- Dependencies, compiler/SDK hashes and notices remain pinned in SOURCES_LOCK.json. SOURCE_SHA.txt identifies the source archive; later documentation commits do not change executable source.
- No HAL, loader, linker map, flash driver, sector layout or physical device command changes. No upstream auto-update, Shrub material, microphone capture or fullscreen VISUAL additions.

## Feedback behavior and evidence

See README.md's feedback matrix and CONTROLS.md. HOME macros now span wider actual tone/modulation/send controls while preserving saved deep baselines. MASTER WARMTH uses the whole-output DJ filter, rather than just the reverb-input filter. Pad/Drone factory sustain is 118/127 and base density 84%; HOME SUSTAIN changes long hold/release/activity. A prompt foundation note avoids the sparse scheduler's old 12–17 s startup wait. Muted/solo-excluded parts stop issuing generated notes. Recorded Pad/Drone melodies keep long holds too.

SOUND DETAIL exposes ENGINE/FILTER/PLAY parameter groups with actual values and normalized bars. LEDs use full visible action illumination, pending REC breathing and solid recording. HOME note-pool changes drive the real shared harmonic generator; root protection and a three-class minimum prevent invalid chords. Manual notes remain literal. Queued harmony carries sounding sustained envelopes/common tones and uses a 1.2 s pitch glide, without allocating duplicate tail voices. The LFO cursor reads trk.lfo_ph and the real shape; FX displays are labelled control-routing/response diagrams rather than decorative fake meters.

Pitched REC captures the last generated 16-slot melody before overdub, or previews a deterministic current phrase if no history exists. Running input replaces the current playback slot; stopped input advances the selected slot. The existing manual format is one note per step; a generated chord contributes its first melodic note. Restore is a guarded EDIT/MODE → EDIT → REC operation that returns only that track to its last loaded-piece state. HOME cancels. Global harmony/FX/pool and other tracks remain intact.

## Native and regression tests

PASS: full AMB+ targeted suite and new feedback tests. These exercise visible action LEDs, inactive GENERATE REC, nonempty generated capture, unchanged other steps during overdub, literal off-scale entry, shared pool → actual voicing, pool save/recall, v2 migration, per-track restore/cancel, real LFO phase source, widened macro span with baseline immutability, four-voice bound, prompt Pad/Drone startup, solo exclusion, preserved envelope during glide and sustained manual gates. Existing tests also cover 48 recipe/two PCM renders, six scenes, recording/storage recovery and non-destructive policies.

PASS: 160 native 240×240 renderer frames with zero clipping/bounds/overlap findings, including all detail groups, five LFO shapes and reset confirmation. Final package HOME, DETAIL, LFO, FX, RECORD and RESET screenshots were captured through real GPIO/encoder scripts. PNGs were visually inspected. Fixture images are separately named; the hardware-emulator LCD crop is reduced from its 2× panel display by nearest-neighbor sampling to exactly 240×240.

PASS: final Linux arm64 GCC ASan/UBSan runs of amb_feedback_test and amb_native_test. Signed-overflow/shift checks retain the upstream fixed-point DSP exclusions; leak detection is disabled. Mac's previously stalled sanitizer probe remains skipped.

The complete upstream tests/run_tests.sh was rerun in the separate AMBPLUS=0 baseline worktree. Overall FAIL: 157 golden renders unchanged, zero health/voice/routing/crash failures, but 22 host CPU-budget entries and two web editor schema failures remain. Native UI/manual checks pass. The first rerun also hit a missing Pillow interpreter; that log is preserved, and a corrected full rerun with the pinned Python environment removes that infrastructure/manual failure. No golden hashes or budgets were refreshed. Optional Emscripten/Chromium/Valgrind checks remain skipped where unavailable.

## Exact-package audio and timing

The pinned Joakim Eriksson fm1-live v0.5.1 runs the actual .fwsc. Host MIDI inputs and USB bridge are disabled. Fast emulation and FM1_STRICT=1 are separate test levels; strict mode includes instruction/cache costs, alignment faults and LCD transfer timing. No HTML instrument or independent WASM synthesizer is used.

The same 45-second Pad-solo script was applied to v0.1 and the final candidate. Ignoring the first 4 seconds, 204/409 approximately 100 ms windows were below RMS 10 in v0.1, versus 0/409 now; below RMS 100, 286/409 became 0/409. RMS increased from 601.46 to 1278.43 while peaks fell from 7787 to 6965/32767. This measures less silence for one repeatable solo trace, not universal musical quality. Both unnormalized WAVs are supplied, alongside HOME macro and queued-harmony demonstrations.

Final measured run summaries are added below from the matching ELF/RAM captures. Do not substitute the pre-foundation or pre-manual-sustain candidates retained under TEST_REPORTS/feedback/ for the delivered package.

## Resource/storage delta versus v0.1

| Quantity | v0.1 | v0.2 | Delta |
|---|---:|---:|---:|
| App / 584336 B slot | 368496 | 374184 | +5688 |
| Static RAM / 98304 B | 74400 | 75776 | +1376 |
| Runtime pool | 303552 | 303552 | 0 |
| Reported pool spare | 15936 | 15936 | 0 |
| RAM code / 25600 B | 21036 | 21452 | +416 |
| Padded package | 610113 | 610113 | 0 |

Version 3 reuses two reserved payload bytes for the relative pitch-class pool: the record is still 1400 bytes versus the existing 3840-byte capacity, with the same eight A/B objects and CRC/readback recovery. v1/v2 records migrate to their stock scale pool; saved envelopes and other musical edits are retained. v0.1 cannot read v3 records, so preserve old projects/flash data for rollback. Track restore is an in-memory operation and does not erase saved objects. Puya separate-process equality and UC25 refusal are tested; the driver chip whitelist remains unchanged.

## Open acceptance and safety gates

Human listening and comprehensive manual emulator operation remain pending. Pitch glides preserve DSP state in tests, but subjective smoothness, all possible transitions and physical audio noise/control feel are not certified. Four active layers share the existing hard four-voice budget; CPU stress can still fade voices and produce late blocks. The strict gate is not passed by an unclipped capture or a fast-emulation soak.

Upstream regression reconciliation, timing headroom and UC25 protocol support remain open. The supplied handoff identifies 240 MHz as the timing target; pinned JIANT additionally has optional native 320 MHz and boot-stage 360 MHz clock paths. A supplementary emulator test at 320 MHz is reported separately where available; it cannot establish an unknown owner's actual clock or waive the retained 240 MHz gate. No clock code or physical clock setting was modified.

NO FLASH: real owner JEDEC, two independently verified complete backups, exported settings, known-good rollback image, model-specific recovery and exact-artifact signoff are absent. The timing gate independently prevents release. Use MILESTONE_TEST_CARD.md only for emulator acceptance at this stage.

## Final measured runs (matching delivered package)

| Test | Results | Acceptance |
|---|---|---|
| 20-minute functional soak, 240 MHz fast emulation | 1200000 ms emulated in 396.63 s wall (3.03×); 52935680 frames; one boot; peak load 30%, max half 897 µs; late=0, shed=0, audio_lost=0 | PASS functional level |
| Soak output | 1199.866 s stereo, peak 8203/32767, RMS 1444.60, zero full-scale samples | Unclipped capture |
| 60-second strict heavy-FX, 240 MHz | Peak 101%, max half 2935 µs; 4 late halves / 5 shedding events; audio_lost=0; ending smoothed load 52.73% | FAIL |
| 30-second strict performance/queued harmony/pool/macros, 240 MHz | Peak 88%, max half 2567 µs; 1 late half / 1 shedding event; audio_lost=0; zero full-scale samples | Timing acceptance incomplete |
| Same 60-second strict heavy-FX trace, 320 MHz + timed LCD | Peak 73%, max half 2139 µs; late=0, shed=0, audio_lost=0; ending smoothed load 39.84%; one boot | PASS at 320 MHz test level only |
| Puya separate-process v3 save/recall | Shared relative pool 0x5af and all four layer hashes identical after restart; 13 emulator flash operations on save | PASS |
| UC25 same save actions | Zero emulator flash operations; persistence refused | Correct refusal; support BLOCKED |

The 320 MHz result shows substantial clock-dependent headroom, but it is supplementary. The current emulator launcher retains the handoff's conservative 240 MHz setting; to reproduce the alternative model, pass --mhz 320 to the pinned emulator as documented. Native JIANT exposes an optional 320 MHz setting and a boot-stage 360 MHz path (firmware/src/main.c cpu_clock and hal/fm1_clk320.h). Neither a simulator argument nor source comments identify the clock on an unknown owner's actual unit. The 240 MHz failures remain reported; no clock/flash hardware action was taken.

The final 240 MHz heavy stress is worse than v0.1's recorded 2 late / 4 shed result; the fuller sustained bed keeps voices active rather than silencing them. No claim is made that broad performance stability is resolved. The source, fresh native tests, two sanitizer suites, exact-package captures and clock-specific results provide a reviewable software candidate for the requested feedback.

Logs for pre-foundation and pre-manual-sustain checkpoints remain in TEST_REPORTS/feedback as history. Only its top-level emulator folder and the files copied here correspond to this delivered package. audio/strict.wav is the 240 MHz stress output; performance.wav contains the queued harmony/pool changes; strict320.wav is separately labelled. All are unnormalized exact-package output. No human listening score is inferred from RMS/peak measurements.
