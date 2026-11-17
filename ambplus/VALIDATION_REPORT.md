# AMB+ 0.1 development validation — 2026-11-17

**DEVELOPMENT CANDIDATE. VALIDATION INCOMPLETE; NOT HARDWARE-VALIDATED. NO FLASH.**

The complete native instrument implementation is available and playable in the Joakim Eriksson FM1 emulator. It is not accepted as a stable hardware release: strict heavy-FX stress fails timing, the upstream full suite remains red, UC25 saving is unsupported, and human interactive/listening acceptance is incomplete. No physical device was accessed, installed, erased, recovered or power cycled.

## Artifact and reproduction

- Package: ambplus.fwsc, 610113 bytes, SHA256 `20ce66b3092ada751e4be0c2c8422a2ce0bb0caaeaf07da1d892a2d88bf82d85`.
- Implementation commit: `6e51711c37e50f18e473a70e635917390a4831a5`, branch ambplus/complete-controls-sample. SOURCE_SHA.txt also identifies the subsequent documentation-only archive commit.
- ELF SHA256: `a41e9956325c06876db484051ad5b8c862336a723a390a7b9aa35f2a4c4d6d37`.
- Two AMBPLUS=1 builds compared byte-for-byte equal. Source archive, build scripts and dependency lock included. Full pins/licenses in SOURCES_LOCK.json, LICENSE, LICENSING.md and LICENSES/.
- Original JIANT package SHA256: `5d486a8bc182f01e9ae69f52a117a769f40c122491bba47d3c8baa662e6cc671`; baseline original target bytes were unchanged after host portability fixes.

Launch with ./launch-emulator.sh; read CONTROLS.md. REPRODUCE_TESTS.md supplies exact test commands. Only the compiled .fwsc is an interactive instrument. The handoff HTML remains a visual specification. Host C fixtures exercise real firmware rendering/DSP against stubs; they do not emulate the chip. fm1-live executes pi32v2 code and its emulated DMA/interrupts/LCD/flash. No separate JIANT Emscripten runtime was built.

## Native tests and upstream regression

AMB+ targeted tests PASS: ADC pickup/retarget, bin hysteresis/jitter/final settle/wrap; 22 distinct repeatable policies; pending queue/cancel; Euclid/chord helpers; 1000-event byte-preservation of local settings; every role in every slot; real step recording and locked patterns; contextual REC LEDs with the real HAL key map; macro baselines; explicit eight-slot save/overwrite/failure recovery/version migration; staged live recall; solo persistence; global four-voice bound under 40-note input. All 48 synth recipes plus two sample sources independently render nonzero distinct PCM; six scenes render 12 seconds each. Native UI audit covers 136 frames with zero clipping/bounds/overlap findings at exactly 240×240. The final PNG contact sheet was visually inspected.

The full upstream suite was run against the pinned baseline. **Overall FAIL**: 157 golden renders have zero changes and zero health/voice/routing/crash failures, but **22 CPU entries exceed stored limits** (examples: idle 516 vs 286 instructions/sample; ANALOG soft pad 1065 vs 820), and **two editor tests fail** (56 mock-schema differences and enum ordering). Those are inherited baseline results, not a passing regression gate. Golden files and CPU budgets were not rewritten. Fixtures referencing retired engines were corrected to active engines without relaxing thresholds. Darwin fixtures required section/prototype portability adjustments; original target bytes stayed identical. Native UI baseline also completed 364 screens × 11 palettes / 2475298 alignment checks without layout failures.

Mac ASan's standalone probe stalled and was skipped, explicitly. Linux arm64 Docker GCC ASan/UBSan full AMB+ native fixture PASS, with the existing fixed-point signed-overflow/shift exclusions and leak checking disabled. Emscripten and manual Chromium checks were skipped because those optional dependencies were unavailable; no pass is claimed. Comprehensive human operation of fm1-live was not completed: launch opened its audio stream, while desktop automation could not attach to the bare executable. Repeatable GPIO/encoder/MIDI scripts and screenshots supply automated exact-package evidence.

## Exact final-package emulator results

Official fm1-live v0.5.1 macOS arm64, binary c7e47c4fc, target 240 MHz, MIDI inputs disabled, no USB bridge. Final RAM decoding uses the matching ELF hash above.

| Test | Observed result | Acceptance |
|---|---|---|
| 1200000 ms functional soak, fast emulator | 404.63 s wall; 2.97× realtime; 52935680 frames; one boot; audio_lost=0; late=0; shed=0; peak load 29%, ending smoothed load 23.05%; max half 846 µs | PASS at this test level only |
| Soak audio | 1199.866 s captured stereo at 44118 Hz, peak 7992/32767, RMS 570.11, zero full-scale samples | Unclipped capture; no aesthetic acceptance implied |
| 60000 ms FM1_STRICT=1, Spring FX + EQ + four-channel MIDI | 118.12 s wall; max half 2802 µs; peak load 96%, ending smoothed load 67.58%; **2 late halves, 4 shed events**; audio_lost=0 | **FAIL** |
| Strict audio | 59.529 s capture; peak 6829, RMS 954.49, zero full-scale samples | Unclipped; timing gate remains failed |
| Separate-process Puya save/recall | Active IDs [1,2,3], variation 88 and all four serialized layer hashes equal after restart; 13 emulator flash operations on save | PASS |
| UC25HQ80 same save input | Native LCD: UNSUPPORTED FLASH – SAVE REFUSED; zero emulator flash operations | Correct refusal; persistence **BLOCKED** |

The 20-minute script changes generation families, variation, role/patch/editor inputs, notes, effects, presets, stopped save/recall and panic/resume. Its exact input trace is preserved. It is fast instruction-level functional coverage; it is not equivalent to 20 minutes of strict or physical timing. Strict idle captures of unchanged JIANT and an earlier AMB+ candidate each show one boot late half; this context does not explain away the final candidate's additional lateness or shedding. Earlier failed candidate logs are retained under TEST_REPORTS and are not substituted for final results.

Audio in audio/strict.wav and audio/soak-excerpt.wav comes from this exact firmware package, unnormalized. native-fixture-demo.wav is separately labeled host C output. Hardware panel PNGs are native emulator captures; *native-240.png files crop the 2× LCD back to its exact 240×240 pixels using nearest-neighbor sampling. Fixture PNGs are from the native renderer using the test display sink.

## Resources and storage

| Measured build quantity | Original JIANT | AMB+ | Delta |
|---|---:|---:|---:|
| Application flash bytes / 584336 | 568436 | 368496 | −199940 |
| Static data+BSS / 98304 | 80324 | 74400 | −5924 |
| Runtime pool | 310596 | 303552 | −7044 |
| Reported spare pool bytes | 8864 | 15936 | +7072 |
| RAM code / 25600 | 19464 | 21036 | +1572 |
| Fixed padded package size | 610113 | 610113 | 0 |

No direct baseline-to-candidate stress CPU percentage delta is asserted: the scripts differ. Both build resource checks pass, while the unchanged baseline CPU golden-budget comparison fails as detailed above. Two 8192-sample PCM16 sources consume 32768 flash bytes, no PCM RAM or writable sample sectors. Four layers share a hard four-voice budget; heavy FX remain the timing risk.

A 1400-byte version-2 whole-piece record fits existing 3840-byte A/B payloads. Eight slots reuse 0x97000–0x9efff and 0xa0000–0xa7fff; no new reserved sectors, loader, linker map or HAL flash-driver changes. tools/build.py only adds the AMBPLUS compile flag; packaging logic remains unchanged. Writes are explicit stopped SAVE actions. Old JIANT data is occupied/incompatible and requires guarded explicit overwrite; returning to JIANT will not read AMB+ pieces. v1 migration is only for same-layout experimental AMB+ records; it is not a converter for arbitrary firmware songs. Actual storage-write bounds on an unknown owner's hardware remain unverified.

## Decisions and remaining work

The pinned HAL and hardware emulator define seven quadrature encoders and only MASTER/battery ADCs. Therefore GENERATE uses actual encoder inputs plus final settle; the contrary absolute Knobs 1–4 specification is technically blocked. The generic absolute decoder is unit-tested but not connected to invented ADC wiring. MASTER pickup is wired to the real absolute input.

Retired STRING is replaced with functioning PHASE/FM/analog recipes. Spatial processing uses JIANT's real shared effects rather than duplicated per-layer reverbs. AMB-1 Euclidean/harmonic helper imports keep MPL-2.0 notices; all original JIANT notices remain. No Shrub code/assets were used. External microphone capture and fullscreen VISUAL alone are intentionally deferred.

Required next software work: profile and resolve strict FX/audio timing without weakening budgets, resolve or explicitly reconcile baseline CPU accounting and editor schema drift, implement/review/test the actual UC25 flash protocol before considering that variant, then repeat strict long-duration tests and comprehensive human emulator/listening acceptance. Do not silently whitelist a chip ID. Manual curation, all possible gesture transitions, physical pot feel, LED visibility, hardware audio noise and real flash behavior are not certified by these tests.

Release-gate decision is **NO FLASH**, consistent with the supplied SAFE_FLASH_GATES.md and local fm1-release-gate skill. Owner JEDEC, two independently verified complete backups, exported settings, known-good image, model-specific recovery and exact-artifact signoff are missing. Timing failure independently meets the STOP condition. The filled MILESTONE_TEST_CARD.md gives future low-volume acceptance requirements and rollback limitations. No owner device testing is requested while these gates are open.
