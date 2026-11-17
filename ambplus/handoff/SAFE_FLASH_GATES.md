# FM1 safety gates — human operator, before every flash

A known recovery route exists, but **the FM1 can still become unbootable or lose user data.** This package never flashes hardware.

## Gate A — identify and preserve, read-only

- [ ] Actual flash JEDEC ID observed on this device and recorded (examples only: Puya `0x856014`, newer UC25HQ80 `0xB36014`). Record current firmware/version, USB behaviour and device model. *Do not infer from the purchase date.*
- [ ] Compatible tool installed on Mac: **M-Flasher** supports read-only flash backup and both known chips. Verify provenance at its GitHub source/releases. FM1 firmware hub rescue guide: https://thefirmwarehub.com/tools/rescue/ .
- [ ] Two independent complete flash readbacks of the actual device, expected size, hashes compared; preserve original backups in two physical/cloud locations. Never share full flash dumps publicly.
- [ ] User presets/songs/samples/settings exported separately, with a copy that can be inspected. Do not wipe a sample bank without explicit acknowledgement.
- [ ] Known-good original or current firmware package obtained with checksum and exact model/chip support. Device-specific recovery path and a tested *read-only recognition* of ROM/download mode recorded.
- [ ] Operator understands macOS may show a seemingly unreadable disk for bootloader; choose **Ignore**, never initialise/format it.

**If anything in Gate A is missing: NO FLASH.** Continue only with local simulation/tests.

## Gate B — candidate validation, still offline

- [ ] Exact source commit, toolchain, SDK, base JIANT revision and external AMB component hashes recorded; licenses/notices reviewed (current AMB main MPL-2.0, JIANT GPL-3.0-only; older AMB GPL).
- [ ] `./build.sh` and `tests/run_tests.sh` pass; the **exact `.fwsc`** boots/plays in FM1 hardware emulator, native `FM1_STRICT=1` timing test and target screen audit pass. JIANT `web/emu` is optional, not required.
- [ ] Binary package identity/loader, flash storage layout and reserved-sector write limits checked against this hardware and current installer; candidate SHA256 recorded.
- [ ] No unexplained CPU/RAM/stack/large-buffer regression. Worse-case four-layer audio with reverb+delay+LFO evaluated on host, and on-device only after signoff.
- [ ] Save-data compatibility/migrations designed and tested. If song slots and SAMPLE storage collide, stop and decide allocation explicitly.
- [ ] Release card states **READY FOR HUMAN DECISION — NOT FLASHED**. A specific human approval is needed for **this exact artifact**; general permission to develop code is not permission to install it.

## Gate C — manual device acceptance (only after human approval)

1. Disconnect DAW/other MIDI apps and browsers using device. Check stable USB data cable, power, correct package and supported chip. Have the recovery notes and backups immediately accessible.
2. The human manually follows the verified installer's published procedure. **Codex does not press Install, issue a USB write or invoke `fm1_install.py`.** If installer shows a mismatch or error: stop—never keep retrying blindly.
3. First boot at low headphone/line volume: screen readable, no crash, PLAY/STOP safe, 4 knobs and SELECT/SEL correct, no loud click or continuous tone. Switch MASTER/individual target without volume jump.
4. Play an AMB-default four-layer piece at low level. Test GLO / GENERATE knobs 1–3 one by one: each style independently, no accidental changes on entry. Knob 4 variation. Inspect queued phrase transition. Test no alteration of saved manual patterns.
5. Test EDIT/SOUND patch, LFO, MODE/REC commit, SAVE/REC overwrite guard, GLO/HARMONY, FX tails. Monitor CPU peak/late/shed blocks using supported firmware diagnostics.
6. Save a small test user piece; **after verifying persistence is safe for the detected chip** and operations are complete, manually power cycle and restore. Compare save slot, three styles and variation; preserve existing user data.
7. 20–30 minute playback and modest stress test. Stop if any hard audio glitch, GUI freeze, heat/power abnormality, corrupted memory, failed save, missing update route or audio runaway. Document issue and roll back using the approved recovery plan.

## Recovery is a contingency, not a test gimmick

M-Flasher supports macOS/Windows/Linux and full-flash readback; the firmware hub documents ROM download-mode recovery and, as a last resort, an FM1 Transporter using a Seeed XIAO RP2040. The newer UC25HQ80 can cause old builds to *appear* to save in RAM only or reject updates. This is not automatically a brick; **do not repeatedly install**. Prefer documented support/recovery for the exact chip and build; get human help if the device does not match the guide.

Sources: https://thefirmwarehub.com/tools/rescue/ · https://github.com/juanjiant-bit/JIANT/blob/main/BUILDING.md · https://github.com/charlesvestal/fm1-amb/blob/main/BUILDING.md .

## Emulator does not authorize flashing

Simulated flash saves, USB and second-core handling are useful evidence, but only read-only JEDEC identification, verified complete backups, compatible loader/recovery, and human authorization can enable a physical test. A fast emulator never establishes real CPU headroom; strict mode should be assessed and real hardware monitored after a supervised install.
