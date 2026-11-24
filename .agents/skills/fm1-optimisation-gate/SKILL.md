---
name: fm1-optimisation-gate
description: Verify that a Tono -opt release retains real-time audio headroom in the strict native FM-1 emulator. Use before packaging or reviewing a new optimised firmware for flashing.
---

# Tono audio performance review

First apply the changelog and diff review in AGENTS.md. Use the extended checks below when relevant to the change. For a lightweight package, record build and checksum evidence, attribute prior evidence to its version and leave hardware signoff to the owner.

1. Record the candidate package SHA-256, matching ELF and source commit. Use the last accepted `-opt` build and its saved traces as the baseline. Validate the exact package being handed to the owner.
2. Run default Garden for 180 seconds after PLAY in the native emulator at 240 MHz, with strict timing and the owner’s flash model. Keep default Auto Travel. For UC25HQ80 use `python3 TEST_REPORTS/garden-budget/benchmark.py /absolute/path/to/tono.fwsc RELEASE_LABEL`. The matching `tono.elf` must be beside the package. For another supported chip adapt the same fixture without changing the audio workload. Preserve report JSON, timed inputs and the audio sample.
3. Repeat the established heavy-FX fixture in `TEST_REPORTS/garden/emulator/travel-defaults-verified/heavy/inputs.txt` against the same exact build. Include focused traces for changed real-time paths, such as Visual rendering or USB audio. Report callback peak and time, deadline misses, shed voices, lost buffers and restarts. The Garden target is under 100% with zero late, shed or lost callbacks. Compare added stress against the accepted baseline and document any explicit owner-approved exceptions.
4. Record flash, static RAM, fast-code and pool headroom against the previous release. Keep audio latency and load-governor validation comparable. When a change is meant to preserve the sound, compare a deterministic render against the previous output. Describe intentional audible changes separately from regressions. Keep pre-existing generic harness warnings visible.
5. Add the measured results to the release card. When a test is incomplete, label the performance review pending. The performance check is emulator evidence and feeds the separate `fm1-release-gate` before a physical install decision.
