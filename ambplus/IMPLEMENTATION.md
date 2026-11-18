# AMB+ implementation status

Latest: v0.4 natural transitions and consistent guidance; branch ambplus/natural-transitions.

The 1.2-second outgoing / 2.4-second incoming source controller is removed in full, including its mixer hook and runtime state. AUTO, manual queued chords, key/scale/pool edits, GENERATE policies and manual/auto mode changes share normal scheduling, exact common-tone holds and synth attacks/releases. No generator-imposed pitch interpolation remains. Patch and role replacements retain shared effect buffers; JIANT's existing millisecond engine handoff remains for click protection. Ordinary parameter edits remain direct. An old engine's full dry release is not retained during track replacement; shared delay/reverb tails continue.

All contextual button/encoder guidance is centralized in three fixed footer lines with matching font, color, casing and spacing. Navigation and action hints remain visible during notices and MASTER pickup. Upper content holds data/diagrams, including proposal/restore data without duplicate button guidance. PATTERN correctly distinguishes capture, overdub and stop from confirmation.

All prior features remain: four assignable layers, seven roles, detailed sound groups, sustained Pad/Drone defaults, shared HOME pool, generated overdub, manual-pattern retention/AUTO return, restored loaded track, real LFO phase and FX routing diagrams, diverse seeded harmony/Pad motifs, eight v3 A/B save slots. The payload remains 1400 bytes. Sources/licenses/toolchain remain pinned; no Shrub code was used.

See NATURAL_TRANSITIONS_VALIDATION_REPORT.md, CONTROLS.md and ../../release/ambplus-0.4-natural-transitions/ for evidence and emulator acceptance. Previous releases are historical, not the current transition specification. External capture and fullscreen VISUAL remain deferred. Strict 240 MHz, baseline CPU/editor tests and unsupported UC25 persistence remain independent open gates pending current measurements. Human listening acceptance remains pending.

FLASH STATUS: NOT AUTHORISED. No physical device work is requested or performed.
