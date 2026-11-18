# AMB+ 0.8 validation

Implementation `73c54f12d4298597bf88e7f68032d337f253379b` includes the generator fix `ad2b73fb` on top of 0.7. Two local builds matched byte for byte. Firmware SHA256 `59ada8c163f4bbc70c5e7d55d3100921b26eb12cb527e152bfe764e7e51ea440`. The recordings and screens came from FM1 Live v0.5.1 running this package at 240 MHz. The explicitly labeled 0.7 comparison uses the prior release.

## Musical checks

- In 0.7 Stone Cloister and Window Seats loaded written 16-slot lines in MANUAL. All 44 factory layers now start in AUTO. In a 64-bar sequencer trace every layer in every factory piece received events. Steady textures sustain a single source by design.
- In a 48-bar host playback test, Stone Cloister's lead changed pitch 76 times, Window Seats' lead 97 times and Departure Hall's lead 74 times. Each produced 12 distinct nonempty four-bar pitch lines. The test reads gated C mixer voices and ignores rests when comparing pitches. Counts describe a specific seed. Confirming a factory preset starts a fresh take. User slots retain their manual steps and seed.
- The old Stone Cloister drone missed every offered cue in the 64-bar trace. A layer can now carry one recent off-grid cue onto its next legal slot after a bar without an onset. Stone's drone played 16 scheduled events in the corresponding run. Manual patterns, steady textures and sparse mode keep their own timing. Low-FOLLOW pedal notes use the selected pitch pool.
- Departure Hall combines 5- and 11-step piano cycles with independently timed sung and triangle-pad parts. A first arrangement offered only three lead notes in the opening eight bars of the fixture. Selecting DRIFT SHIFT gave 19 and left room between them. Harmony moves every 16 bars within a C major pentatonic pool.
- Four two-note holds through the packaged firmware kept audible output into the sixth second. Their held and released RMS levels appear in `evidence/audio-checks.txt`. Under identical input the old FM Glass Haze held about 38 dB below the replacement. Only the old half of the optional A/B recording received level matching. The song recordings and four-pad comparison retain capture gain. Musical character is for the listener to judge.

## Host and emulator

The complete AMB+ host suite passed, including all 64 recipes, both sample sources, natural releases, the new generator checks, 6,404 knob bindings and 291 native 240×240 UI frames. The bitmap audit found zero clipping, bounds or overlap findings. The exact-package PATTERN screen showed AUTO / CHANT for Stone Cloister and AUTO / WALK for Departure Hall.

The pinned JIANT baseline suite finished with 157 golden renders unchanged, zero health failures, zero voice/routing failures and zero crashes. The existing 22 CPU-budget entries and two editor-schema failures keep that suite red. The Emscripten check was skipped because emcc was unavailable. An incremental checkpoint used short emulator runs and the accelerated host musical trace. No new 20-minute soak was run.

| FM1 Live run | Timing | Peak load | Late halves | Voices shed | Audio lost | Max half |
|---|---|---:|---:|---:|---:|---:|
| departure-hall | fast | 29% | 0 | 0 | 0 | 867 µs |
| departure-strict | strict | 72% | 1 | 0 | 0 | 2092 µs |
| strict-boot | strict | 44% | 1 | 0 | 0 | 1293 µs |
| heavy | strict | 96% | 2 | 4 | 0 | 2789 µs |
| stone-cloister | fast | 31% | 0 | 0 | 0 | 914 µs |
| window-seats | fast | 31% | 0 | 0 | 0 | 912 µs |
| glass-haze | fast | 20% | 0 | 0 | 0 | 603 µs |
| amber-brass | fast | 20% | 0 | 0 | 0 | 603 µs |
| cedar-triangle | fast | 20% | 0 | 0 | 0 | 603 µs |
| umber-pulse | fast | 20% | 0 | 0 | 0 | 603 µs |
| save | fast | 20% | 0 | 0 | 0 | 603 µs |
| recall | fast | 20% | 0 | 0 | 0 | 603 µs |
| uc25 | fast | 20% | 0 | 0 | 0 | 603 µs |

The strict heavy-FX trace still exceeds its timing gate. Check the table for late callbacks and voice shedding. No measured timing gate or golden file was changed.

## Resources and storage

| Resource | 0.8 | Delta from 0.7 |
|---|---:|---:|
| App image | 390172 B of 584336 B | +1044 B |
| Static RAM | 76336 B of 98304 B | +16 B |
| RAM code | 22540 B of 25600 B | +20 B |
| Pool spare | 15936 B | 0 B |
| Package | 610113 B | 0 B |

Layout and HAL checks passed. Separate Puya emulator processes saved and recalled the same 1420-byte v4 payload. The UC25 emulator displayed UNSUPPORTED FLASH – SAVE REFUSED. Selecting Glass Haze uses the new PHASE recipe while an old slot with the FM6 engine still selects its previous operator program. Slots with new patch IDs or CHANT mode can require 0.8. Each release launcher has its own state file.

FLASH STATUS: NOT AUTHORISED
