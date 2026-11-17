# AMB+ implementation ledger

DEVELOPMENT CANDIDATE — VALIDATION INCOMPLETE; NOT HARDWARE-VALIDATED.
FLASH STATUS: NOT AUTHORISED. No physical device actions were run.

The native implementation and its tests are committed at `6e51711c37e50f18e473a70e635917390a4831a5`, branch `ambplus/complete-controls-sample`. Source and dependency pins are in SOURCES_LOCK.json. The exact package SHA256 is `20ce66b3092ada751e4be0c2c8422a2ce0bb0caaeaf07da1d892a2d88bf82d85`.

## Implemented and exercised

- [x] Baseline build and exact-package hardware emulator audio/screen capture; baseline host suite executed and failures retained.
- [x] Four assignable layers; five HOME targets, MASTER pickup, macros, mute/solo and panic.
- [x] Native 240×240 bitmap UI: six EDIT and five GLO pages; contextual inputs/LEDs; 136-frame layout audit with no findings.
- [x] 48 distinct recipe renders and six factory compositions using JIANT DSP; two original bundled PCM sources.
- [x] Harmony/voice leading/evolution and 22 distinct deterministic event policies; manual state serialization remains unchanged by scheduling.
- [x] Manual melodic/drum step recording, locks and independent drum kit selection.
- [x] Shared native FX, layer sends and master EQ/protection. Strict timing acceptance remains failed.
- [x] Version-2 whole-piece save, eight existing A/B objects, CRC/failure recovery tests, guarded overwrite and version-1 experimental migration.
- [x] Exact-package Puya save/recall across emulator processes.
- [x] Linux ASan/UBSan test of the full native fixture.
- [ ] Strict timing acceptance: latest stress records peak 96%, two late halves, four shedding events; one late half is also observed at unchanged JIANT idle startup. No claim that the other late half is benign.
- [ ] UC25 persistence: pinned JIANT rejects `0xB36014`; emulator save is refused with zero flash operations. No flash-driver whitelist was guessed.
- [ ] Full upstream suite green: 157 golden renders unchanged, but 22 CPU-budget entries and two web mock schema tests fail. No golden or budget updates were made.
- [ ] Human listening, interactive control acceptance and physical hardware certification.

## Decisions and limits

The verified HAL and emulator expose KNOBS 1–4 as quadrature encoders, with only MASTER/battery ADC inputs. GENERATE uses the real encoders and 90 ms final settle; the generic absolute-pot pickup/bin/hysteresis decoder is separately tested but cannot be connected to nonexistent ADC inputs. This resolves the handoff's wiring contradiction using source evidence.

Retired STRING is replaced by live PHASE/FM/analog recipes. The four-voice global budget is enforced under dense input; four simultaneous layers share it. Heavy shared FX can still exceed timing headroom. DSP output and distinct PCM hashes establish actual signal generation, not aesthetic quality or click-free operation across every possible gesture.

MPL-2.0 AMB-1 Euclid, chord-tone and voice-leading code is isolated in amb_harmony.h with original notices and complete license. Existing JIANT processing supplies spatial/tape effects. No Shrub materials were used. External capture and fullscreen VISUAL remain intentionally deferred.

The root release folder contains the final report, complete feature matrix, human card, emulator launch script, checksums, source archive, WAV examples and native screenshots. Earlier failed candidates remain under TEST_REPORTS; only TEST_REPORTS/final matches the delivered package. See the final report for exact evidence and outstanding gates.
