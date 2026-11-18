# AMB+ implementation status

Latest: v0.5 bounded old-engine release, silent selected-step entry, correctly divided playhead, and per-layer EQ; branch ambplus/release-handoff-layer-eq.

The owner confirmed that retained FX alone did not solve abrupt patch/type changes. Replacements now retain the old engine and effective parameters until its voices finish, accelerating only slower releases to the configurable GLO/HARMONY TAIL CAP (default 3000 ms, range 250–6000 ms). Requests coalesce without restarting a tail. The new engine starts only after the release, using its own envelope, with no AMB+ incoming fade. MIDI/keyboard/generator notes do not retrigger a releasing engine. Release tails are protected from ordinary voice stealing; realtime overload protection remains. No parallel fifth layer/duplicate synth renderer or new audio buffer was added.

PATTERN now distinguishes actual divided/swing-dispatched PLAY position from the selected EDIT cursor. It updates at 20 Hz; rests still have a step and effect/envelope tails are not note triggers. REC enters notes silently into the selected slot, instead of auditioning them or overwriting the live playhead. The sequencer plays them on its next visit. All earlier generated capture/retained manual/AUTO features remain.

A seventh EDIT page provides independent LOW/MID/HIGH/OUTPUT EQ per layer, before dry mix and shared sends, including drum image/send paths. State is separate per track and side bus; unity is bit-exact bypass, controls smooth, boosted output has a bounded knee. The master EQ remains separate. This is a broad-band control diagram, not a measured spectrum.

Append-only save v4 is 1420 bytes, within existing A/B sectors/capacity. Legacy 1400-byte v1/v2/v3 records migrate with neutral EQ and a 3000 ms cap. Old software cannot read v4; preserve old pieces for rollback. No flash driver, layout, loader or bootloader changed.

v0.4's source fader removal and normal harmony progression are retained. UI guidance remains in the three-line footer; pending release state is explicit. All earlier features, licenses and source/toolchain pins remain. See RELEASE_HANDOFF_VALIDATION_REPORT.md, CONTROLS.md and ../../release/ambplus-0.5-release-handoff/ for exact evidence and the human emulator card. Human listening acceptance and hardware release gates remain pending. FLASH STATUS: NOT AUTHORISED.
