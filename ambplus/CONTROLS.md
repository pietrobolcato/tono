# AMB+ control card

The playable instrument is `build/felucca.fwsc` in Joakim Eriksson's FM1 hardware emulator. The handoff HTML remains a visual specification.

- HOME SELECT: MASTER → L1 → L2 → L3 → L4. Four sound layers; MASTER adds no voice.
- MASTER pot: the last HOME target's level, on every page. After changing target, cross its stored level to pick it up. Existing JIANT gain ramps smooth the audio.
- HOME KNOBS 1–4: displayed performance macros. Layer macros add bounded offsets to deep baselines; the neutral position is 50%. MASTER uses the same shared effects as GLO.
- EDIT: selected layer's MODE; MASTER opens GLO MASTER FX. ALGORITHM previews a role; REC commits. SAMPLE is the seventh role.
- SCL: this is the physical button used as **SEL** in the handoff. It cycles MODE / SOUND / ENVELOPE / LFO / PATTERN / FX, or EVOLVE / HARMONY / GENERATE / MASTER FX / EQ.
- SOUND SELECT: immediate patch audition. EDIT changes PATCH/DETAIL. SAMPLE has SOURCE/PLAYBACK/CHARACTER; SELECT chooses one of two read-only PCM examples. Capture is unavailable.
- LFO: shape, rate, depth, destination (filter/pitch/pan/volume). One control-rate LFO per layer, with gentle defaults.
- PATTERN: EDIT toggles generated participation versus locked manual playback. REC starts/stops step entry and locks the local pattern. For pitched parts, SELECT chooses the step and keys enter literal notes, including off-scale notes. For drums, SELECT chooses KICK/SNARE/HATS/PERC; the 16 white keys toggle steps. Pitched knobs set mode, speed, density and harmony follow; drum knobs set density, swing, fill and mutation.
- HOME or GLO EVOLVE ALGORITHM: preview a compatible scale degree/mood. REC commits now while stopped or at the next bar while playing. SELECT on EVOLVE chooses OFF/1/2/4/8/16/32/64 auto bars.
- GLO GENERATE: KNOBS 1–3 choose 8 rhythms, 8 phrases, 6 interplay policies, with a 90 ms final settle. KNOB 4 changes variation. During PLAY, styles queue together to the four-bar grid; returning to the active choice cancels it. SELECT, ALGORITHM and REC are inert here.
- PRESETS: preview six read-only factory scenes followed by eight user slots. REC installs the selected piece, at a bar boundary when playing. It does not write flash.
- SAVE: SELECT chooses a user slot. Stop playback before saving. REC writes an empty slot; an occupied slot needs REC twice. SELECT or HOME cancels an armed overwrite. No date is fabricated. A failed write retains the previous CRC-verified copy.
- HOME held + SELECT in EDIT: change the edited layer on the same page; the volume target stays at the last HOME selection. Returning HOME restores that target for keyboard routing.
- HOME held + OCT−: mute selected HOME layer. HOME held + OCT+: solo/unsolo it.
- PLAY tap: play/stop. Hold PLAY for 800 ms: emergency output fade, independent of target. A later PLAY tap reopens the output with a ramp.
- ENV / LFO / FX / SEQ / ARP are direct layer editor shortcuts when a layer is selected; OCT± otherwise changes the keyboard octave.

The verified JIANT matrix HAL and fm1-emu model KNOBS 1–4 as quadrature encoders. Only MASTER is an absolute ADC pot. No analog wiring was invented to satisfy the handoff's contrary assumption. The generic absolute 8/8/6 selector decoder is separately tested but is not falsely connected to the FM1's encoder inputs.

FLASH STATUS: NOT AUTHORISED. This control card is for the emulator until the owner completes SAFE_FLASH_GATES.md and approves one exact artifact manually.
