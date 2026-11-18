# Correction: user changes versus normal generated playback

Base: 1168b6674e0386e3f448f1c4f6eb5c5df489a65a (v0.3 code c810c570).
The v0.3 fade bridge was wrongly applied to AUTO harmony, creating repeated dips. Its generator also forced 1.2 s pitch sweeps while changing Pad material.

Fix: explicit change-origin argument; AUTO and piece initialization never request the transition fader. Only explicit user chord/key/pool, queued GENERATE and manual/auto/discrete mode changes use it. Tempo/glide-continuous edits and no-op mode changes do not. Exact common notes carry; changed pitches take the normal engine note-on/release path. Generator code no longer writes pitch targets/glide slopes. Explicit patch GLIDE remains supported. Harmony variety and preserved manual patterns stay implemented.

Regression tests reproduce the pre-fix AUTO fader error, exercise ten actual automatic chord changes with no source fade/forced Pad portamento, preserve manual tails/fades and check continuous-control editing. No hardware/loader/sector changes. FLASH STATUS: NOT AUTHORISED.
