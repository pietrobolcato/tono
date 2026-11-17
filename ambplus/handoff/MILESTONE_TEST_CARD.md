# AMB+ — human milestone test card (template)

**Milestone / feature:** _____________________

**Git source SHA:** _____________________  **JIANT upstream SHA:** _____________________  **AMB component SHA:** _____________________

**Build artifact (absolute local path):** _____________________

**Artifact SHA256:** _____________________

**Device JEDEC chip ID and firmware:** _____________________  **Flash backups verified:** YES / NO

**Hardware action authorized?** NO by default. **FLASH STATUS: NOT AUTHORISED**

## Codex completed (do not mark PASS without observed result)
- [ ] `./build.sh` — pass/log path: ______
- [ ] `tests/run_tests.sh` — pass/log path: ______
- [ ] FM1 hardware emulator loads exact `.fwsc` + interactive UI/audio inspection — pass/log path: ______
- [ ] Native UI 240×240 clipping/overlap screenshots — pass/log: ______
- [ ] Audio golden tests + worst-case CPU/late/shed budgets — baseline/delta: ______
- [ ] Flash footprint, reserved sectors and migration tests — notes: ______
- [ ] Global conductor changed local layer state? NO, verified with serialized compare: ______
- [ ] Known-good rollback image and procedure available: ______

## What the human tests **only after signing off on a manual flash**
1. At low volume verify startup, PLAY/STOP, physical controls and emergency output mute/fade.
2. Open GLO / GENERATE; move Knobs 1–3 **one at a time** and verify each of the three names and graphs changes. SELECT must do nothing here; SEL advances the GLO page.
3. Enter/leave GLO with pots at different positions: no unexpected style or volume jump. Test pickup and hysteresis near category boundaries.
4. During PLAY, choose a different rhythm/phrase/interplay. Verify audible changes at the next 4-bar grid, with smooth effect tails and no stuck notes.
5. Make and store a manual drum pattern. Swap global generation styles. Confirm the manual steps are identical.
6. Save a small test composition, confirm occupied overwrite guard, ensure flash writing completes, *then* power-cycle and recall. Confirm all settings including Variation.
7. Listen for 20–30 min, note CPU/late audio blocks, clipping, output level and musical spaciousness.

**STOP and do not retry** for unknown chip/installer mismatch, failed backup, freeze, crash, clipped or distorted output, missing persistence, high CPU/audio late blocks, unreliable LEDs/buttons, or inaccessible recovery route.

**Actual results / issues and owner signoff:** _____________________
