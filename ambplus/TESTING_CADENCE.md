# Owner-approved checkpoint cadence

Use targeted/native host tests and short (typically 15–90 second) exact-.fwsc emulator checks for incremental features, with relevant storage/variant coverage and short FM1_STRICT checks for DSP changes. Keep the mandatory baseline/full-host regression and source/resource checks required by AGENTS.md. Human playing can assess a concrete checkpoint once short checks pass.

Do not run a 20-minute functional soak for every incremental change. Reserve longer soaks for release milestones or investigations of crashes, leaks, long-run transport/storage or timing instability. Previous-software soak evidence must not be attributed to a new artifact. Human playing/short checks do not authorize physical installation. Record source/artifact SHA, results and a STOP condition for every checkpoint.
