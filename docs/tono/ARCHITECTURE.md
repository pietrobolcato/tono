# Tono architecture

Tono runs inside JIANT on the FM-1. The AMBPLUS build uses the same hardware abstraction, audio engine and persistence layer as the upstream firmware.

- The instrument UI, layer model and orchestration live in `firmware/src/amb_native.c`. Each of the four layers has its own role, patch, envelopes, modulation and sequence.
- The Generate page combines rhythm, phrase and interplay policies independently of the selected patches. Auto Travel can also evolve harmony, sounds, roles and generator settings. Garden synthesis and effects run as shared modules.
- `firmware/src/a22/` contains the A22 DSP ports. `eng_amb_sample.c` handles factory samples and recorded takes. The USB recording companion lives in `ambplus/sampler/`.
- `firmware/src/amb_zen.h` draws the two music-reactive visual modes. Theme, startup, and HOME navigation share the firmware renderer.
- Host tests exercise production C code with simulated I/O. The native FM-1 emulator runs the packaged firmware. Use [the optimisation checks](../../.agents/skills/fm1-optimisation-gate/SKILL.md) for changes that can affect audio headroom.
