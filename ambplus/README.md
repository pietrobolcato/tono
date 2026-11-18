# Tono native firmware

Tono is the instrument formerly named AMB+. Its 240×240 display uses an off-white panel, charcoal type and an orange activity index. The rotary startup animation settles over 800 ms and holds for 900 ms. Reduced motion opens on the settled dial. [Instrument Panel release](../../release/tono-0.12/README.md).

The `ambplus/` scripts, build flag, save record format and package identity retain their established names for compatibility.

Build and open the exact firmware package:

```sh
./ambplus/build.sh
./ambplus/test.sh
./ambplus/launch-emulator.sh build/felucca.fwsc
```

All scripts above operate locally. The emulator launch disables host MIDI inputs and does not enable its USB bridge. Keep the original package in `../TEST_REPORTS/baseline/jiant.fwsc` as an emulator reference.

The handoff's dependencies live beside this repository in `../dependencies/`: the pinned JieLi compiler, AC79 SDK, Python environment and official fm1-emu v0.5.1 binary. `ambplus/build.sh` supplies those paths and pins the Docker image by digest. Override PYTHON, JIELI_TOOLCHAIN, AC79_SDK or AMB_EMULATOR for another location. See BUILDING.md and IMPLEMENTATION.md for the source and dependency pins. Compilation uses JIANT's unmodified package builder and loader sources.

The default `./build.sh` with AMBPLUS unset still builds upstream JIANT. This keeps the original regression/golden baseline available. For the complete upstream suite, build with `AMBPLUS=0`, then run `tests/run_tests.sh` with the SDK and Python dependencies available. Preserve the AMB+ candidate first. In this workspace, `../baseline-firmware` is a separate pinned worktree for those checks. Sanitizers are also run in Linux because this Mac's sanitizer probe stalled; the skip is explicitly reported rather than counted as a sanitizer pass.

The targeted Tono tests exercise real C DSP and bitmap rendering against hardware stubs. The hardware emulator executes the actual pi32v2 firmware package, including flash command support, DMA, interrupts and the native LCD. To capture the settled splash, run `fm1-live --fwsc /absolute/path/to/felucca.fwsc --midi none --shot 1400 --lcd-only` in an empty working directory. The capture is written to `work/lcd.ppm`.

## Implemented storage contract

Eight 1420-byte AMB+ records fit the existing 3840-byte A/B payload capacity. Slots use existing project objects 0..3 at 0x97000..0x9efff and song objects 0..3 at 0xa0000..0xa7fff. No flash layout or reserved-sector bounds were changed. Writes occur only through explicit stopped SAVE/REC; AMB+ disables upstream session autosave and mutation. Old JIANT payloads are treated as occupied and incompatible, requiring explicit overwrite; they are never silently converted. Returning to JIANT will not interpret an AMB+ payload as its own project.

AMB+ record version 4 stores complete layer parameters, roles, macros, manual patterns, harmony, conductor IDs/variation/seed, effects, master/layer EQ and the release cap. The original 1400-byte prefix remains intact; v4 appends 20 bytes for a 1420-byte payload. Versions 1–3 migrate with flat layer EQ and a 3000 ms cap; v1 gets default conductor semantics and v1/v2 get the stock scale pool. Older software cannot read v4. Keep old projects/exports and separate emulator state for rollback. Unknown sizes, versions, IDs or ranges are refused. Existing A/B CRC/readback verification and sector bounds are unchanged; no general external-firmware converter is claimed.


## DSP/source choices

There are 74 patch choices across Pad, Drone, Bells, Bass, Drums and Texture, plus two PCM sample sources. Twelve factory pieces generate all four layers. Garden combines the selectable Garden conductor with Sine FM Rhodes, Bell and Pedal patches, Soft Pulse drums, a stereo plate and tape delay. Six additional Sine FM recipes extend that palette to the Drone role and add keyboard, bell and bass variations. Departure Hall uses unequal-length piano cycles, Stone Cloister has a modal chant generator and Window Seats generates its piano line. PHASE/FM/analog strings replace the retired STRING/resonator engine rather than exposing a dead STRING menu. Each drum part selects its own immutable kit, using existing per-part runtime voice state. Shared JIANT reverb, delay/tape drift, chorus, auto-pan and saturation remain the real DSP paths. Per-layer send pages expose the actual available send/filter/pan/drive controls; they do not claim separate per-layer reverbs or unsupported per-layer predelay/duck engines.

The SAMPLE engine appends ID 15, preserving all legacy engine numbers. Two original, offline-generated 0.743 s PCM sources consume 32768 bytes of flash and no PCM RAM or writable sample sectors. `tools/gen_amb_samples.py` reproduces them. No voice recording or microphone bytes are fabricated.

The AMB-1 Euclidean and voice-leading helpers remain in `firmware/src/amb_harmony.h` under MPL-2.0, pinned to da0047dc468a7f4f299e0d54fd12bac0c0c11a96. Its notice and full license are included. The Garden conductor, Sine FM and Soft Pulse voices and shared space effects also retain MPL-2.0; see LICENSING.md and LICENSES/. No Shrub sources, samples, presets or dependencies were used.

See CONTROLS.md, the final test report and the milestone card for validation and remaining acceptance work. No successful software test authorizes hardware installation.

## Candidate acceptance status

Tono 0.10 and the earlier Garden release are documented in [the release validation report](../../release/tono-0.10/VALIDATION_REPORT.md) and [the preceding Garden report](GARDEN_VALIDATION_REPORT.md). For the current package and listening examples, see the [Tono playing guide](../../release/tono-0.10/README.md).

The performance-feedback candidate is under `../../release/ambplus-0.2-feedback/`. See CONTROLS.md for HOME note-pool editing, SUSTAIN, generated overdub, detailed sound groups, track restore and emulator combinations. The previous v0.1 release and evidence remain preserved.

Latest v0.6 checkpoint: `../../release/ambplus-0.6-transitions-textures/`. See PLAYING_CHECKPOINT_VALIDATION_REPORT.md and CONTROLS.md for uniform active-synth transition duration, style/whole-piece transitions, transient selected-layer gain, steady unpitched factory textures and manual rest entry. Save layout remains v4/1420 B; old PHASE texture snapshots remain readable. Older v0.5 may reject records using new Noise Texture recipes 2/4. No new 20-minute soak for this incremental checkpoint; use short checks/human playing, with longer soaks at milestones.
