# AMB+ — START HERE · Execute the complete firmware build

> **One-command handoff for Codex.** The user will say: **“Read START_HERE.md and execute the complete project.”** That is sufficient instruction. This file is the canonical entry point for AMB+ v1.7.

## Mission

**Implement the entire AMB+ ambient instrument as real JIANT-based M-VAVE FM1 firmware, get it genuinely playing in the Joakim Eriksson FM1 hardware-level emulator via a compiled `.fwsc` package, and validate it thoroughly.** This is not an audit-only assignment, not a UI-only implementation and not a separate HTML/browser synthesizer. Voice/microphone capture and the purely visual performance mode are deferred; the other features in `FULL_BUILD_PROMPT.md` are in scope.

## Read order

1. **`AGENTS.md`** — binding physical-device safety and scope guardrails.
2. **`FULL_BUILD_PROMPT.md`** — **primary execution instructions and definition of done**. Perform the whole task; don't stop after writing a plan.
3. **`ROADMAP.md`** — rich product/music/UI specifications and milestones.
4. **`GLOBAL_GENERATORS.md`** — unified global GENERATE behavior, 22 strategies and non-destructive rules.
5. **`FM1_EMULATOR_FIRST.md`, `DEV_SETUP_MAC.md`, `SAFE_FLASH_GATES.md`** — real firmware testing environment and hardware restrictions.
6. **`index.html` and `screens/`** — UX/visual reference only (NOT working firmware or sound).
7. **`MILESTONE_TEST_CARD.md`, `.agents/skills/`, `SOURCES.md`** — human test protocol, agent workflows and source provenance.

## Scope precedence

- **Safety, law and licenses are non-negotiable.** Never auto-flash, erase, reprogram or access a physical FM1. Firmware emulation and non-destructive host tooling are permitted.
- This file and `FULL_BUILD_PROMPT.md` **replace the older sequential prompts** in previous versions of `CODEX_START_HERE.md` and `DEV_SETUP_MAC.md`. A baseline audit/build/test is step 1 of the complete assignment, **not a point to stop for confirmation**.
- Use **JIANT** synthesis/effects, **AMB-1** generative concepts and selected legally reusable effects. **Do not use Shrub** or its proprietary sounds/code.
- **One playable runtime:** the compiled `.fwsc` in the **FM1 hardware-level emulator**. Keep JIANT's native/host automated tests; the standalone HTML is just the design reference.
- Phase exclusions: external microphone/voice capture and full-screen music-reactive landscapes/visual mode. A bounded SAMPLE playback engine from a legally reusable bundled source is desirable and should be implemented if the proven memory budget permits. Do not fake recording.
- Keep implementing testable milestones until the full software scope is delivered or a real blocker is recorded. Do not silently omit features.

## Where to work

This ZIP is **a specification and tooling handoff, not the JIANT source repository**. If there is no JIANT checkout already, create an appropriate `firmware/` Git checkout from the verified JIANT upstream, pin the source revision, record your base commit and read upstream `BUILDING.md`. Preserve this handoff and its safety instructions in the working repository. Download/build dependencies using the verified upstream instructions, and do not execute any physical firmware installer.

## What success must produce

- Reproducible JIANT-based AMB+ source code and version-pinned toolchain.
- An actual compiled, bootable `.fwsc` demonstrated in the **FM1 hardware emulator** with real audio output, not screenshots alone.
- Implemented four-layer synthesis, curated ambient patches/macros, six EDIT pages, five-target HOME, GLO/EVOLVE/GENERATE, 22 global scheduling strategies, local sequencers/recording, shared FX/EQ, safe save/recall, controls and contextual LEDs, as specified in `FULL_BUILD_PROMPT.md`.
- Host and emulator test reports, native 240×240 screenshots, WAV listening samples, binary/source hashes, CPU/memory/flash measurements, persistence tests and documented failures/limits.
- A release folder containing the validated emulator candidate, simple emulator launch instructions and a human physical-device acceptance checklist.
- **Prominent release label:** `EMULATOR-VALIDATED; NOT HARDWARE-VALIDATED`. Until hands-on device testing, do not claim 1:1 safety or that the unit cannot brick.

## Your next action

Read all of the above and **begin implementing**. Use a small-commit, test-after-each-feature workflow internally, but finish the full scope. If time, access or a verified hardware constraint prevents completion, deliver the working source and tests, explicitly mark unfinished items and identify the precise blocker—never report a stub as completed.
