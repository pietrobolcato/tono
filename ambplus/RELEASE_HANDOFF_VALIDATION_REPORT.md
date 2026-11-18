# AMB+ 0.5 validation — 2026-11-18

Development candidate. Software implementation/evidence delivered; human musical acceptance and hardware release gates remain incomplete. **FLASH STATUS: NOT AUTHORISED.** No physical FM1 was accessed, installed, reset, erased, recovered or power-cycled.

## Identity and scope

Implementation source **6c6b487bd60937af1c39867387dfdb1cf41b5c7f**, branch `ambplus/release-handoff-layer-eq`, based on documentation commit b53fb5ba and prior implementation ea8d65175e3f96fbe45a7a301f1ff00557263d00. Firmware SHA256 **16a03688f473bde6387b288551fb8169bc8ec8f7f80f5657a1422ff3cdc6a300**. Two builds matched byte-for-byte. SOURCE_ARCHIVE.txt identifies the later documentation checkout archived with identical implementation code. Dependency/toolchain/license identities are in SOURCES_LOCK.json; no upstream auto-update or Shrub material.

User-authorized changes: finish the outgoing patch/role/source release, accelerate only longer releases to a configurable cap (250–6000 ms, default 3000), then start the new engine with its own envelope. Preserve shared FX; no added incoming musical fader. Coalesce rapid choices without restarting the old tail. Protect outgoing tails from normal stealing while retaining realtime overload protection and the four-voice cap. The destination waits for completion; this is sequential handoff, not two independent engines sounding on the same track. Ordinary sound parameters remain direct; queued-patch edits apply on arrival.

PATTERN now separates divided/swing-dispatched PLAY position from SELECTed EDIT cursor and refreshes at 20 Hz. The marker is a step clock including rests, not an audio meter; attacks/releases/echoes may sound between steps. REC captures generated notes and enters selected slots silently instead of immediate audition/live-playhead overwrite. New seventh EDIT page supplies independent layer EQ before dry mix/shared sends, including drum image/send paths. Unity bypass is exact; gains smooth and high boosts have bounded headroom. Earlier natural harmony/style progression and consolidated footer remain.

## Automated checks

- PASS: complete AMB+ controls/policies/music/native/feedback/transition/normal-playback/handoff/renderer suite. All roles/patches, six scenes, generated/manual preservation, recording, note pool, restore, storage A/B recovery and forty-note voice bound checked. Ten automatic chord changes in 35 seconds: no extra source fade or forced Pad portamento. No golden/budget rewrites.
- PASS: handoff keeps outgoing engine/parameters and releasing voices, coalesces rapid requests, covers all seven outgoing roles and FM6 internal envelopes. Measured Pad release 2.700 s under the 3 s cap. Short-cap completion, voice ceiling, silent selected-slot input and SPEED-divided playhead assertions pass.
- PASS: EQ flat identity and layer independence, persisted values, broad-band selectivity and extreme-signal headroom. +6 dB settings yield selective power ratios ~3.48 low / 3.39 mid / 3.19 high at 80/1000/9000 Hz; broad crossover bands are not a precision parametric equalizer.
- PASS: native bitmap audit **187 frames at exactly 240×240**, zero clipping/bounds/overlap. Exact-package LCD screenshots confirm pending release, SOUND and new LAYER EQ. Host C bitmap captures are labelled separately; the handoff HTML remains a visual specification.
- PASS: six Linux ARM64 ASan/UBSan fixtures. Upstream signed-integer-overflow/shift exclusions retained; leaks disabled. Mac sanitizer probe remains explicitly skipped because it hangs in baseline. Host tests are fixtures, including mocked installer cases, never physical device operations.
- FAIL overall: mandatory full upstream host suite in baseline worktree e4e346a3f74a236ab286349a6a9bff68f3ed8a31 (pinned JIANT with recorded host portability support): **157 unchanged goldens, 0 health/voice/routing failures/crashes, 22 inherited CPU-budget entries, two inherited editor mock/schema failures**. Emscripten absent; no separate interactive JIANT web synth was built. Native .fwsc in fm1-emu is the interactive emulator.

## Exact-package execution

Official fm1-live v0.5.1 macOS ARM64 binary, reported c7e47c4fc; source pin 2ed986561c6f26596d37fd306fac0e7558a41356. MIDI inputs/USB bridge disabled. Puya emulator unless stated. Measured load is peak load_diag, not the analyzer's ending smoothed load mislabeled average.

| Run | Peak | Late | Shed | audio_lost | Max half |
|---|---:|---:|---:|---:|---:|
| Patch changes, 45 s, 240 MHz fast | 29% | 0 | 0 | 0 | 842 µs |
| All features/four layer EQs/cap/entry, 190 s, 240 MHz fast | 32% | 0 | 0 | 0 | 955 µs |
| 20-minute functional soak, four layer EQs, 240 MHz fast | 33% | 0 | 0 | 0 | 985 µs |
| Canonical strict stress, 60 s, 240 MHz | 98% | 4 | 11 | 0 | 2845 µs |
| Stronger strict stress/four active layer EQs, 70 s, 240 MHz | 104% | 8 | 19 | 0 | 3026 µs |
| Same stronger strict stress, 70 s, 320 MHz | 78% | 0 | 0 | 0 | 2268 µs |

The 20-minute soak completed in 454.95 s wall (~2.64×), one boot, with explicit patch/type/key/scale/style/EQ/cap/silent-entry changes followed by normal AUTO. It is a functional fast-emulation test, not strict timing certification. No long-soak WAV was captured; shorter actual-firmware WAVs provide audio evidence. Patch capture peak 7260, all-features peak 8346, no full-scale samples. A/B patch WAVs are unnormalized; the previous package used the identical patch script. Human listening acceptance remains pending.

Strict **240 MHz gate FAILS**. Prior v0.4 canonical stress measured 100%, 3 late / 9 shed / max 2915 µs; current canonical 98%, 4 / 11 / 2845 µs is not a release pass. Stronger four-EQ stress is a different workload and must not be called a like-for-like regression delta. The 320 MHz setting passes but does not establish the owner's actual clock. Default/source clock was not raised. audio_lost=0 and unclipped samples do not excuse late halves or shedding.

## Resources and storage

| Resource | Current | Delta from v0.4 |
|---|---:|---:|
| Application | 378324 B / 584336 B | +2980 B |
| Static .data+.bss | 76116 B / 98304 B | +332 B |
| RAM code | 22280 B / 25600 B | +836 B |
| Pool | 303552 B; spare 15936 B | 0 B |
| Package | 610113 B | 0 B |

Layout/register checks pass. Loader 6957 B; no bootloader, driver, memory layout, reserved-sector or save-write bounds changed. No new synth or audio buffer was allocated for tails; existing voices release before replacement. Nonflat EQ costs DSP time; flat layers bypass it exactly.

Eight A/B payloads are now append-only **v4, 1420 B**, within existing 3840 B capacity and the same 0x97000..0x9efff / 0xa0000..0xa7fff objects. v1–v3 1400-byte records migrate with flat layer EQ and a 3000 ms cap; old software cannot read v4, so retain old exports/state for rollback. Native version/size/range/CRC/torn-write tests pass. Separate-process Puya save/recall matches the full piece, including EQ [5,0,-4,0], 1750 ms cap, shared pool and retained manual notes. Exact-package v3 migration preserves every prior piece field. The corrected migration fixture copies only legacy save objects into a current-code emulator image; an initial full old-image attempt booted old firmware and is excluded, with setup notes preserved.

Puya 0x856014 persistence passes. UC25HQ80 0xB36014 remains unsupported by the pinned whitelist: SAVE REFUSED, zero flash operations. No guessed driver fix. Emulator launch uses a private artifact-specific state file because full emulator snapshots include firmware code; do not substitute older full images.

## Remaining acceptance and delivery

Follow MILESTONE_TEST_CARD.md at low emulator volume. STOP on clicks/distortion, unexpected pumping/pitch sweeps, missing release/tails, immediate step-entry auditions, incorrect play position, freezes or lost settings. Earlier packages/state are preserved for emulator rollback. Current v4 records are not readable by prior v3 firmware.

Evidence includes scripts, build/targeted/sanitizer/full-host logs, strict timing/RAM-derived JSON, WAVs, native screenshots, pins/licenses and a source archive. Large raw captures and full emulator RAM/flash fixture files remain under TEST_REPORTS/release-handoff; release evidence includes their SHA256 identities and decoded reports. The physical owner's actual JEDEC, two verified full-chip backups, exports, known-good image, recovery plan and exact-artifact signoff are still absent. **FLASH STATUS: NOT AUTHORISED.**
