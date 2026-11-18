# AMB+ 0.3 — transition and harmonic variety validation

2026-11-18. DEVELOPMENT CANDIDATE; NOT HARDWARE-VALIDATED. FLASH STATUS: NOT AUTHORISED.

Implementation commit c810c57093f7b151b6871dfc701ac7c005754967, branch ambplus/transitions-and-harmony, based on v0.2 documentation checkpoint e0dc1fcc. Exact package SHA256: 90e258ec820d445a8a0b38a5b4203bc0ca1ee4c288e374761ade5b4ca7447817. Final build and rebuild compare byte-for-byte equal. SOURCE_SHA.txt identifies the corresponding source archive; dependencies and licenses remain pinned in SOURCES_LOCK.json.

## Root causes and changes

The old AUTO successor table was `{3,4,5,0,1,2,0}`. Starting at degree 0 it entered the two-cycle 0→3→0; default C minor therefore alternated Cm7 and Fm7. This was AMB+'s progression-table bug, not a property of the licensed AMB-1 chord helper. GENERATE schedules rhythm/phrases/interplay, so changing it did not fix that cycle. Pad CHORD mode independently chose the same voicing positions 0 and 2 every time.

The new seeded weighted degree bag visits all seven degrees in the first seven-note-scale cycle and rejects immediate repeats and A–B–A. Native tests cover 128 changes, multiple 3–7-note pool sizes, deterministic replay, valid degrees and the actual AUTO runtime path. The default native example begins `1→2→3→6→7→4→5→2→4→1→7→3→6…`. Chords remain derived from the owner's key/note pool through the pinned AMB-1 stacked-third helper and nearby voicings. No new key/scale changes are generated implicitly.

Pad material now uses six voiced pairs, eight PHRASES-specific motif rules, inversions and slow register movement. Native rendered-note tests observe distinct traces for all eight phrase choices, multiple pairs, valid pool tones and no saved-layer mutation. A surviving Pad voice is carried under allocation pressure instead of restarting its attack when its upper note was stolen. Timbre/FX evolution offsets slew slowly rather than jumping at a bar hash change.

The old chord change only retuned voices; it did not provide an outgoing-source tail and incoming fade. New transitions fade changing sources down for 1.2 s with a smoothstep curve, leave shared delay/reverb returns untouched, retire source voices only at zero gain, then start fresh notes under a 2.4 s smoothstep fade-in. The gain is applied before dry/send splitting, never to the wet return or whole master. There are no duplicate synth voices, frozen PCM loops or new reverb engines. Dry sources simply fade without forced wet effects. Requeued changes coalesce to the latest target without a gain jump. Policy targets still commit at the four-bar boundary; audible admission follows the fade bridge. Locked manual layers are unaffected by global transitions.

PATTERN now visibly explains EDIT / SEQ to switch MANUAL/AUTO; a second SEQ press on the page performs that switch. Manual note/drum data stays stored during AUTO and is reused when returning to MANUAL. REC from AUTO remains an explicit new capture/overdub operation. Local generated pitched-mode/speed edits use the same source fade. Neither the save format nor flash map changed.

## Observed tests

- PASS: full AMB+ targeted native suite, including all prior controls/storage/48 recipe/two PCM/six-scene/non-destructive policy checks.
- PASS: new pure music tests — deterministic anti-repeat progression, all eight distinct Pad motifs, bounded/monotonic fades, correct zero crossing and requeue continuity.
- PASS: actual C mixer test — outgoing held pitches remain unchanged while the fade starts; fresh sources exist at zero incoming gain, while old wet-return energy is still 6988997 sum-of-squares over that control block. Gain-ramp code does not attenuate those wet returns. Four-bar policy commit and untouched manual payload are checked.
- PASS: manual/AUTO switching through real input handlers preserves edited notes; surviving Pad tone retains its sustain stage/envelope/pitch state under a full four-voice budget.
- PASS: Linux arm64 GCC ASan/UBSan for amb_music_test, amb_transition_test, amb_feedback_test and amb_native_test. Upstream fixed-point signed-overflow/shift exclusions and leak-check setting remain explicit in the recipe.
- PASS: 176 native 240×240 renderer frames; zero clipping/bounds/overlap findings, including manual/AUTO hints and both fade phases. Exact-package emulator phase/mode screenshots were also visually inspected.
- FAIL overall: mandatory upstream tests/run_tests.sh rerun in the AMBPLUS=0 baseline worktree — 157 unchanged golden renders, no health/voice/routing/crash failures, but 22 host CPU-budget entries and two web editor schema failures persist. No golden or budget rewrites. Optional missing Emscripten/Chromium/Valgrind and Mac sanitizer-probe limitations remain recorded.

## Exact final-package emulator measurements

Pinned official fm1-live v0.5.1, MIDI input disabled, no USB bridge. FM1_STRICT includes instruction/cache costs, alignment faults and timed LCD. Fast emulation is a separate functional level.

| Run | Observed | Result |
|---|---|---|
| 1200000 ms functional soak, fast 240 MHz model | 398.43 s wall, 3.01× realtime; 52935680 frames; one boot; peak load 31%; zero late, shed and audio_lost | PASS at functional level |
| Soak audio | 1199.866 s stereo; peak 7381/32767, RMS 1252.90; zero full-scale samples | Unclipped capture |
| 60 s strict heavy-FX trace, 240 MHz | Peak 96%; max half 2811 µs; 3 late / 2 sheds; audio_lost=0; peak sample 6611, zero full-scale samples | FAIL timing gate |
| Same strict trace, 320 MHz | Peak 75%; max half 2203 µs; zero late, shed and audio_lost; peak sample 6672, zero full-scale samples | PASS at 320 MHz test level only |
| Puya save / separate-process recall | AUTO flag, edited manual payload, custom relative pool 0x5af and all four layer hashes equal after restart; 13 emulator save operations | PASS |
| UC25 same save actions | Zero emulator flash operations; existing unsupported-chip refusal retained | Correct refusal; persistence BLOCKED |

The 20-minute trace includes repeated AUTO chord evolution, manual chord commits, multiple GENERATE changes, captured manual Pad edits, returns to AUTO and back to MANUAL, stopped save/restart/recall and resumed generation. Actual inputs are supplied under evidence/inputs. The exact native progression test establishes the anti-cycle property; the final emulator RAM/audio is not presented as a complete per-chord event log.

The A/B WAVs use identical scripted input on preserved v0.2 and this exact package, with AUTO disabled to align manual commits. Chord commits land around 14 s/28 s; a phrase change lands around 44 s. Both peaks are 7602 with zero full-scale samples; RMS is 1272.61 before and 1174.68 after. Pad voicing material differs too, so these are listening comparisons of the whole change, not an isolated causal DSP benchmark. Audio is unnormalized. Transition photos show the real LCD's OLD PARTS FADE / TAILS CONTINUE and NEW PARTS FADING IN states. Fixture images are labelled separately from hardware-emulator captures.

The handoff's 240 MHz gate is retained even though pinned JIANT also supports an optional 320 MHz path and a 360 MHz boot path. The source changes neither clock code nor a physical setting. A simulator frequency cannot identify an unknown owner's actual clock. A passing 320 MHz minute does not waive the 240 MHz failures or certify a unit.

## Resource and persistence delta from v0.2

| Quantity | v0.2 | v0.3 | Delta |
|---|---:|---:|---:|
| App / 584336 B | 374184 | 376500 | +2316 |
| Static RAM / 98304 B | 75776 | 75852 | +76 |
| RAM code / 25600 B | 21452 | 22024 | +572 |
| Runtime pool | 303552 | 303552 | 0 |
| Reported pool spare | 15936 | 15936 | 0 |
| Fixed padded package | 610113 | 610113 | 0 |

Saves remain version 3, 1400 bytes within the existing 3840-byte A/B payload capacity and same eight objects. No HAL/loader/linker/flash driver/sector changes. Fade state, current wet audio, progression-bag and transport phase are runtime state, not a saved audio snapshot. Existing musical settings, seed, pool and manual notes remain compatible with v0.2. Old v0.1 still cannot read v3 records. Licensing/source notices are preserved; no Shrub materials were used.

## Remaining acceptance

Human listening and comprehensive manual interaction acceptance remain pending. The code demonstrates tails/fades and variety, but subjective smoothness, musical taste, all gesture combinations and hardware sound/control behavior are not certified. Four active layers still share the hard four-voice DSP budget. Strict 240 MHz timing, baseline CPU/schema reconciliation and UC25 protocol support remain open.

NO FLASH. The owner has not supplied a real JEDEC ID, two independently verified full-chip backups, exported presets/settings, known-good stock image, model-specific recovery or exact-artifact signoff. Timing failures independently retain the STOP condition. MILESTONE_TEST_CARD.md is for emulator review only. No physical device was accessed.
