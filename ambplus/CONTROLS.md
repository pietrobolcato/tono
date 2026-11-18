# AMB+ control card

The playable instrument is `build/felucca.fwsc` in Joakim Eriksson's FM1 hardware emulator. The handoff HTML remains a visual specification.

- HOME SELECT: MASTER → L1 → L2 → L3 → L4. Four sound layers; MASTER adds no voice.
- MASTER pot: the last HOME target's level, on every page. After changing target, cross its stored level to pick it up. Existing JIANT gain ramps smooth the audio.
- HOME KNOBS 1–4: displayed performance macros. Layer macros add bounded offsets to deep baselines; the neutral position is 50%. MASTER uses the same shared effects as GLO.
- EDIT: selected layer's MODE; MASTER opens GLO MASTER FX. ALGORITHM previews a role; REC commits. SAMPLE is the seventh role.
- SCL: this is the physical button used as **SEL** in the handoff. It cycles MODE / SOUND / ENVELOPE / LFO / PATTERN / FX, or EVOLVE / HARMONY / GENERATE / MASTER FX / EQ.
- SOUND SELECT: patch audition with retained shared effect tails. EDIT changes PATCH/DETAIL. SAMPLE has SOURCE/PLAYBACK/CHARACTER; SELECT chooses one of two read-only PCM examples. Capture is unavailable.
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

## Performance feedback update (v0.2)

- **Pad / Drone HOME knob 3 = SUSTAIN.** New factory patches have higher sustain and 84% base activity. The macro changes hold length, release, sustain and generated activity. Pad/Drone start a gentle bed at PLAY and after sound changes, rather than waiting for a sparse first event. Muted/solo-excluded layers stop generating new notes.
- **HOME keyboard = shared note pool.** Bright keys belong to the current chord; softer lit keys belong to the available pool. Tap keys to add/remove pitch classes for the whole generator, as in AMB-1. The tonic stays enabled and at least three pitch classes must remain. Octave duplicates toggle the same class. To audition literal notes, enter EDIT; recording still accepts off-scale notes. Choosing a stock scale in GLO/HARMONY replaces the custom pool; changing key transposes it.
- **SOUND DETAIL:** EDIT switches PATCH/DETAIL. In DETAIL, SELECT cycles ENGINE / FILTER / PLAY groups. Four rows show actual parameter values and normalized bars. Select patches in PATCH; SELECT no longer changes a patch while deep-editing.
- **Record over generation:** on a pitched PATTERN, the first REC captures the latest generated 16-slot melody (or previews a deterministic current phrase if nothing has played), then starts overdubbing. It does not open an empty pattern. During PLAY, notes replace the current playback slot; stopped notes fill the selected slot and advance. Other slots stay intact. Recorded Pad/Drone steps keep their long SUSTAIN gates too. The current manual format stores one note per step, so a generated chord contributes its first melody note. Manual locks remain literal across pool and generation changes.
- **Track restore:** select a track, open EDIT/MODE, then press EDIT again. REC confirms RESTORE THIS TRACK; HOME cancels. This restores sound, role, macros, level, pattern and track controls to the last loaded factory/user piece. Other layers, shared pool, global harmony/FX and solo selection stay intact. It does not erase a saved slot.
- **Chord changes:** sustained Pad/Drone voices hold exact common tones; different notes use normal synth attacks/releases without generator-imposed pitch slides. Existing effect tails remain. Discrete harmony still commits on the bar; this does not delay the musical boundary.
- **LFO:** the dot follows the real DSP oscillator phase across one cycle, using the chosen shape. It runs independently of sequencer steps. S&H shows the actual current held value. Fast oscillations are sampled at the display refresh rate.
- **FX graphics:** delay/reverb show the dry path and a labelled shared-bus send with its actual effective send percentage. DRIVE shows a response model, CHORUS its amount, SPATIAL the effective pan. These are control diagrams, not audio meters or frequency analyzers.
- **LEDs:** available button actions are steadily lit, pending REC confirmations breathe, recording REC stays solid. Scale/pool notes have a visible intermediate brightness; current chord/root/pressed keys are stronger.
- **Emulator combinations:** hold computer **7** (HOME), then press **Z** (OCT−) for mute or **X** (OCT+) for solo. Hold 7 and use left/right for cross-layer editing in EDIT. You can also hold 7 while clicking a panel button. The mouse alone holds one control at a time; no separate HTML latch is involved.

Saves are now version 3, still 1400 bytes in the same eight existing A/B objects. Version 1/2 records load with their saved musical settings and stock scale pool; old v0.1 firmware cannot read version 3 records. Keep old projects/backups for rollback. New factory sustain defaults apply when loading a factory piece/patch; existing user envelopes are preserved.

## Current musical behavior (v0.4)

AUTO harmony no longer uses the erroneous 0↔3 progression table. The seeded weighted degree bag avoids immediate repeats and A–B–A, traversing seven different degrees in the first seven-note-scale cycle. Harmonic notes still come from your fixed key/scale or edited shared pool using the licensed AMB-1 helper. GENERATE governs timing/phrasing/interplay, not a new key or scale; its eight PHRASES now also choose Pad motifs/voicing pairs. Six pairs and slow register motion replace the fixed pair; surviving Pad voices are not retriggered just because the upper tone was stolen.

On EDIT/PATTERN, **EDIT or SEQ again toggles MANUAL/AUTO**. Manual notes and drum lanes stay intact while AUTO plays; toggle back to reuse them. REC from AUTO captures a new generated snapshot for overdub, so use the mode toggle to return to existing edits. Both mode directions leave existing notes to their natural releases. Runtime progression-bag/transport state is not a saved audio snapshot; v3 musical settings/manual payloads remain compatible with v0.2.

## One transition rule

AUTO and explicit user changes share normal note/envelope behavior. HOME ALGORITHM/REC, GLO HARMONY key/scale, shared pool edits, GENERATE rhythm/phrase/interplay, and pattern changes never start a separate source-volume fade or forced pitch slide. Common tones carry; other scheduled notes use their synth's attack/release. GENERATE still commits on four bars; a proposed chord still commits on the next bar.

SOUND SELECT patch changes and MODE ALGORITHM/REC layer changes retain shared delay/reverb state and restart the new patch with its own envelope. The underlying JIANT track handoff retains its brief click protection (approximately 2.9 ms for the same engine, 11.6 ms for different engines); it cannot render two full independent engine states on the same track. It does not preserve an old engine's full dry release across a replacement. No 1.2/2.4-second AMB+ transition envelope remains. Ordinary sound parameters and HOME macros remain responsive; intentionally configured GLIDE/pitch modulation still applies.

All interaction guidance is in the footer: navigation, context actions, then notices/pickup/status. These use the same aligned uppercase bitmap text and color. Page content holds musical data and diagrams. Recording says REC CAPTURE/OVERDUB/STOP; it is not described as a generic confirmation. Pickup does not replace the context action.
