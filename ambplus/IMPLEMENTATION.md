# AMB+ implementation ledger

FLASH STATUS: NOT AUTHORISED. No physical device actions are permitted.

## Source pins
- JIANT: beffdd0b32ffebce1d62fd608ffc653fc7feec82 (GPL-3.0-only plus notices in LICENSING.md).
- AMB-1: da0047dc468a7f4f299e0d54fd12bac0c0c11a96 (MPL-2.0; reference under review, no code imported yet).
- fm1-emu source: 2ed986561c6f26596d37fd306fac0e7558a41356 (MIT).
- Emulator release: v0.5.1, binary reports c7e47c4fc.
- SDK: d179b4484759423312073f5fbb232501aa491047, tag AC79NN_SDK_V1.2.1_2023-12-13 (Apache-2.0).
- Compiler archive: jieli-linux-toolchains-20260730.1, SHA256 a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd.
- Python environment: Pillow 13.0.0, fonttools 4.68.0.
- Docker: Debian bookworm slim digest sha256:734a72b3d6142a71993b4bc580ca01d8ca687f77a2c422d96a961d3376b179bc.

## Milestones (continue without intermediate approval)
- [ ] Unmodified build, full host suite, exact-package hardware emulator baseline; resource/audio/screenshots.
- [ ] Four active assignable layers, HOME target and soft takeover, macros and panic.
- [ ] Native Phosphor six EDIT / five GLO pages, real inputs, contextual LEDs.
- [ ] 48 distinct ambient patches and six factory pieces using licensed DSP.
- [ ] Harmony/voice leading/evolution, 22 non-destructive scheduling policies.
- [ ] Manual melodic/drum recording and locks.
- [ ] Shared FX, sends, master EQ and headroom.
- [ ] Versioned save/recall with CRC, safe overwrite and measured slot budget.
- [ ] Bounded licensed bundled SAMPLE playback subject to resource evidence.
- [ ] Strict emulator, both chip models, extended playback, release evidence and human card.

## Resolutions / investigation
The full build brief supersedes the older audit-only and single-feature stopping points. Microphone capture and VISUAL remain deferred. GLO has five active pages; VISUAL ID reserved.

Pinned JIANT has six tracks (four active already supported through trk_live); retired STRING/SAMPLE need a measured adaptation. Its HAL and panel define seven matrix quadrature encoders (SELECT, ALGORITHM, PRESETS, KNOBS 1-4) and only MASTER/battery ADC inputs. This conflicts with the handoff describing KNOBS 1-4 as absolute pots. Do not invent analog wiring; inspect emulator/hardware sources and document the verified interface before choosing an implementation.

No firmware modifications before the baseline checks. A failed check stays failed; do not rewrite golden hashes or budgets to hide it.
