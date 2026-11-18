# AMB+ 0.4 validation — 2026-11-18

Source ea8d65175e3f96fbe45a7a301f1ff00557263d00, branch ambplus/natural-transitions, previous source 53b065f670dfbc43ef091b312c5faa5faedfe00e.
Artifact SHA256 fd5114000b5f42961c3f85653b47c2c9805f70c1e9dc6d82cd3bb838038d88bf. Two clean builds match byte-for-byte. App 375344 B (−820), static RAM 75784 B (−68), RAM code 21444 B (−580), pool 303552 B (unchanged), spare pool 15936 B, package 610113 B (unchanged).

PASS: complete AMB+ targeted tests, including all roles/patches, storage recovery, note pools, recording, controls, diverse harmony and ten automatic chord changes with no source fader/forced portamento. Actual key/scale/patch/role inputs preserve shared FX state and buffers. Direct knob update checked. Native renderer: 178 frames at exactly 240×240, zero clipping/bounds/overlap. Five Linux ASan/UBSan fixtures pass, upstream signed-overflow/shift excluded and leak detection disabled; Mac ASan remains skipped because baseline probe hangs.

Exact package in official fm1-live v0.5.1: 95-second scripted changes, peak 29%, no late/shed/audio_lost; 45-second patch run peak 28%, no late/shed/audio_lost. Captured peaks 7403/8130, no full-scale samples. Screens are real firmware; host C audit images are separately identified. The HTML handoff is a visual specification.

20-minute functional fast-emulator soak: peak/late/shed [31, 0, 0], audio_lost [0], boots 1; 388.08 s wall, 3.09× real time. Includes explicit changes and continued AUTO. Audio was not captured for the long soak; the shorter WAV runs provide audio evidence. Fast emulation is separate from instruction-timed strict stress.

Strict 240 MHz: FAIL, peak 100%, max half 2915 µs, 3 late, 9 sheds, audio_lost 0. Previous source measured 97%, 2839 µs, 3 late, 5 sheds: current stress is worse, not a performance pass. Strict 320 MHz: PASS, peak 77%, max 2246 µs, 0 late/shed/audio_lost. Clock source/default was not changed; 320 MHz does not establish the owner's hardware clock.

Full baseline host suite: FAIL overall, 157 unchanged golden renders, 0 health/voice/routing failures/crashes, 22 CPU-budget entries and two inherited web-editor mock-schema failures. No goldens/budgets edited. Emscripten is absent and its separate interactive emulator is not required. Host installer tests use mocks, not hardware.

Puya 0x856014: separate-process save/recall payload equality, including custom pool 0x5af and retained manual data. UC25HQ80 0xB36014: SAVE REFUSED with zero flash operations; pinned driver only whitelists Puya. No driver/loader/flash-bound changes. Save v3 remains 1400 B.

Known limitation: patch/type changes retain FX tails but still use JIANT's 2.9/11.6 ms old-engine handoff, so the full dry release is cut. Owner reports this can sound abrupt. The next iteration will wait for the old release with a configurable cap. Removal of the musical source fader does not solve this separate engine handoff.

Evidence: sibling evidence directory and original TEST_REPORTS/natural-transitions logs, WAVs, scripts, RAM captures and timing JSON. Sources/licenses/toolchain pinned in SOURCES_LOCK.json. Human acceptance is pending. No physical FM1 was accessed. FLASH STATUS: NOT AUTHORISED.
