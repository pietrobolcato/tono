# AMB+ · persistent instructions for Codex (complete build v1.7)

Copy this file into the root of your dedicated AMB+ fork of JIANT. It is not firmware, not a license, and not a guarantee of safety. The human owner must personally approve each hardware install.

## Current execution scope
- If the user says **“Read START_HERE.md and execute the complete project,”** this authorises a **complete software implementation** according to `FULL_BUILD_PROMPT.md`, not an audit-only or GLO-only task. Do not stop after baseline investigation, but do not skip its safety/reproducibility checks. No permission for any physical hardware commands is implied.
- Phase exclusions: external microphone/voice recording and fullscreen music-reactive VISUAL mode; see `FULL_BUILD_PROMPT.md`. Other capabilities are required unless proved technically blocked.

## Mission and sources
- Four active layers with seven assignable roles: Pad, Drone, Bells, Bass, Drums, Texture, Sample. HOME has five **targets** (Master + 4 layers), not five audio layers.
- JIANT is the primary source for synthesis/DSP, with selected properly licensed AMB-1 generative and tape processing. Shrub is out of scope: no source, algorithms, waves, factory presets or dependencies from it.
- Keep curated ambient presets, expressive HOME macros, stable chord/scale, sparse self-running music, no sudden parameter jumps. The six EDIT pages: MODE, SOUND, ENVELOPE, LFO, PATTERN, FX. GLO pages: EVOLVE, HARMONY, GENERATE, MASTER FX, EQ, later VISUAL.
- Follow `ROADMAP.md`, `GLOBAL_GENERATORS.md`, `DEV_SETUP_MAC.md`, `FM1_EMULATOR_FIRST.md` and `SAFE_FLASH_GATES.md` in this handoff. Pin exact upstream commits, toolchain versions and license provenance before using code.

## HARD STOP: physical FM1 safety
1. **Do not flash or modify the FM1 from Codex, any agent, CI, script or scheduled job.** Never invoke installers, erase/repair/recovery functions, alter bootloader, flash layout, OTP/fuses or save partition, and never ask agents to do so unattended.
2. Stop physical installation until the owner provides an actual JEDEC flash ID, two verified complete flash backups, exported presets/settings, a known-good stock/working image, a model-specific recovery plan, and a pre-flash signoff. Known examples only: Puya `0x856014`, newer UC25HQ80 `0xB36014`; never guess from the purchase date.
3. Treat any unsupported/unknown flash ID, backup error, loader incompatibility, unexpected memory use or unexplained audio regression as **NO FLASH**.
4. `build/felucca.fwsc` is only an artifact. Compiling and browser emulation do not prove safe installation. Protect reserved sectors and confirm supported save-write bounds.
5. All real-device tests are run manually by the owner at low volume with a known rollback path. No automatic power cycling, destructive recovery rehearsal, mass erase or external power wiring.

## Standard iteration loop
- Use a separate Git branch/worktree for one narrow feature. Record source commit and changelog; never auto-update upstream.
- First baseline: unchanged JIANT `./build.sh` and `tests/run_tests.sh`, plus the exact output `.fwsc` opened in the FM1 hardware-level emulator. Do not maintain a separate JIANT Emscripten UI unless an identified blocker requires it. Save logs, native-renderer screenshots, WAV samples and CPU/RAM/flash measurements. Source of commands: JIANT `BUILDING.md`.
- Every code change: reproduce host tests; load the resulting exact `.fwsc` into the FM1 hardware emulator; inspect UI clipping/overlap at exactly 240×240; compare DSP golden files and CPU baselines. For risky work, run native `FM1_STRICT=1` and capture measured timing/report. Never blindly rewrite golden snapshots or budgets to make tests pass.
- Global GENERATE: Knobs 1–3 choose RHYTHM (8), PHRASES (8), INTERPLAY (6); Knob 4 controls VARIATION (0–100). These knobs are *absolute pots* in real hardware: use soft pickup, stable quantization/hysteresis and last-settled selection. No REC commit on this page; SELECT is inert; SEL changes GLO page. During PLAY, discrete changes queue for four-bar boundary. Manual patterns/patches/sample sources and stored layer parameters must be unchanged.
- SAMPLE: mic source requires a computer/phone bridge or independently verified USB audio path; simulation does not record sound. Ensure memory and save-slot compatibility.
- HOME uses physical MASTER potentiometer to control only the last selected HOME target, with soft takeover. Never retarget from EDIT/GLO silently. UI button LEDs only indicate actions that actually work when pressed.
- Release every milestone with artifact SHA256, source SHA, precise tests and results, CPU/RAM/flash delta, expected human steps, rollback notes, and a clear `FLASH STATUS: NOT AUTHORISED` until owner explicitly signs off.

## Final output expectations
- Be precise about what the browser HTML simulates versus what native C code can really draw/play.
- Prefer small stable improvements and ambient musical quality over unused feature breadth.
- Produce a human test card (`MILESTONE_TEST_CARD.md`) with instructions and a STOP condition before asking for any device testing.

## One emulator, multiple test levels
- The only *interactive* development emulator is `joakimeriksson/fm1-emu` (browser and/or native `fm1-live`) running our **real** `.fwsc` package. `index.html` in handoff is a visual specification, not device firmware.
- JIANT `tests/run_tests.sh` remains mandatory for regression, golden audio, UI bounds, resource budgets and storage logic. No requirement to compile JIANT `web/emu/build.sh`; it may be optional when host or hardware emulator debugging is insufficient.
- Test both `FM1_STRICT=1` native emulator and relevant device flash variant before release. An emulator pass is not hardware certification.

For Tono interface changes, follow the **Interaction pattern for new features** in ambplus/CONTROLS.md. It supersedes the original handoff’s immediate SELECT patch changes.

## Listening review for Tono sound changes

For factory songs, patches, sample loops, effects or audible generator changes, render representative WAVs from the actual playback path. Run `python3 tools/judge_audio.py candidate.wav --reference baseline.wav` with `GEMINI_API_KEY` supplied privately via the environment. Use a previous render with the same notes, seed and levels to isolate a change, or Garden when it is the musical reference. Use `--start`, `--seconds` and `--focus` to check a specific passage. Include a later passage when judging repetition or evolution. Judge the musical feeling and how it develops. For patch decisions, also render the individual sounds and ask about tone, attack, sustain and release. Match notes and levels when comparing two versions of the same patch.

Save the report alongside the test evidence. Attribute listening observations to Gemini, check its timestamped claims against the renders, and treat its suggestions as experiments. Keep the owner's listening feedback and existing DSP tests in the loop. If credentials, model access or audio are unavailable, report the listening review as pending. Only submit owner-authorized recordings to Google. Keep keys in the environment or an owner-authorized local `.env` outside version control. Exclude it from evidence and release archives.

## Creating Tono song starters

1. Write a musical brief in a sentence. Name the feeling, the lead voice, what answers it, and what quietly holds the piece together. Give layers distinct jobs. Use the existing Tono patches, effects and generator policies so the starter plays through the shared engine.
2. Start with a genuinely generative arrangement. Set the key, pace, range, density and relationships between parts. Let the rule set create the melody, accents and rests. Render at least two seeds and verify that sounding note events differ. Listen to a later window as well as the opening so a pleasant first phrase cannot mask a static loop.
3. Listen to the full mix for musical identity. Ask Gemini what the piece makes it feel, which moment earns another minute of listening, and what falls short. Explain in one or two sentences that Tono generates music live. Judge the song on its own unless the owner requests a reference. If an instrument reads as the wrong one, render it in isolation and ask for blind identification based on attack, sustained tone and decay.
4. Change one audible idea at a time. Use the same seed, notes and settings for an A/B mix or patch comparison, then match loudness and label the clips neutrally. Prefer a specific experiment that follows the feedback. Recheck the approved balance with another seed or a later passage. Use at most ten audible revisions per song, stopping early when the owner’s direction is met and Gemini supports keeping the mix. Retry transient API failures with bounded delays and report any listening verdict that remains pending.
5. Keep the accepted WAV, listening notes and source revision together. Give the owner the full path. Verify the approved render against the factory preset and follow the normal package checks before firmware release.

Last Tram is the reference workflow. It started with a tenor sax and a sparse Kawai Grand answering over a low pedal and tape noise. The early sax patch read as an organ and the piano as a celesta. A real tenor take with played vibrato, quicker attacks, the recorded piano decay and a lower piano register restored more of each instrument’s character. A loudness-matched, same-seed comparison then favored a wetter sax send. It gave the pauses a tail and left space for the piano. Gemini still wanted to keep listening when it heard an early and a late passage from the three-minute take.
