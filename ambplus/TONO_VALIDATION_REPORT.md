# Tono · Garden validation

Firmware source `dcc2656d759c263c1eb786bbd0169cf1f48819f6`
FWSC SHA256 `937933f6b9f5af53c56b7aa005e07a4968230124ee6521c97c61f2649b3e6cd8`
The package and matching ELF reproduced byte for byte on a second build. Toolchain, emulation and source revisions are in [SOURCES_LOCK.json](../../release/tono-0.10/SOURCES_LOCK.json).

## Tono on the LCD

The film captures the packaged application in the native 240×240 display at 100 ms intervals, through the splash and into HOME. The splash draws the landscape in the existing LCD canvas. Native bitmap checks also exercise the reduced-motion and grayscale boot settings, the final hold and the Tono header across the play and edit views. [Startup film](../../release/tono-0.10/screens/startup.gif) · [HOME in Garden](../../release/tono-0.10/screens/garden.png) · [SOUND on Sunroom Keys](../../release/tono-0.10/screens/sunroom-keys.png)

## Six new patches

We selected each patch with the on-screen SOUND selector and played the same C, G and C-major chord on the emulated keyboard. All six delivered nonzero audio with zero full-scale samples. The montage contains nine seconds per patch in the guide’s order. Each excerpt includes a 60 ms fade at the edit and a short gap. The recordings use capture gain. A separate take switches Garden’s Pad, Bells and Bass to Sunroom Keys, Dew Bell and Linden Bass and plays all four tracks together. Separate Puya save and recall processes recovered that combination. Existing Garden patch IDs and factory scene settings were also checked.

[Six-patch audition](../../release/tono-0.10/audio/tono-patches.wav) · [New voices playing Garden](../../release/tono-0.10/audio/sunroom-garden.wav)

## Musical behavior

Garden is the twelfth arrangement and uses the shared track voices, pitch pool and effect sends. Factory settings are D Dorian, 72 BPM. It pairs the independently selectable RHYTHM generator with Rhodes swells, overlapping 13/17/23/29-beat bell loops, a pedal bass and soft percussion. On the native host, 64 bars produced Pad 58, Bells 111, Bass 7, Pulse 398. Four-bar generator changes, manual pattern preservation, transport cleanup and selection of these patches under the older conductors passed.

The first 45 seconds of the original host render had 20 pad starts, 23 bells, 2 bass and 97 percussion hits. The port's factory event trace had 14, 23, 2 and 94. Random streams, note allocation, track envelopes and per-track panning differ. The port uses fixed-point operators, a half-rate stereo plate and a companded quarter-rate tape line. Pitch, timing and dynamics can diverge between performances.

| Band | AMB-1 average / peak dBFS | This build average / peak dBFS |
|---|---:|---:|
| Full band | -24.5 / -10.2 | -23.5 / -11.8 |
| Below 120 Hz | -33.8 / -20.7 | -34.1 / -20.1 |
| 120–600 Hz | -25.9 / -11.2 | -24.8 / -12.3 |
| 600–2500 Hz | -32.3 / -15.8 | -31.1 / -15.0 |
| Above 2500 Hz | -47.3 / -22.9 | -43.8 / -23.1 |

Stereo side-to-mid energy over the same excerpts was -0.5 dB for AMB-1 and 0.4 dB for this build.

Measurements compare the two packaged 45-second recordings at capture gain. The two Morning Air generator and patch demonstrations each last 15 seconds. Independent bus impulse tests found short repeats at 7101 and 2701 frames, a two-head repeat at 110368 frames and a stereo plate tail in the second second.

## Exact-package native emulator

fm1-live v0.5.1 ran the packaged FWSC at 240 MHz. “Late” counts audio halves missing their deadline. “Shed” counts voices faded by the overload guard. “Lost” is a separate firmware audio counter. Strict timing was enabled on garden-strict, strict-boot and heavy, plus the 30.5-second sunroom-garden arrangement. The heavy workload ran for 60 simulated seconds and the strict Garden run for 25. No new 20-minute soak was run.

| Run | Peak load % | Late | Shed | Lost |
|---|---:|---:|---:|---:|
| dew-bell | 23 | 0 | 0 | 0 |
| garden | 33 | 0 | 0 | 0 |
| garden-strict | 68 | 1 | 0 | 0 |
| heavy | 98 | 2 | 2 | 0 |
| legacy-rhodes | 34 | 0 | 0 | 0 |
| linden-bass | 23 | 0 | 0 | 0 |
| linen-pad | 23 | 0 | 0 | 0 |
| morning-garden | 31 | 0 | 0 | 0 |
| morning-space | 29 | 0 | 0 | 0 |
| recall | 25 | 0 | 0 | 0 |
| save | 23 | 0 | 0 | 0 |
| skylight | 20 | 0 | 0 | 0 |
| strict-boot | 42 | 1 | 0 | 0 |
| sunroom-garden | 68 | 1 | 0 | 0 |
| sunroom-keys | 23 | 0 | 0 | 0 |
| sunroom-recall | 25 | 0 | 0 | 0 |
| sunroom-save | 23 | 0 | 0 | 0 |
| uc25 | 25 | 0 | 0 | 0 |
| upgrade-garden09 | 25 | 0 | 0 | 0 |
| upgrade-slot | 20 | 0 | 0 | 0 |
| willow-tine | 23 | 0 | 0 | 0 |

Garden save/recall and 0.8 migration both matched all 1420 bytes. Garden payload c8d1489a7b1b1cffd3c1e863f38d16129662acc177448399c4f9883781132e38. Standalone PLATE (5) and ECHO+TAPE (4) confirmed on Morning Air.
Both the imported 0.9 Garden record and the Tono record with Sunroom Keys, Dew Bell and Linden Bass also matched byte for byte after recall. The UC25HQ80 emulator ran the piece and displayed the unsupported-save message. Puya save tests used isolated emulator flash files.

Each evidence folder includes the timestamped front-panel input, emulator log and decoded end state. To replay a run from this release, set FM1_BENCH_KEYS to @ followed by the absolute path to its inputs.txt, use the duration and clock from its emulator.log, then start fm1-live with `--fwsc tono.fwsc --midi none --mhz 240 --bench <milliseconds>`. Set FM1_STRICT=1 for strict cases. Save and migration checks also require the appropriate isolated emulator flash fixture.

## Regression and budgets

The Tono host suite passed all 7,388 context-specific knob checks. The native bitmap audit covered 298 frames and reported zero clipping, bounds or overlap findings. Full generic JIANT host runs (baseline, Garden branch and Tono source at dcc2656d, AMBPLUS=0) matched 157 golden renders. All three retained 22 CPU budget findings and two editor schema failures. Full suite status remains failing. The dedicated Garden test also passed AddressSanitizer on Linux. The Darwin sanitizer runtime stalled in shadow-memory initialization before entering the test program. Emscripten was unavailable. Exact binary control and audio evidence is in the per-run folders under evidence.

| Resource | Bytes | Change since 0.8 |
|---|---:|---:|
| Application / 584336 | 401696 | +11524 |
| Static RAM / 98304 | 78256 | +1920 |
| Pool spare | 8832 | -7104 |
| RAM code / 25600 | 23504 | +964 |
| FWSC | 610113 | 0 |

[Listening instructions and rollback](../../release/tono-0.10/MILESTONE_TEST_CARD.md)

**FLASH STATUS: NOT AUTHORISED**
