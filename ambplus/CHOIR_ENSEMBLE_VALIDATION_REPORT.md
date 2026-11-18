# AMB+ 0.7 validation

Implementation `ff4e3e5e979bfab7d974d06d908757a48d475ffb` on `ambplus/choir-and-ensemble`, from `9cff660704b22e6ee1ad861f7303c7d578216630`. Exact package SHA256 `bd815b121fd335bce468e63d8cc684fa5d10dfdf7d887c92ee8152136ecedac8`. Independent local rebuild matched byte for byte. The native C firmware ran in pinned FM1 Live v0.5.1. Human musical acceptance is open.

## Coverage

- Full AMB+ host suite passed. All ten factory pieces rendered and validated. Three-second isolated renders checked signal energy and distinct hashes within all 61 recipes and both bundled sample sources. The catalog tests checked recipe bounds and the native JIANT preset index.
- 5,988 context-specific knob checks exercised turn, value readback, label and help across roles, recipes, manual and generated pattern modes, deep banks and global controls. These are control-binding checks. Perceived importance at a particular mix level is a listening judgment. The native renderer audited 290 frames at 240×240 with zero clipping, bounds or overlap findings. Exact-package screens confirm the live meter and vowel guide.
- Natural chord playback, timed sound replacement, silent manual entry, texture holds, per-layer EQ, old save migration and storage failure recovery passed the existing targeted tests. The new tests also cover help return, silent navigation, individual ensemble gains and retained MASTER targeting.
- The complete pinned upstream baseline suite still fails overall. Of 157 golden renders, zero changed. There were zero health failures, zero failed voice/routing checks and zero crashes. 22 existing CPU-budget entries and two web editor checks failed. No budgets or golden snapshots were changed. Emscripten was unavailable.
- Code cleanup separates the variable-length patch banks from playback, shares FX routing between parameter editing and display, and publishes a deep edit to its own layer. The ensemble reads the mixer's existing peak accumulator. These changes add zero bytes of static RAM in the linked image.

## FM1 Live at 240 MHz

| Exact package run | Mode | Peak load | Late halves | Shed | Lost audio | Max half |
|---|---|---:|---:|---:|---:|---:|
| window-seats | fast | 28% | 0 | 0 | 0 | 833 µs |
| stone-cloister | strict | 72% | 1 | 0 | 0 | 2090 µs |
| tidal-loom | fast | 28% | 0 | 0 | 0 | 815 µs |
| kite-strings | fast | 31% | 0 | 0 | 0 | 908 µs |
| vowels | fast | 21% | 0 | 0 | 0 | 615 µs |
| strict-boot | strict | 43% | 1 | 0 | 0 | 1257 µs |
| heavy | strict | 96% | 2 | 7 | 0 | 2805 µs |

All four 24-second song tests selected the preset through PRESETS/REC, started PLAY and opened ENSEMBLE. Decoded RAM confirms the intended keys, roles and patches. Each recording has audible signal and zero full-scale samples. The strict chant run counted one late half. A strict cold-boot control already showed one before user input. The 60-second heavy-FX trace still fails the timing gate, with two late halves and seven sheds. The previous checkpoint measured 98%, five late and eight shed on that trace. A single comparison does not establish a general speedup.

The first automated capture accidentally suppressed its scripted controls and remained on Morning Air. It was discarded. The reported song, help, persistence and heavy-FX results use corrected inputs and final-state verification. The intentionally idle strict-boot control is separate. No 20-minute soak was run for this checkpoint.

## Storage and footprint

Save v4 remains 1420 bytes within the existing eight A/B slots. On the Puya model, separate save and recall processes matched all 1420 bytes of Stone Cloister, SHA256 `6fea6b18591ac0958b666ce25e8795b6fce697e962de9b0bf750ce7da1432e30`. The UC25 emulator displayed SAVE REFUSED. Existing recipe IDs and saved PHASE textures are accepted by this build. Rolling back to 0.6 can reject a save containing one of the new recipe IDs. Keep a separate state per emulator build.

| Linked resource | 0.7 | Change from 0.6 |
|---|---:|---:|
| App image | 389128 B of 584336 B | +9236 B |
| Static RAM | 76320 B of 98304 B | 0 B |
| RAM code | 22520 B of 25600 B | −32 B |
| Package | 610113 B | 0 B |

The build's layout and HAL-register checks passed. Source licenses, toolchain pins, exact-package input scripts, decoded reports and host logs accompany the release. Physical testing and installation remain outside this checkpoint.

FLASH STATUS: NOT AUTHORISED
