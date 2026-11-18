# AMB+ · Garden validation

Firmware source `fdcf12e3401590d6702283f8b90bd63567d7e393`  
FWSC SHA256 `ecbb30daf128f2a5808e78a58249f3e01f8a0ed8373f7e2ed7e35eceb5ae2d3f`  
The package and matching ELF reproduced byte for byte on a second build. Toolchain, emulation and source revisions are in [SOURCES_LOCK.json](../../release/ambplus-0.9-garden/SOURCES_LOCK.json).

## Musical behavior

Garden is the twelfth arrangement and uses the shared track voices, pitch pool and effect sends. Factory settings are D Dorian, 72 BPM. It pairs the independently selectable RHYTHM generator with Rhodes swells, overlapping 13/17/23/29-beat bell loops, a pedal bass and soft percussion. On the native host, 64 bars produced Pad 58, Bells 111, Bass 7, Pulse 398. Four-bar generator changes, manual pattern preservation, transport cleanup and selection of these patches under the older conductors passed.

The first 45 seconds of the original host render had 20 pad starts, 23 bells, 2 bass and 97 percussion hits. The port's factory event trace had 14, 23, 2 and 94. Random streams, note allocation, track envelopes and per-track panning differ. The port uses fixed-point operators, a half-rate stereo plate and a companded quarter-rate tape line. Pitch, timing and dynamics can diverge between performances.

| Band | AMB-1 average / peak dBFS | This build average / peak dBFS |
|---|---:|---:|
| Full band | -24.5 / -10.2 | -24.2 / -11.9 |
| Below 120 Hz | -33.8 / -20.7 | -33.7 / -18.8 |
| 120–600 Hz | -25.9 / -11.2 | -26.0 / -12.7 |
| 600–2500 Hz | -32.3 / -15.8 | -30.9 / -15.1 |
| Above 2500 Hz | -47.3 / -22.9 | -44.0 / -21.9 |

Stereo side-to-mid energy over the same excerpts was -0.5 dB for AMB-1 and -0.1 dB for this build.

Measurements compare the two packaged 45-second recordings at capture gain. The patch and generator demonstrations each last 15 seconds. Independent bus impulse tests found short repeats at 7101 and 2701 frames, a two-head repeat at 110368 frames and a stereo plate tail in the second second.

## Exact-package native emulator

fm1-live v0.5.1 ran the packaged FWSC at 240 MHz. “Late” counts audio halves missing their deadline. “Shed” counts voices faded by the overload guard. “Lost” is a separate firmware audio counter. Strict timing was enabled on garden-strict, strict-boot and heavy. The heavy workload ran for 60 simulated seconds and the strict Garden run for 25. No new 20-minute soak was run.

| Run | Peak load % | Late | Shed | Lost |
|---|---:|---:|---:|---:|
| garden | 33 | 0 | 0 | 0 |
| garden-strict | 67 | 1 | 0 | 0 |
| heavy | 96 | 2 | 2 | 0 |
| legacy-rhodes | 34 | 0 | 0 | 0 |
| morning-garden | 31 | 0 | 0 | 0 |
| morning-space | 29 | 0 | 0 | 0 |
| recall | 24 | 0 | 0 | 0 |
| save | 24 | 0 | 0 | 0 |
| strict-boot | 42 | 1 | 0 | 0 |
| uc25 | 25 | 0 | 0 | 0 |
| upgrade-slot | 20 | 0 | 0 | 0 |

Garden save/recall and 0.8 migration both matched all 1420 bytes. Garden payload 8c7723ee334fc7c9e7e023f0980bf0648b5666447dc26a9cb42f7235afb77a38. Standalone PLATE (5) and ECHO+TAPE (4) confirmed on Morning Air. The UC25HQ80 emulator ran the piece and displayed the unsupported-save message. Puya save tests used isolated emulator flash files.

## Regression and budgets

The AMB+ host suite passed. 6796 context-specific knob bindings passed. Native bitmap audit covered 293 frames, 0 clipping/bounds/overlap findings. Two full generic JIANT host runs (upstream baseline and feature branch, AMBPLUS=0) matched 157 golden renders. Both retained 22 CPU budget findings and two editor schema failures. Full suite status remains failing. Emscripten was unavailable. Exact binary control and audio evidence is in the per-run folders under evidence.

| Resource | Bytes | Change since 0.8 |
|---|---:|---:|
| Application / 584336 | 402508 | +12336 |
| Static RAM / 98304 | 78256 | +1920 |
| Pool spare | 8832 | -7104 |
| RAM code / 25600 | 23500 | +960 |
| FWSC | 610113 | 0 |

[Listening instructions and rollback](../../release/ambplus-0.9-garden/MILESTONE_TEST_CARD.md)

**FLASH STATUS: NOT AUTHORISED**
