---
name: tono-patch-voicing
description: Design and audition distinct factory preset variations for an existing Tono patch. Use when a patch's presets sound too similar or a live player needs clearer musical choices.
---

## Choose an audible difference

Play Original first. Pick a musical job for each candidate and identify where the player will hear the difference, such as a softer attack under a lead, beating during a held chord, or a brighter tail between phrases. Choose controls that the patch's actual engine implements. The number of useful ideas can vary by patch.

Cloud Bloom used a sine bed for Still Water, two slightly detuned triangles for Twin Reeds, and a slowly swelling upper interval for Glass Tide. Changing the oscillator mix made the differences audible through the sustain. The first Glass Tide sounded too distorted to the owner. Reducing the upper oscillator and auditioning it against the same chord gave them a choice without changing the engine.

## Audition before adding to the picker

1. Render Original and candidates through the actual playback path with the same notes, register, velocity, seed and performance controls. Hold pads long enough to hear an entire modulation cycle and release. For drums, include both isolated hits and a groove.
2. Compare solos at matched loudness, then swap just the patch preset in the same passage of a song. Keep the mix gain fixed. Listen for whether a useful difference survives the mix and whether a brighter attack or sustained overtone becomes tiring.
3. Give Gemini neutral labels and ask for timestamped audible differences, the mix role of each candidate, and any pair it struggles to distinguish. Use the repository's audio judge and save its report with the WAVs. Record pending reviews and the owner's listening decision. Give the owner full paths to the comparison.
4. Add the approved choices to the patch's own picker. Keep Original as the factory reference. Preserve realized sound parameters in previously saved songs. Describe what a player can hear in a short live cue. Check switching in either direction, reapplying the same choice and returning to Original. Re-render through the actual picker so audition-only processing cannot silently disappear from the shipped preset.
