# Working on Tono

Tono is the AMBPLUS configuration of JIANT. Start with [architecture](docs/tono/ARCHITECTURE.md), [build instructions](docs/tono/BUILD.md), and [controls](ambplus/CONTROLS.md). Follow the interaction pattern in the controls guide.

- Review the changelog and diff against the previously packaged source. Match the checks to the affected behavior. For straightforward Help, copy or layout changes, build the package and verify its checksum. Skip processing benchmarks, audio renders and broad regression suites for those changes. Use focused renderer or emulator checks when they resolve a concrete risk. Use audio, storage, timing and upgrade checks when those paths change.
- Use the [iteration](.agents/skills/fm1-safe-iteration/SKILL.md), [optimisation](.agents/skills/fm1-optimisation-gate/SKILL.md) and [release](.agents/skills/fm1-release-gate/SKILL.md) workflows where relevant.
- Every physical firmware write is owner-run. Automation uses disposable flash images. Keep user backups, recordings, local emulator state and credentials outside Git and release bundles.
- For audible changes, follow the [song-starter and listening workflow](docs/tono/SONG_STARTERS.md). Audio review is optional and uses an environment-supplied key. Send only material authorised for external review.
- Keep upstream attribution and bundled notices. Follow the [release workflow](docs/tono/RELEASING.md).

Keep on-device contextual Help current when changing controls, signal plots or page navigation. Update the packed cards in `tools/help/guide.txt`, regenerate them with `python3 tools/help/pack.py`, and check the overview and active knob cards on each affected page. Describe the signal a plot measures, what its curve or meters represent, and which controls affect it. Cover both LFO slots, including switching and turning modulation off, whenever their operation changes.
