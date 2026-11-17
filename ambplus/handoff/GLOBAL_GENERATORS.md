# AMB+ — Unified GLO / GENERATE specification (v1.5)

**Goal:** Turn three separate GLO pages into **one**, without reducing the 8 × 8 × 6 combinations available. The user can improvise generative ambient music by moving just four knobs. AMB-1's rhythmic/phrase aesthetic remains the baseline; new algorithm options are AMB+ implementation proposals rather than claims about current JIANT or AMB firmware.

## GLO navigation: six pages
`EVOLVE → HARMONY → GENERATE → MASTER FX → EQ → VISUAL (later)`

### GLO / GENERATE — control mapping

| Physical control | Exact action | Confirmation |
|---|---|---|
| KNOB 1 | RHYTHM (8 styles) | Direct when stopped; quantize to next 4-bar grid while playing |
| KNOB 2 | PHRASES (8 styles) | Same |
| KNOB 3 | INTERPLAY (6 styles) | Same |
| KNOB 4 | VARIATION (0–100, default 40) | Smooth bounded adjustment, no REC |
| SELECT rotary | **Inactive** on this page | No redundant selector |
| SEL button | Next GLO page | Standard page indicator |
| ALGORITHM | Reserved for chord preview on HOME/GLO EVOLVE | Not used on GENERATE |
| REC | Reserved for chord commit/record/save where relevant | Not used on GENERATE |

**Default style:** `RHYTHM = AMB DRIFT (01/08)`, `PHRASES = AMB BREATHE (01/08)`, `INTERPLAY = BLEND (01/06)`, `VARIATION = 40`. The three styles select policies for generated music only. Knob 4 controls how strongly generated events deviate from the selected policies; it does not control gain, density of manual sequences or the global EVOLVE harmony/timbre/rhythm/FX routing amounts.

### Style choices

- RHYTHM: AMB DRIFT · EUCLIDEAN · ODD LOOPS · POLYRHYTHM · SPARSE · TIDAL · BROKEN · SLOW CLOCK
- PHRASES: AMB BREATHE · LONG ARC · WANDER · BLOOM · CALL ANSWER · DRIFT SHIFT · MEMORY · MINIMAL
- INTERPLAY: BLEND · STAGGER · ALTERNATE · COMPLEMENT · SPARSE TALK · TIDE TOGETHER

The exact musical rule of each style is described in `ROADMAP.md`. These are lightweight event-level scheduling policies sharing one global event scheduler, not 22 expensive DSP instances.

## Three synchronized visuals on one 240 × 240 page

1. **RHYTHM** mini-grid: pulse placement, gaps and asymmetric timing pattern. Reflects choice of KNOB 1.
2. **PHRASES** dynamic curved contour: how activity grows/rests over bars. Reflects choice of KNOB 2.
3. **INTERPLAY** interleaved micro-lanes: how generated layers answer, stagger or overlap. Reflects choice of KNOB 3. May show fewer than four active lanes when tracks are disabled.

A standard four-knob value strip occupies the bottom. A chosen algorithm is rendered by name in its respective row; queued replacements are distinguished by a small `Q` and amber text. **These are abstract graphical explanations, not measured sound waveforms or evidence of implemented audio generation.** Knob 4 affects visual liveliness but not the saved instruments' settings.

Native layout budget: header 0–47 px, three rows 48–146, helper 147–174, four control values 175–237. Use native readable bitmaps, no overlapping labels. Do not flash merely because browser graphics look right.

## Absolute-pot implementation constraint

The FM1 has four absolute parameter potentiometers. The HTML click simulator treats them as step increments; **this is not their real signal**. Implement discrete-choice controls as a robust finite-state decoder of ADC readings with:

- **8/8/6 stable bins** mapping to the style indices. Document ADC calibration/noise.
- **Pickup** on entry and after restoring a saved piece; do not force style to current physical position until pot reaches the existing style.
- **Hysteresis** across bin boundaries to prevent style toggling with tiny hand movement or ADC noise.
- **Final-settle** for rapid continuous movement so a single swipe queues the final choice rather than many abrupt transitions. Do not implement blocking sleeps in the audio ISR.
- **No memory changes to layer settings**, and no implicit global audio-level jump. Confirm actual pickup usability with the human on the real device before changing calibration.

## Preservation contract / tests

- Generative output may react to the shared harmony, tempo, global chosen policy, variation, and read-only per-layer opt-in, but it must not mutate any saved patch, envelope, LFO, source sample, layer volume, FX send or manually entered step pattern.
- Recordings/manual locks remain literal. If a layer needs to follow global rhythmic structure it must opt in via EDIT/PATTERN; never assume all four layers follow globally.
- While PLAYing, all style changes land at the **next shared four-bar grid**, without resetting tempo, chord, sustained notes or reverb tails. If the user cancels a queued choice, discard it. Selection order doesn't matter: the three families are independent.
- Save/recall new `genVariation` plus three style IDs. Migrate older saves with `genVariation=40` and missing IDs defaulting to AMB DRIFT / AMB BREATHE / BLEND. Existing legacy style-specific parameters may remain in project metadata for compatibility, but are not exposed in the compact UI.
- Tests: each family all choices, no-op SELECT, 4-bar queue in PLAY, knob 4 monotonic and safe, checkpoint/power-cycle persistence, 20-minute ambient listening, native 240px render fit, best/worst four-layer CPU and stack metrics.
