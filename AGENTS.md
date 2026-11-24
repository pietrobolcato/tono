# Working on Tono

Tono is the AMBPLUS configuration of JIANT. Start with [architecture](docs/tono/ARCHITECTURE.md), [build instructions](docs/tono/BUILD.md), and [controls](ambplus/CONTROLS.md). Follow the interaction pattern in the controls guide.

- Review the changelog and diff against the previously packaged source. Match the checks to the affected behavior. Build and verify checksums for copy or layout releases. Use focused renderer or emulator checks when they resolve a concrete risk. Use audio, storage, timing and upgrade checks when those paths change.
- Use the [iteration](.agents/skills/fm1-safe-iteration/SKILL.md), [optimisation](.agents/skills/fm1-optimisation-gate/SKILL.md) and [release](.agents/skills/fm1-release-gate/SKILL.md) workflows where relevant.
- Every physical firmware write is owner-run. Automation uses disposable flash images. Keep user backups, recordings, local emulator state and credentials outside Git and release bundles.
- For audible changes, follow the [song-starter and listening workflow](docs/tono/SONG_STARTERS.md). Audio review is optional and uses an environment-supplied key. Send only material authorised for external review.
- Keep upstream attribution and bundled notices. Follow the [release workflow](docs/tono/RELEASING.md).
