# AMB+ developer environment — macOS + Codex (planning checklist)

**Recommended:** develop in a **separate pinned fork of upstream JIANT**, on a Mac with Docker Desktop. Use VS Code/Codex or Codex CLI in that folder. Do **not** use the physical FM1 for normal edit/test iterations. All commands in this document are source setup/build/test only: none flashes the device.

## 1. Create an isolated repository

```bash
git clone https://github.com/juanjiant-bit/JIANT.git ambplus-jiant
cd ambplus-jiant
git submodule update --init --recursive
git rev-parse HEAD  # record this exact base revision
git switch -c ambplus/initial-ui
```

Pin and record upstream revision and external modules; do not track `main` automatically. Keep JIANT license and component LICENSES directory. Add the **supplied** `AGENTS.md`, `ROADMAP.md`, `GLOBAL_GENERATORS.md`, `SAFE_FLASH_GATES.md` and `.agents/skills/` into the root of this local fork. Open Codex **from that directory** and ask it first to summarise `AGENTS.md` and review the repo without modifications.

## 2. Native dependencies (JIANT `BUILDING.md`)

- Docker Desktop (required for JieLi x86-64 Linux tools on macOS; Apple silicon Docker can emulate linux/amd64). Allow Docker to share your source folder under `/Users`.
- Python 3 with `Pillow` and `fonttools` to rasterise the firmware UI fonts/icons.
- JieLi toolchain supplied by project's `tools/get_toolchain.sh` (installs under `~/.jieli/toolchain`). Verify package origin and hashes before running scripts from external repos.
- JieLi AC79 SDK branch `AC79NN_SDK_V1.2.1_2023-12-13` from `https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git`, in `~/fw-AC79_AIoT_SDK`. JIANT includes none of the required SDK package files by itself.
- Node.js is optional for web tests. Emscripten (`emcc`) is **not required** for the recommended workflow. We deliberately use the separate **FM1 hardware-level emulator** for interactive firmware playback instead of JIANT's Emscripten application build.
- JIANT `BUILDING.md` is the definitive reference: `https://github.com/juanjiant-bit/JIANT/blob/main/BUILDING.md`.

Examples (run only after reviewing the pinned JIANT files):

```bash
python3 -m venv .venv && source .venv/bin/activate
python3 -m pip install Pillow fonttools
./tools/get_toolchain.sh
# SDK (one-time):
git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
  https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK
./build.sh                               # produces build/felucca.fwsc; no flashing
tests/run_tests.sh                       # host and regression tests
# Open build/felucca.fwsc in the FM1 hardware emulator (not JIANT web/emu):
# https://joakimeriksson.github.io/fm1-emu/ → Open .fwsc…
# Optional native executable downloaded from fm1-emu releases:
# ./fm1-live --fwsc /absolute/path/to/build/felucca.fwsc
# Native, stricter hardware-timing model (slower):
# FM1_STRICT=1 ./fm1-live --fwsc /absolute/path/to/build/felucca.fwsc
```

**Do not mistake** the included `index.html` for firmware. It is a UX simulator only. Our default interactive test environment is the **FM1 hardware emulator**, which loads the exact device `.fwsc`; JIANT's separate `web/emu/build.sh` is optional and **not part of the normal loop**. Native `fm1-live --usb` offers some USB simulation, but actual device USB/microphone sampling must still be validated physically. Read `FM1_EMULATOR_FIRST.md`.

## 3. Standard continuous engineering loop

- Branch / single feature → host test → compiled `.fwsc` in FM1 hardware emulator → native 240px screenshots → CPU/RAM/flash-size report → human review → *optional* carefully approved hardware test → tag known-good release.
- Run preflight **before any candidate is sent to a user**. `tests/run_tests.sh` covers persistent flash, UI text bounds, key events, engines, CPU and golden audio regressions. Compare `tests/golden.txt`, `tests/cpu_baseline.txt` and `tests/target_budget.txt` intentionally.
- Use `git worktree` so Codex can explore another task without disturbing the release candidate. Freeze toolchain/compiler versions and dependencies once an artifact works.
- For each milestone keep `TEST_REPORTS/YYYY-MM-DD-feature.md` recording source SHA, firmware SHA256, exact test commands/outcomes, CPU late/shed metrics, WAV diffs, screenshot diffs, known bugs, risk and recovery plan.
- Keep a *known-good* binary with its exact source commit separate from experimental `.fwsc` files. Do not let automation choose the latest unknown build to install.

## 4. Codex setup — v1.7 full-build scope

- `AGENTS.md` is the correct filename and contains binding no-flash safety guidance. Keep both reusable skills under `.agents/skills/`.
- **Start Codex with one instruction:** `Read START_HERE.md and execute the complete project.`
- `FULL_BUILD_PROMPT.md` is the canonical end-to-end software assignment. It replaces the earlier audit-only / single-page prompts. Baseline audits, native compilation, tests and emulation are internal milestones, **not places to stop awaiting approval**.
- Only the **FM1 hardware-level emulator** is the playable interactive runtime for compiled `.fwsc`; JIANT host regression tests are kept for fast diagnosis. Do not build a second HTML-only audio engine.
- No software progress authorises touching a physical FM1. See `SAFE_FLASH_GATES.md`.

## 5. Hardware boundary

**No device installation is implicit in any successful command above.** Before hardware trials read `SAFE_FLASH_GATES.md`; use a read-only chip-detection/backup utility, confirm USB data cable and compatibility, and have a human operate the installer only after approval. JIANT warns that flashing directly over another custom firmware can brick the FM1; its installer recommends returning to official firmware first with backup. Do not automate that sequence.

## Why FM1 hardware emulator is the only interactive emulator

A single **device-package** `.fwsc` means the displayed firmware, hardware-modeled peripherals, and candidate for the physical FM1 are the same compiled artifact. No parallel Emscripten application build to maintain or confuse with the release. Keep `tests/run_tests.sh` for deterministic failure diagnosis and fast CI feedback. The hardware emulator does **not** eliminate those tests or the physical-device gate. See `FM1_EMULATOR_FIRST.md`.
