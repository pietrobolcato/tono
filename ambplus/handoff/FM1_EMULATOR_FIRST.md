# AMB+ — Single-emulator firmware development contract (v1.6)

**Status:** Plan and Codex handoff. This package includes **NO compiled JIANT firmware** and changes **NO physical device**.

## The decision

Use **only Joakim Eriksson's `fm1-emu` for interactive firmware trials**, with two front ends of the SAME emulator when needed:
1. Browser `https://joakimeriksson.github.io/fm1-emu/` for visual/UI/audio trial of a locally produced **`.fwsc`** (choose **Open .fwsc…**, point at `build/felucca.fwsc`).
2. Optional native `fm1-live` for repeatable CPU/flash/USB-oriented testing, and the related headless `fm1-emu` runner for scripted tests. These execute the **firmware package**, not a separate Emscripten adaptation.

Keep JIANT's **`tests/run_tests.sh`** (host/unit/golden tests) for fast bug diagnosis. This is not a second playable emulator. Do **not** make JIANT's `web/emu/build.sh` a blocking dependency or keep a second browser instrument in sync by default.

## Repeatable loop (Codex; no device)

1. Pin JIANT source commit, JieLi compiler, AC79 SDK, sources of any licensed AMB code, and generated data.
2. Build: `./build.sh` → normally `build/felucca.fwsc` (check actual output names from pinned JIANT build).
3. Run: `tests/run_tests.sh`. Investigate all regressions; do not auto-refresh golden snapshots.
4. Record SHA256 of `.fwsc` and source commit; open **that exact package** in browser fm1-emu via **Open .fwsc…**.
5. Check HOME MASTER + four layers, SELECT/SEL, MODE preview/REC commit, SOUND immediate patch changes, FX, GLO GENERATE, sample flow, save/recall, visual bounds and audible clicks.
6. On Mac, install supported `fm1-live` from its **official releases** if available for architecture. Example after inspecting release instructions: `./fm1-live --fwsc /absolute/path/to/build/felucca.fwsc`.
7. Stronger native hardware simulation: `FM1_STRICT=1 ./fm1-live --fwsc /absolute/path/to/build/felucca.fwsc`. This runs much slower (interpreted) but models measured instruction cost, LCD transfer and some memory faults. Not a guarantee of device behaviour.
8. Save logs, native screen captures and audio artifacts. Sign an emulator release gate; **do not flash**.

**Browser clock:** the real FM1 is 240 MHz; the FM1 emulator site may default to 100 MHz for browser performance. Choose 240 MHz when assessing timing, but avoid interpreting browser audio glitches or lack thereof as proof of physical CPU headroom. Use strict native tests for stronger evidence.

**Flash chip variants:** test the emulator's default flash and newer `?chip=uc25hq80` (or corresponding native `--flash-chip uc25hq80`) for save/recall compatibility; only physical JEDEC ID later decides which matches the real unit. The emulator's browser can retain saved flash between sessions; versioned saves are **not** a physical backup.

**USB and sampling:** FM1 emulator `fm1-live --usb` has a modeled USB controller and host; firmware-specific microphone/USB audio capture is **not guaranteed** until deliberately exercised. Test bridge protocol and sample/storage corruption offline; physical microphone bridge and USB behavior must be checked on the real device.

## Why not the JIANT web adaptation?

It is useful when deeply instrumenting application C code or debugging a failure that the hardware emulator hides, but maintaining another WebAssembly build adds another compilation path. Since exact `.fwsc` fidelity is the user's priority, **FM1 hardware emulator is our primary and only routine playable runtime**. Host tests are complementary: they are faster, more deterministic and offer direct source-level assertion of DSP/math/storage state.

## Limits / non-negotiable safety

- The emulator cannot guarantee a firmware will run safely on every hardware revision. Timing, audio hardware, flash chip, and power/USB interactions still require physical checks.
- Never automatically invoke installer, recovery/erase, firmware writing, flash reset, or physical USB write utilities. Codex may produce candidate packages and tests, but the human decides if/when to install only after `SAFE_FLASH_GATES.md`.
- Keep a known-good package and complete flash backups. If any chip, loader, memory limit, or recovery capability is uncertain, stop at emulator.

Verified documentation (revalidate before executing actual commands):
- FM1 emulator: https://github.com/joakimeriksson/fm1-emu
- Browser: https://joakimeriksson.github.io/fm1-emu/
- Releases: https://github.com/joakimeriksson/fm1-emu/releases
- JIANT: https://github.com/juanjiant-bit/JIANT/blob/main/BUILDING.md
