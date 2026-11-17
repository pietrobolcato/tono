# AMB+ native firmware

Build and open the exact firmware package:

```sh
./ambplus/build.sh
./ambplus/test.sh
./ambplus/launch-emulator.sh build/felucca.fwsc
```

All scripts above operate locally. The emulator launch disables host MIDI inputs and does not enable its USB bridge. Keep the original package in `../TEST_REPORTS/baseline/jiant.fwsc` as an emulator reference.

The handoff's dependencies live beside this repository in `../dependencies/`: the pinned JieLi compiler, AC79 SDK, Python environment and official fm1-emu v0.5.1 binary. `ambplus/build.sh` supplies those paths and pins the Docker image by digest. Override PYTHON, JIELI_TOOLCHAIN, AC79_SDK or AMB_EMULATOR for another location. See BUILDING.md and IMPLEMENTATION.md for the source and dependency pins. Compilation uses JIANT's unmodified package builder and loader sources.

The default `./build.sh` with AMBPLUS unset still builds upstream JIANT. This keeps the original regression/golden baseline available. For the complete upstream suite, build with `AMBPLUS=0`, then run `tests/run_tests.sh` with the SDK and Python dependencies available. Preserve the AMB+ candidate first. In this workspace, `../baseline-firmware` is a separate pinned worktree for those checks. Sanitizers are also run in Linux because this Mac's sanitizer probe stalled; the skip is explicitly reported rather than counted as a sanitizer pass.

The targeted AMB+ tests exercise real C DSP and bitmap rendering against hardware stubs. They are noninteractive fixtures, not another playable synthesizer. The hardware emulator executes the actual pi32v2 firmware package, including flash command support, DMA, interrupts and the native LCD.

## Implemented storage contract

Eight 1400-byte AMB+ records fit the existing 3840-byte A/B payload capacity. Slots use existing project objects 0..3 at 0x97000..0x9efff and song objects 0..3 at 0xa0000..0xa7fff. No flash layout or reserved-sector bounds were changed. Writes occur only through explicit stopped SAVE/REC; AMB+ disables upstream session autosave and mutation. Old JIANT payloads are treated as occupied and incompatible, requiring explicit overwrite; they are never silently converted. Returning to JIANT will not interpret an AMB+ payload as its own project.

AMB+ record version 2 stores complete parameters, roles, macros, manual patterns, harmony, conductor IDs/variation/seed, effects and EQ. A same-layout version-1 experimental record defaults the missing conductor semantics to the AMB defaults/variation 40. Unknown sizes, versions, invalid IDs or out-of-range parameters are rejected; A/B headers and payloads use upstream CRC and readback verification. There is no claim of a general converter for arbitrary external firmware song formats.

## DSP/source choices

48 recipes use JIANT ANALOG, PHASE, WHEEL, NOISE, FM6 and DRUM-X. PHASE/FM/analog strings replace the retired STRING/resonator engine rather than exposing a dead STRING menu. Each drum part selects its own immutable kit, using existing per-part runtime voice state. Shared JIANT reverb, delay/tape drift, chorus, auto-pan and saturation remain the real DSP paths. Per-layer send pages expose the actual available send/filter/pan/drive controls; they do not claim separate per-layer reverbs or unsupported per-layer predelay/duck engines.

The SAMPLE engine appends ID 15, preserving all legacy engine numbers. Two original, offline-generated 0.743 s PCM sources consume 32768 bytes of flash and no PCM RAM or writable sample sectors. `tools/gen_amb_samples.py` reproduces them. No voice recording or microphone bytes are fabricated.

The AMB-1 Euclidean and voice-leading helpers remain in `firmware/src/amb_harmony.h` under MPL-2.0, pinned to da0047dc468a7f4f299e0d54fd12bac0c0c11a96. Its notice and full license are included. All AMB+ scheduling/runtime additions are GPL-3.0-only with JIANT; see LICENSING.md and LICENSES/. No Shrub sources, samples, presets or dependencies were used.

See CONTROLS.md, the final test report and the milestone card for validation and remaining acceptance work. No successful software test authorizes hardware installation.
