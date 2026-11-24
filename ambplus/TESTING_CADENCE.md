# Owner-approved checkpoint cadence

Start with the changelog and the diff since the last packaged source. Follow **Choose release checks from the changelog** in AGENTS.md. For a small UI-only release, build and verify the package and record any skipped tests. For changes to behavior, use targeted host and short exact-firmware emulator checks for the affected paths.

Use longer runs when needed to measure real-time headroom, evolution over time, or suspected instability. Record source and artifact hashes, any earlier evidence with its version, and the result of the checks selected for this release. The owner decides on physical installation.
