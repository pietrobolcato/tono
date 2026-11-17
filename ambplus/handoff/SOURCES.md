# Sources verified for v1.5 planning (17 Nov 2026)

1. JIANT upstream source and build guide — https://github.com/juanjiant-bit/JIANT and https://github.com/juanjiant-bit/JIANT/blob/main/BUILDING.md . macOS Docker / JieLi toolchain + SDK, `./build.sh`, `web/emu/build.sh`, `tests/run_tests.sh`, actual native renderer clipping tests, golden audio hashes, CPU budget; install warnings and rollback.
2. AMB-1 generator baseline — https://github.com/charlesvestal/fm1-amb and https://github.com/charlesvestal/fm1-amb/blob/main/BUILDING.md . Unequal-length loops, Euclidean rhythmic events, evolving harmonies and host/browser simulation.
3. AMB licensing — https://github.com/charlesvestal/fm1-amb/blob/main/LICENSING.md . Current AMB main `MPL-2.0`, older releases `GPL-3.0-only`. Verify each exact file and combined-license obligations at integration time; JIANT is GPL-3.0-only.
4. FM1 flash recovery / chip variants / M-Flasher — https://thefirmwarehub.com/tools/rescue/ . Puya ID `0x856014`, UC25HQ80 `0xB36014`, M-Flasher Mac support, full flash backup and rescue ladder.
5. OpenAI Codex `AGENTS.md` — https://learn.chatgpt.com/docs/agent-configuration/agents-md . Project-level persistent instructions discovery.
6. OpenAI Codex Skills guidance — https://learn.chatgpt.com/docs/learn/best-practices . `.agents/skills/.../SKILL.md` for reusable workflows.
7. April OS example for voice/microphone bridge — https://github.com/beashoko/april-os . A reference to investigate, not proof that JIANT has sample storage or that direct USB microphone input works.

Sources may change. Codex must pin exact source revisions, verify hashes, licenses and up-to-date installer support before developing/launching firmware. This package does not contain copyrighted JIANT/AMB source files; it includes only original planning text, UX simulator and screenshots.

FM1 hardware-level emulator (direct `.fwsc` loading, browser save/flash options, native `fm1-live`, strict mode):
- https://github.com/joakimeriksson/fm1-emu
- https://joakimeriksson.github.io/fm1-emu/
- https://github.com/joakimeriksson/fm1-emu/releases
JIANT source build, optional separate Emscripten mode, mandatory host tests:
- https://github.com/juanjiant-bit/JIANT/blob/main/BUILDING.md
