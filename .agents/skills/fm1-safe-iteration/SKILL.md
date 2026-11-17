---
name: fm1-safe-iteration
description: Develop and test one AMB+ FM1 feature in JIANT, with host/browser tests and a human test card, without touching or flashing physical hardware.
---

# Safe AMB+ iteration
1. Read project `AGENTS.md`, `ROADMAP.md`, `GLOBAL_GENERATORS.md` and pinned JIANT `BUILDING.md`.
2. Log exact base commit and scope; make a fresh Git branch/worktree. Write targeted tests before coding.
3. Implement one small change; do not modify bootloader or flash sectors. A new layout needs explicit migration review.
4. Run `./build.sh` and `tests/run_tests.sh`, plus `web/emu/build.sh` if Emscripten is installed. Inspect native UI clipping, golden DSP tests and CPU baselines; never blindly refresh snapshots to make tests pass.
5. Record source SHA, firmware SHA256, resource deltas and a low-volume human acceptance procedure. Produce a filled `MILESTONE_TEST_CARD.md`.
6. Output `FLASH STATUS: NOT AUTHORISED` until owner explicitly authorizes manual installation after backup and recovery checks.

Never flash, reset, erase, power-cycle or recover a device from this skill. Browser simulation is not proof of FM1 compatibility.
