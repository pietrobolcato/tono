# AMB+ 0.6 playing checkpoint — 2026-11-18

Implementation 596d2d151132921da807cb3fb44b5ff7df66c1fe, branch ambplus/uniform-transition-time; based on v0.5 implementation 6c6b487bd60937af1c39867387dfdb1cf41b5c7f. Exact artifact SHA256 **c1c710c17a3ee18382d24e9a33241b295553448ed9b1276952900daf68ad3a01**. Rebuild matched byte-for-byte. Native firmware runs in the pinned fm1-live v0.5.1 emulator; HTML remains a visual specification. **FLASH STATUS: NOT AUTHORISED.** No physical FM1 access or installation.

## Requested behavior

Active synth releases use the selected **TAIL TIME** for short and long sounds (250–6000 ms, default 3000). A snapshot of the current voice envelope falls smoothly to zero; it never boosts the initial level, duplicates the synth, freezes an audio buffer or adds an incoming source fade. FM6 operator/pitch envelopes are held while real oscillator phases continue. Finished one-shots cannot be recreated from silence. Realtime overload protection can still shorten voices under stress. Normal note-off/AUTO chord playback retains its existing envelopes.

Queued RHYTHM/PHRASE/INTERPLAY commits run this transition only on generated layers, at the four-bar boundary. Manual payloads remain unchanged. Factory/saved-piece recalls coordinate all incoming layers at the next bar, retaining outgoing master/EQ/FX settings until sources finish; then the new piece starts. A change of shared reverb model still uses native JIANT model replacement; no parallel master effect is claimed.

HOME temporarily shows the selected layer name, gain percentage and bar while MASTER changes that layer. It returns to MASTER after 1200 ms without an accepted gain change, or immediately on retargeting. No-op ADC readings do not extend it. Soft pickup and last-HOME-target audio routing are unchanged.

Fresh Texture recipes use unpitched NOISE, sustained envelopes, density 100 and STEADY AUTO: continuous fixed-note noise despite sparse conductor events, with no repeated note retrigger. Other modes permit gated patterns. Manual empty steps release Texture normally. ARP clears the selected manual pitched step; its context hint/LED appears when useful. Silent entry and EDIT/SEQ AUTO return remain. Factory Glass Wind/Distant Voices become High Air/Distant Hiss. Existing saved PHASE textures keep parameters/names; fresh selections use the new palette.

## Short-check evidence

- PASS complete AMB+ targeted suite: controls, policies, music, all roles/patches, scenes, storage recovery, feedback, normal playback, handoff, minimum-duration/whole-song/GLO/header cases, Texture and renderer. **191 native frames at 240×240; zero clipping, bounds or overlap findings.** Exact-package HOME screenshots verify layer feedback and expiration; live piece screenshot confirms High Air and selected-layer gain.
- PASS Bass → Drums via actual ALGORITHM/REC controls at 250, 500, 3000 and 6000 ms. Nominal 3 s test: 132352 frames versus minimum 132300, within a control block plus handoff dispatch. Dry source energy remains late in the release. FM6, rapid coalescing, unchanged short ordinary note-off, manual pattern preservation, four-bar PHRASE, coordinated song FX/master/EQ and header timeout/no-op/retarget assertions pass.
- PASS all eight fresh Texture recipes use NOISE; 20 s native steady playback has one continuously held voice despite sparse policy. Manual clear/rest and old PHASE save/recall preservation pass. Texture recipe audio changes are intentional; no upstream golden snapshots or budgets were rewritten.
- PASS Linux ASan/UBSan for minimum-release, Texture and complete native fixtures; corrected-envelope fixture rerun afterward. Upstream signed-overflow/shift exclusions and disabled leaks retained; Mac sanitizer probe remains skipped for its recorded baseline hang. A short exact-package A/B caught a Q24×Q15 scaling overflow in the initial candidate; corrected with wide-safe fixed-point multiplication and full-/half-level regression checks before delivery.
- Mandatory full baseline host suite: FAIL overall, **157 unchanged goldens, zero health/voice/routing failures/crashes, 22 inherited CPU-budget entries and two inherited web-editor schema failures**. Baseline source e4e346a3f74a236ab286349a6a9bff68f3ed8a31 with pinned host support. No emcc; no separate interactive web synth build. Host installer cases are mocks.

## Exact firmware runs

| Run | Peak | Late | Shed | audio_lost | Max half |
|---|---:|---:|---:|---:|---:|
| 15 s isolated Bass → Drums A/B, 240 MHz fast | 20% | 0 | 0 | 0 | see JSON |
| 90 s feature checkpoint, 240 MHz fast | 34% | 0 | 0 | 0 | 1000 µs |
| 60 s canonical strict, 240 MHz | 98% | 5 | 8 | 0 | 2856 µs |
| 60 s canonical strict, 320 MHz | 74% | 0 | 0 | 0 | 2164 µs |

The current strict **240 MHz gate FAILS**, as before; compare v0.5 canonical 98%, 4 late / 11 shed / max 2845 µs, without claiming a performance pass. 320 MHz is an additional model setting, not the owner's verified clock. Default clock unchanged. audio_lost=0 is not permission to dismiss late halves/shedding.

Corrected isolated dry Bass A/B RMS in 300 ms windows starting 0, 0.5, 1, 1.5, 2, 2.5, 3 seconds after commit: **2311.978, 2073.162, 1626.789, 1088.811, 548.058, 108.304, 0.0**. Previous v0.5 was already silent from the 0.5 s window onward. Same script, no chorus/delay/reverb sends, unnormalized PCM. Corrected Bass peak 3928; feature run peak 4785, no full-scale samples. Captures are actual .fwsc audio, not a simulated or frozen tail.

The feature trace explicitly selects L1 after stopped factory recall resets HOME to MASTER. It exercises timed Bass/Drums, PHRASE, playing factory recalls, steady Texture, High Air selection, silent capture/manual clear/AUTO return and layer-gain adjustment. RAM decoding confirms Texture/High Air/NOISE/AUTO afterward. An earlier trace missed reselection and is not counted as evidence for those later interactions.

Per owner preference, **no new 20-minute soak** for this incremental checkpoint. Targeted tests and 15–90 s exact-package runs plus short strict checks precede human playing. Longer functional soaks are reserved for milestones or stability/timing investigations; the prior v0.5 soak is historical and not evidence for this hash.

## Resources and saves

App **379892 B** (+1568 vs v0.5) of 584336; static RAM **76320 B** (+204) of 98304; RAM code **22552 B** (+272) of 25600. Pool 303552 B, spare 15936 B, package 610113 B: unchanged. Layout and HAL register checks pass. Only envelope snapshots/counters added, no new playable layer or audio buffer. No flash driver, loader, bootloader, sector, reserved bounds or physical clock changes.

Save layout remains **v4, 1420 B** within the same 3840-byte A/B capacity. Separate-process Puya save/recall matches complete pieces including EQ, duration, pool and notes. Existing legacy PHASE texture cases pass host serialization/recall. Older v0.5 may reject newly mapped NOISE Texture recipes 2/4; retain separate old state/exports for rollback. UC25HQ80 remains SAVE REFUSED with zero flash operations; Puya persistence passes. No guessed chip-driver support.

Source/dependency/license pins, commands, scripts, decoded JSON, logs, native screenshots, A/B WAVs and source archive are delivered. SOURCE_ARCHIVE.txt distinguishes frozen implementation from later documentation. Raw/RAM/local emulator fixtures remain under TEST_REPORTS/minimum-release, with capture hashes. Human listening acceptance pending. STOP for distortion, jumps, freezes, wrong duration or lost settings. Owner JEDEC, two verified complete backups, exported settings, known-good image, recovery plan and exact-artifact signoff remain absent. **FLASH STATUS: NOT AUTHORISED.**
