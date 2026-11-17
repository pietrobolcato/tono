> **v1.7 full-build scope note:** `START_HERE.md` and `FULL_BUILD_PROMPT.md` are authoritative for the initial complete firmware build. The sequential roadmap below describes implementation milestones; they are not permission to stop after planning or to require owner approval between software-only stages. **Phase 6 external microphone capture and Phase 8 VISUAL mode are postponed.** An emulated/bundled SAMPLE playback engine is in scope where memory/flash budgets permit. Hardware safety gates remain mandatory and unchanged.

# AMB+ — Ambient-first FM1 product and development roadmap

> **v1.6 development-environment decision:** Use **FM1 hardware-level emulator ONLY for interactive simulation** (browser or native `fm1-live`) by opening the exact firmware `.fwsc` produced by `./build.sh`. **Do not maintain a separate JIANT Emscripten web application as normal workflow.** Keep `tests/run_tests.sh` host tests, native `FM1_STRICT=1` stress passes, exact-package checks, and human device-safety gates. This changes development tooling only, not the UI, synthesis, generative algorithms, or physical instrument contract. See `FM1_EMULATOR_FIRST.md`.


**Planning specification v1.5 — 17 November 2026.** For a Codex-led implementation, reviewed by the human owner at each hardware gate. This is a specification, **not firmware**, and it does not claim hardware validation.

## North star

AMB+ is a **four-layer, autonomous ambient instrument** on the M-VAVE FM1. It starts sounding beautiful immediately. The musician should be able to choose a richly designed sound, shape one of four layers **or the unified MASTER performance target** with a few forgiving macro controls, move toward a different harmonic feeling with ALGORITHM and REC, and leave a piece evolving for twenty minutes without it getting harsh, repetitive or unstable.

The depth exists behind EDIT, not in the first 30 seconds. We prefer excellent curated sound presets + rewarding continuous controls over exposing every synthesis operator in the primary workflow.

### Source responsibilities (verify and pin exact revisions before coding)

- **JIANT** — primary firmware architecture, synthesis engines, patch storage, voice allocation, envelopes/filters/LFO, CPU-aware effect infrastructure, audio USB input candidate. Source: https://github.com/juanjiant-bit/JIANT (upstream). Forks exist; choose and document one exact source revision, and verify its features and release state before modifying it.
- **AMB-1** — adaptation/reference for shared evolving chord logic, independent unequal-length note loops, Euclidean rhythms, breathing drones, tape echo/long loops and ambient phrasing: https://github.com/charlesvestal/fm1-amb .
- **April OS** — proof of product interaction for recording short mic samples via an external computer/phone browser bridge (not a direct USB mic on the FM1); investigate code availability and license before porting any functionality: https://github.com/beashoko/april-os .
- **ZEN-1** — *much later* reference for audio-linked landscape/abstract graphics and low-cost renderer/tests: https://github.com/aceyprime/zen-1 . The user specifically likes https://joakimeriksson.github.io/fm1-emu/?fw=zen&mhz=100 .
- **FM1 developers' wiki / emulator** — hardware limits, boot and recovery details, firmware tests: https://sbegueria.github.io/fm1-dev-wiki/Recovery.html and https://joakimeriksson.github.io/fm1-emu/ .

**Scope lock:** JIANT is the sole primary synthesis codebase for this plan. Shrub is **excluded**; do not depend on, copy from or import its proprietary code, wavetable files or factory presets. Ambient sound targets in this plan are original design briefs to build from the permitted source.

**Licensing gate:** Record the chosen file/commit origins, licenses and notices for JIANT (GPL), April (Felucca-derived GPL), AMB-1 (current main is MPL-2.0 as of 17 Nov 2026, older releases GPL-3.0-only; pin specific commits, check MPL/GPL compatibility, preserve notices) and ZEN-1 (GPL). Do not assume that public demos provide reusable firmware source. Preserve notices and comply with obligations if distributing a derivative build.

## Locked control and UX decisions

| Context | Control | Expected behaviour |
| --- | --- | --- |
| HOME | SELECT rotary | Cycle through **five performance targets: MASTER, L1, L2, L3, L4**. MASTER is a global control target, **not a fifth synthesis layer**; no redundant pop-up |
| HOME, target = MASTER | physical MASTER potentiometer | Control **overall output volume** with soft takeover and smoothing |
| HOME, target = L1–L4 | physical MASTER potentiometer | Control **only the selected layer volume** with soft takeover and smoothing; overall gain remains unchanged |
| HOME, any target | KNOBS 1–4 | Set four **friendly performance macros** for the selected target immediately; no REC commit |
| HOME, target = MASTER | EDIT button | Open **GLO > MASTER FX** instead of a layer editor |
| HOME, target = L1–L4 | EDIT button | Open the selected layer's **EDIT > MODE** page (1/6) |
| EDIT or GLO | SEL button | Advance pages; indicator shows position; no silent no-op LED |
| EDIT > MODE (page 1) | ALGORITHM | **Preview** changing the selected layer's role (PAD/DRONE/BELLS/BASS/DRUMS/TEXTURE/SAMPLE) |
| EDIT > MODE (page 1) | REC | **Commit** the visibly proposed role; fade old voice safely |
| EDIT > SOUND (page 2), six synth roles / PATCH | SELECT | Immediately audition/change one of eight starting synth patches; **no REC** |
| EDIT > SOUND (page 2), SAMPLE / SOURCE | SELECT | Choose a captured-audio source slot (not a synth patch) |
| EDIT > SOUND (page 2), SAMPLE / SOURCE | REC | Start/stop short audio capture via verified external microphone bridge; occupied slot requires confirmation before replacement |
| EDIT > SOUND (page 2) | EDIT | Cycle **PATCH ↔ DETAIL** for synth roles, or **SOURCE → PLAYBACK → CHARACTER** for SAMPLE |
| SOUND/ENV/LFO/FX and HOME | KNOBS 1–4 | Continuous musical changes; no commit. HOME macros must mirror the underlying EDIT/GLO DSP state (not a second FX chain) |
| EDIT > PATTERN | SELECT | Navigate drum lane or melodic step, depending on role |
| EDIT > PATTERN | REC | Start/stop note or step recording; in-scale key LEDs for pitched recording |
| GLO > EVOLVE | ALGORITHM | Browse possible next harmonic destinations, with short mood descriptors |
| GLO > EVOLVE | REC | Commit current visible proposal; quantize to next bar if playing |
| GLO > EVOLVE | SELECT | Set auto interval OFF/1/2/4/8/16/32/64 bars |
| GLO > GENERATE | SELECT rotary | **Inactive**: no redundant selector; the three algorithm styles are chosen by the physical knobs |
| GLO > GENERATE | KNOBS 1–3 | Select RHYTHM (8), PHRASES (8), INTERPLAY (6) directly; stopped = immediate, playing = last settled choice queued to the next four-bar grid; absolute-pot pickup/hysteresis required |
| GLO > GENERATE | KNOB 4 | Adjust one global VARIATION amount (0–100) without altering any stored layer edits |
| GLO > GENERATE | ALGORITHM / REC | **Inactive on this page**; ALGORITHM previews harmonies on HOME/GLO EVOLVE and REC commits only a pending contextual proposal |
| PRESETS rotary (normal performance) | PRESETS | Preview saved piece; REC commits if visibly pending |
| SAVE page | SELECT | Choose destination slot; changing cancels armed overwrite |
| SAVE page | REC | Save to empty slot; occupied slot needs two REC presses to overwrite |
| Any page | HOME | Return to five-target HOME (MASTER + four layers); short press cancels pending confirmation where applicable |
| GLO/SAVE/EDIT | physical MASTER potentiometer | **Retain the last HOME volume target** until HOME target changes; do not silently reassign volume when navigating pages |
| Any page | hold PLAY/STOP | Proposed emergency output ramp-to-silence / PANIC; verify gesture collision and hard-recovery options on actual firmware |
| Later: VISUAL mode | hold HOME (without another key) | Toggle full-screen, number-free music-synced view; no changes to music or saved state; timing distinct from HOME+SELECT |

**Button lighting rule:** Illuminate only if pressing it does something now: dim for actionable; steady for an active mode; slow pulse for pending commit. No bright LFO/FX/SEQ lights just because a corresponding page is visible. Knobs cannot physically illuminate; show brief parameter feedback on-screen if helpful. The on-screen helper strip is consistently placed above the four parameter values; never duplicate an instruction in the header. Display: **240 × 240 native pixels**, test actual embedded fonts and draw cost.

### Performance-first synthesis UX — five HOME targets, four audio layers

**HOME is the musical performance surface; EDIT is detailed layer sound design; GLO is detailed whole-piece design.** The HOME screen displays a full-width **MASTER** tile above four layer tiles. SELECT navigates five items `MASTER → L1 → L2 → L3 → L4 → MASTER`; selecting MASTER creates no additional voice or DSP engine. An accessible 240×240 renderer must fit the full MODE names, patch names, target highlight, current chord and the four context-dependent values without collisions.

**Physical MASTER pot (a fixed-position, software-readable potentiometer):**
- With MASTER selected, it adjusts actual summed/output volume; with L1–L4 selected, only that layer's level. Switching to a new target must not change its saved level until **soft takeover/pickup** detects the pot crossing that level. Show a small `PICKUP` cue, and after pickup smooth all gain changes (no jumps).
- **Changing page must not change the target of the pot.** The last HOME-selected target persists in EDIT, GLO and SAVE; the user explicitly chooses another target on HOME. This prevents an accidental volume jump or surprise reassignment when opening an editor.
- The physical knob cannot provide an LED indication or motorized pickup. Input is a continuous absolute ADC value, unlike SELECT's relative encoder. Provide a tested emergency all-output fade/panic gesture independent of the current volume target (suggested **hold PLAY/STOP**). Do not assume a long-press is safe until tested for gesture conflicts and press timing on hardware.

**HOME four musical knobs, immediate adjustments:**

| Selected target | Knob 1 | Knob 2 | Knob 3 | Knob 4 |
|---|---|---|---|---|
| **MASTER** | **SPACE** — shared reverb amount | **ECHO** — shared delay/tape echo amount | **MOTION** — gentle shared modulation/width | **WARMTH** — overall tonal tilt / safe saturation |
| **PAD** | WARMTH | MOTION | BLOOM | SPACE |
| **DRONE** | WARMTH | DRIFT | DEPTH | SPACE |
| **BELLS** | SOFTNESS | MOTION | SHIMMER | SPACE |
| **BASS** | ROUNDNESS | PULSE | GLIDE | SPACE |
| **DRUMS** | SOFTNESS | DENSITY | DUST | SPACE |
| **TEXTURE** | BLUR | MOTION | GRAIN | SPACE |
| **SAMPLE** | PITCH | BLUR | MOTION | SPACE |

The PAD/etc rows follow each layer's **assigned musical role**, not slot number. Curated per-patch macro ranges are musical, gain-compensated and slow/smoothed; no clicks, harsh resonances or accidentally silent extremes. The four MASTER macros are high-level controllers of the **same** shared DSP buses / EQ exposed in GLO, *not duplicate effects*. A nominally dry layer stays comparatively dry when MASTER SPACE increases: per-layer sends still matter. No unnecessary value duplication in helpers; control labels stay in one fixed strip.

**Deep editing:** `MODE → SOUND → ENVELOPE → LFO → PATTERN → FX` is the **final six-page EDIT structure**, with SEL cycling pages. **MIX is removed**; HOME already owns volume, PATTERN owns harmonic FOLLOW, and FX owns spatial placement and sends. No duplicate MIX screen. The first MODE page uses **ALGORITHM preview + REC commit** when assigning any of the **seven layer types**. Sound editing is **type-dependent**:

- **PAD / DRONE / BELLS / BASS / DRUMS / TEXTURE:** on SOUND (page 2), **SELECT** immediately changes among eight synthesized starting patches; **EDIT** alternates `PATCH ↔ DETAIL`. REC is inactive in these sound banks. PATCH and DETAIL edit different underlying parameters of the *same engine*, and the first controls must be engine-appropriate (e.g. FM ratio is not a generic analog parameter).
- **SAMPLE:** on SOUND (page 2), **EDIT** cycles `SOURCE → PLAYBACK → CHARACTER`. `SOURCE`: SELECT chooses captured source slots; REC captures through the supported external audio bridge; four knobs adjust start/end trim, gain and fade. `PLAYBACK`: four knobs set Pad/Drone/Texture/One-shot playback style, pitch, looping and crossfade. `CHARACTER`: four knobs shape blur, filtering, motion and space. **Do not insert sampling as a third SOUND bank for the six synth roles.**
- **All roles:** ENC/KNOBS 1–4 adjust parameters immediately, SEL moves to the next of **six** *top-level* EDIT pages, and HOME macros shape the same underlying state non-destructively. ENVELOPE/LFO/PATTERN/FX expose precision controls; GLO contains EVOLVE/HARMONY/**GENERATE**/MASTER FX/EQ and later VISUAL preferences. SAMPLE reuses only musical processing appropriate for captured audio; do not claim every synthesis-specific parameter applies to it.


**Selection and routing specifics:** HOME `EDIT` with a layer selected opens EDIT/MODE; HOME `EDIT` with MASTER selected opens GLO/MASTER FX. GLO itself still opens global pages. `SELECT` in HOME always changes the five targets, while in editors it browses the specific page item (patch, LFO slot, drum lane, effect module, sample slot, etc). `SEL` changes **pages only inside EDIT/GLO**, never HOME targets. ALGORITHM on HOME/GLO EVOLVE proposes harmony; REC commits only if a visible proposal exists; preview-only button LEDs pulse; continuous controls apply instantly.

**Factory/User composition navigation:** PRESETS browses whole-piece proposals. REC commits only a valid visible piece proposal. SAVE opens a dedicated destination page; SELECT selects the writable slot; REC saves empty or requires two presses on occupied, and SELECT/HOME cancels a pending overwrite. Date is shown only if valid time metadata exists—no invented hardware clock.

### Real-device acceptance tests specific to the five-target HOME

1. At MASTER, turn physical MASTER to set global level. Move to PAD: the PAD's saved level and overall output **must not jump**. Slowly cross the PAD's saved value; only then should layer level follow the pot, with short gain smoothing. Return to MASTER, confirm no global jump until pickup.
2. With MASTER selected, sweep each of SPACE/ECHO/MOTION/WARMTH through its curated range; listen for musical changes, preserved effect tails and no output clipping. GLO/MASTER FX must reflect those same underlying values, not a second copy.
3. With each layer selected, sweep its four contextual macros, then EDIT that layer and verify the DSP state is shared; verify playing another layer is unaffected by changes to a single-layer macro, except intended shared FX interactions.
4. While in EDIT/GLO/SAVE, move the physical MASTER pot: its *last HOME target* must be stable, with the correct pickup cue. Test return to HOME and target cycling using SELECT only.
5. Propose chord with ALGORITHM, commit with REC, observe that **no gain jumps** occur when a chord evolves. Try performance panic / all-output fade without having to select MASTER; verify the short PLAY action still works.

## Preset-first musical scope

Four configurable layer slots, **seven assignable musical types**: six synthesized roles (PAD, DRONE, BELLS, BASS, DRUMS, TEXTURE) **plus SAMPLE**. **Eight hand-voiced sounds for each of the six synthesized roles** remains the sound-library target (48 total), plus read-only example pieces. SAMPLE is a different class: it uses **captured-source slots**, not eight synth-patch presets. Every synth sound needs a recipe: source engine, voice/CPU cost, envelope, macro mappings, default FX sends, modulation and suggested role. SAMPLE uses short captured audio transformed into pad/drone/texture/one-shot behaviour while still occupying only one of the four audio layers.

Suggested ambient names/themes to commission or voice:

| Role | Eight starting patches (targets; not claims of existing JIANT sounds) |
| --- | --- |
| PAD | Cloud Bloom · Soft Choir · Velvet Saw · Glass Haze · Warm Strings · Low Sun · Slow Orbit · Distant Light |
| DRONE | Deep Current · Hollow Earth · Quiet Organ · Tape Hum · Underwater · Harmonic Bed · Dark Air · Warm Voltage |
| BELLS | Felt Glass · Distant Chime · Wooden Star · Rain Glass · Soft FM · Resonant Dust · Floating Metal · Night Sparks |
| BASS | Round Sub · Soft Pulse · Breathing Low · Warm Fifth · Faded Mono · Slow Glide · Low Echo · Root Bed |
| DRUMS | Dust Steps · Soft Pulse · Brush Rain · Broken Clock · Slow Ritual · Hollow Perc · Hidden Beat · Sparse Ticks |
| TEXTURE | Ocean Air · Tape Mist · Glass Wind · Paper Grain · Distant Voices · Rainfield · Organic Noise · Midnight Static |

**SAMPLE has no synthetic preset bank.** Its starting experience is a guided short recording (or a clearly labeled, licensed demo clip only if the storage budget allows it). Choosing SAMPLE via MODE must not silently overwrite other layers or replace the global arrangement. If recording is unavailable, show a clear bridge/input-unavailable state rather than pretending a clip was captured.

**Curated factory compositions:** aim for at least six differentiated musical scenes (e.g. Morning Air, Rain Study, Dark Water, Low Sun, Glass Room, Night Garden) with optional more only if memory allows. Distinguish **factory read-only content** from **writable user saves**; user-facing goal is eight save slots, but *current JIANT branches may reserve two of those slots for samples and expose only six songs*. Measure flash and sample-bank allocations before promising eight writable slots. Present the trade-off to the owner if necessary.

## Locked engine-aware synthesis UX and FX routing (v1.3)

**Engine selection is implicit in the curated patch.** On each of the six synthesized roles, EDIT > SOUND > PATCH offers eight hand-designed patches; **SELECT changes the patch immediately**, and the patch loads its designated JIANT-compatible synthesis engine. No confirmation is required for a patch change. Changing the *role* in EDIT > MODE is still a major change that requires **ALGORITHM preview → REC commit**. A patch switch should release/crossfade sounding voices, not hard-reset the shared reverb or delay buffer.

Every patch exposes *two* SOUND banks with the same four physical parameter knobs, cycled using EDIT: PATCH gives four high-value sound design controls; DETAIL gives four deeper, engine-specific controls. These banks control the same underlying patch state, and both are independent of the non-destructive HOME macro offsets. The reference simulator's illustrative defaults are **not JIANT factory patch data** or evidence that these parameter mappings are wired to real DSP yet.

| Candidate JIANT engine | PATCH bank: four controls | DETAIL bank: four controls | Ambient-first use and guardrails |
| --- | --- | --- | --- |
| **ANALOG** | Wave, Detune, Filter, Resonance | Glide, Drift, Width, Tone | Soft pads, drones and bass; keep resonance/gain constrained |
| **FM6** | Ratio, Mod Amount, Feedback, Brightness | Detune, Glide, Width, Drift | Distant bells/soft metallic pads; clamp high-frequency feedback |
| **STRING** | Excitation, Damping, Resonance, Tone | Glide, Drift, Width, Detune | Plucked/struck sustained textures, gentle string drones |
| **NOISE** | Colour, Filter, Grain, Motion | Width, Attack, Drift, Shape | Air, rain and synthetic texture; no piercing filtered noise |
| **DRUM-X** | Body, Snap, Decay, Noise | Filter, Tone, Resonance, Width | Sparse, soft generative percussion, even at high mutation settings |

The role → engine mapping is **curated per patch**, not fixed per role. If a chosen JIANT revision lacks one candidate engine or parameter, Codex must substitute a documented compatible control and test the sound—not expose a dead knob. Additive, phase or other heavyweight engines are *optional later*, evaluated through listening and CPU profiling, never loaded simply because a reference screenshot names them.

**HOME is a musician's macro layer:** each of its four musical controls can map non-destructively to multiple parameters across SOUND, ENVELOPE, LFO and FX. Its neutral position returns to the base state from EDIT; an EDIT change persists and is never silently overwritten by a HOME macro. Limit macro destinations/ranges per patch, smooth changes and compensate loudness so the entire knob sweep yields appealing ambient results. The physical MASTER potentiometer remains the separate, soft-takeover volume control, not a fifth macro.

**Effects routing:**

- **EDIT > FX** is *per selected layer* and SELECT cycles the five modules `DRIVE → CHORUS → DELAY → REVERB → SPATIAL`. Each module exposes four appropriate parameters on KNOBS 1–4. `SPATIAL` has Pan, Width, Drift and Rotate. Per-layer DELAY/REVERB primarily control **sends and per-layer shaping**, *not separate expensive delay/reverb engines per voice*.
- **GLO > MASTER FX** is shared processing. SELECT cycles `REVERB → DELAY → MOTION → WARMTH`. REVERB and DELAY provide a selectable **model** as a knob parameter (e.g. lighter JIANT Room/Plate versus more expensive Cloud/Shimmer/Resonant candidates; tape, clean or granular delay as resources allow). The same shared DSP buses and master macros drive both HOME and GLO; do not duplicate effect instances or settings merely because controls are exposed twice.
- **GLO > EQ** retains global equalisation and safe output control. **PATTERN** contains melodic FOLLOW instead of the removed MIX page. **HOME physical MASTER** owns overall or selected-layer level; EDIT has no volume-mixer duplicate.
- Favor light ambient defaults, stable reverb tails, smoothed feedback and parameters, safe effect-model crossfades or model-change deferrals, headroom and graceful ECO fallback. On-device profiling decides which models and simultaneous effects ship; browser presentation is *not* DSP capability proof.

**Browser UI test acceptance:** EDIT shows `1/6` through `6/6`, then wraps to MODE; PATCH and DETAIL update their four knob labels on actual engine change (e.g. **no FM Ratio on ANALOG**), SOUND changes without REC, MODE has visible REC commit, FX SELECT reaches SPATIAL on its fifth position, and GLO reverb/delay controls display their actual selected model. The host must additionally test solo, samples, save overwrite, chord evolution and pot pickup.

## Ambient DSP priorities

1. **Source synthesis:** start from JIANT ANALOG, FM6, STRING, NOISE and DRUM-X where practical; avoid porting every expensive engine in the first build. Be especially careful with multivoice FM6.
2. **Shared effects:** lightweight spacious ROOM reverb, one musical echo/tape-style delay and one gentle chorus/modulation path; independent per-layer sends, continuous tails when muting and changing a patch. Evaluate AMB-1's tape echo/long loop against JIANT options.
3. **Selective atmosphere:** SHIMMER/CLOUD/RESO, grain effects and long texture loops as selectable alternatives—not all simultaneously active by default. Master EQ, limiter/output headroom, per-layer warmth/saturation.
4. **Modulation:** 1–2 control-rate LFOs per active layer initially, slow default rates and curated destinations; avoid jitter and sudden audible steps.
5. **Voices and load:** use a shared voice limit, dynamic stealing with short fades and a fallback effects profile. Baseline/test four simultaneously active layers; compare actual audio CPU headroom on hardware. JIANT's load meter and ECO/LITE approach are good references, not free performance guarantees.
6. **Audio integrity:** smoothed changes, quantized harmonic transitions, voice-leading, held common tones, no accidental 100% wet jumps, stable output level across patches.

## Global generative conductor — v1.5 unified GLO / GENERATE

**Design:** One compact, audible *global conductor*, not three separate editor pages and not a fifth synthesizer layer. AMB-1's unequal loops, light Euclidean rhythms, compatible pitches and long density arcs are the initial musical recipe. Alternate AMB+ choices reuse an event-level shared scheduler; they are design targets, not existing DSP algorithms that have been proven on the hardware.

### Final GLO navigation (six pages)

`01 EVOLVE → 02 HARMONY → 03 GENERATE → 04 MASTER FX → 05 EQ → 06 VISUAL (later)`.

- **SEL** advances pages, with `n/6` header position. **SELECT does nothing on GENERATE** because the four physical knobs already have complete control of the page.
- **Knob 1 (RHYTHM):** eight global timing algorithms.
- **Knob 2 (PHRASES):** eight global development algorithms.
- **Knob 3 (INTERPLAY):** six global coordination algorithms.
- **Knob 4 (VARIATION):** one 0–100 amount applied to the global generation engine, with gentle, gain-safe musical boundaries. No new layer LFO or additional FX processing. Default 40.
- **ALGORITHM** retains its chord-proposal role on HOME and GLO/EVOLVE only, **REC** commits that proposal there. Neither is commandeered to select generator styles; no REC commit for changing GENERATE.
- **MASTER pot** keeps controlling the last selected HOME target's volume with soft takeover, even while GLO is open.

### Single-screen visualization — native 240 × 240 target

Reserve the standard header (0–47 px), content (48–146), the consistent helper strip (147–174), and four knob readouts (175–237). The main area has **three horizontal graphical tracks**: a step/pulse raster for RHYTHM, a swelling/returning line for PHRASES, and several interlocking micro-lanes for INTERPLAY. Each track labels the algorithm name and changes shape when the appropriate knob changes. The fourth knob scales visual intensity/complexity consistently. This is a *preview* of generative structure, **not an audio waveform** and not proof of the final music engine.

During playback, selecting a different discrete algorithm **queues the replacement for the next four-bar transition grid**; indicate queued style in amber (`Q`), never reset transport/notes or cut long effects tails. Turning a knob while stopped applies immediately. New selection replaces earlier queued selection; returning to the active style cancels that family’s pending change. Variation is a smooth continuous amount (not a required REC commit). Saving a piece stores all three algorithm IDs, the variation amount, deterministic seed/phase metadata where feasible, and any useful legacy settings.

### Actual potentiometers: must not be treated as endless encoders

The four parameter knobs on the real FM1 are **absolute physical pots**, while the HTML simulates clockwise/counterclockwise clicks. When GLO / GENERATE is opened, do not overwrite the saved algorithms with their physical pot positions. Implement **soft pickup** for each discrete selector, 8/8/6 stable bins, **hysteresis** around selection boundaries, and optional settle/debounce before previewing a newly crossed bin. Show a compact pickup cue without covering the graph. The fourth analog variation knob also needs pickup when entering the page. Use an audible, gradual change where appropriate. Define what happens if a user skips several selector bins quickly: queue only the final settled style. Human-test all bins, both rotation directions, repeated page changes, and switching HOME→GLO without surprise changes.

### RHYTHM — 8 options (Knob 1)

| No. | Style | Musical behaviour |
|---|---|---|
| 1 | **AMB DRIFT** (default) | Unequal-length loops + restrained Euclidean hits and rests |
| 2 | EUCLIDEAN | Mathematically spaced onsets and phase shifts |
| 3 | ODD LOOPS | Independent 5/7/11 cycles |
| 4 | POLYRHYTHM | Restrained 3:4 or 5:7 relationships |
| 5 | SPARSE | Longer probabilistic silences and anti-clustering |
| 6 | TIDAL | Gradual rhythmic density waves |
| 7 | BROKEN | Asymmetric gaps, offsets and accents |
| 8 | SLOW CLOCK | Soft stable pulse and long spaces |

### PHRASES — 8 options (Knob 2)

| No. | Style | Musical behaviour |
|---|---|---|
| 1 | **AMB BREATHE** (default) | Slow density arcs and periodic rests |
| 2 | LONG ARC | Development across many measures |
| 3 | WANDER | Slowly varying compatible motifs |
| 4 | BLOOM | Progressive addition/removal of details |
| 5 | CALL ANSWER | A phrase answered by a compatible phrase |
| 6 | DRIFT SHIFT | Gentle shift of loop phases |
| 7 | MEMORY | Familiar motifs return |
| 8 | MINIMAL | Rare changes and unusually long quiet moments |

### INTERPLAY — 6 options (Knob 3)

| No. | Style | Musical behaviour |
|---|---|---|
| 1 | **BLEND** (default) | Generated voices evolve independently within a shared harmony |
| 2 | STAGGER | Entrances offset in time |
| 3 | ALTERNATE | Parts answer each other |
| 4 | COMPLEMENT | Generated notes fill silence |
| 5 | SPARSE TALK | Deliberately limit simultaneous generated onsets |
| 6 | TIDE TOGETHER | Grouped rises, decays and rests |

### Non-destructive contract

**The conductor only emits future event-scheduling decisions for generative participation.** It may read the shared harmony, tempo, eligibility and local routing, but never rewrite a layer’s assigned type, patch, sample, volumes, effect sends, ADSR, LFO, or manual drum/note data. It must not edit factory or user preset assets. Any optional generated timbre/rhythm/FX variation uses bounded temporary offsets and a global variation amount, not base-patch edits. Manually recorded patterns remain stable; a layer explicitly chooses to participate in generated variation under EDIT/PATTERN. Layer FOLLOW stays in PATTERN. Maintain compatibility with older saved pieces (without `genVariation`: default to **40** and the three AMB-style defaults where IDs are missing).

### Minimal validation matrix: owner + Codex

1. With transport stopped, choose all 8 RHYTHM, all 8 PHRASES, all 6 INTERPLAY styles using **Knobs 1–3**; verify the visualization and style names respond independently. SELECT must not change them; SEL must continue through six GLO pages.
2. While playing, switch three styles within the same four-bar phrase; confirm none affects audio immediately, the *last requested* styles land together at the next boundary, and voices/effect tails remain smooth.
3. Sweep Knob 4 and verify only the variation amount changes; no individual stored layer field changes. Listen for useful changes at low and high variation without spiking CPU or turning ambience into frantic percussion.
4. Switch HOME↔GLO repeatedly with physical knobs left at different positions. A saved style must not jump until its pot is picked up; crossing thresholds must not flutter between choices.
5. Save and reload a piece. All three algorithms and the variation amount persist through safe power-cycling on supported flash. Old pieces load with defaults.
6. Compare before/after serialized layer data and manually recorded drum/note patterns: **byte-for-byte identical** through global algorithm changes. Automated and on-device test.
7. A 20-minute, low-volume, four-layer listening session: check tension/release, sparse/restful musicality, stereo placement, CPU/late audio blocks, no stuck notes.


## Shared generative music engine

- One stable key/mode, a progression graph of suitable next harmonies, short human-readable mood descriptors (e.g. warmer, darker, suspended, brighter); chord quality/mood are approximate, not guaranteed emotional outcomes.
- Layer-specific responses: pads play chord voicings, drone can hold pedal notes, bells play unequal-length compatible motifs, bass follows roots or voice-led steps, drums use Euclidean or manually edited rhythms, texture may ignore literal notes but react to energy/mood. A SAMPLE layer may follow shared harmony using a pitched loop, act as an evolving unpitched texture, or play a one-shot depending on its playback style; the source recording must not be unpredictably replaced when the composition evolves.
- ALGORITHM previews next compatible direction and REC commits on a bar boundary. AUTO interval OFF/1/2/4/8/16/32/64 bars. No duplicate Freeze feature.
- Independent evolution routing amounts: HARMONY, TIMBRE, RHYTHM, FX. Max amounts must not suddenly make every patch unusable. Allow individual layers to follow / stay anchored.
- Patterns: manual drum lane/step entry and constrained generative variations; deterministic seed for reproducible saves; manual locks don't mutate unless explicitly allowed.
- Aim for a 20-minute self-playing piece that has recognizable identity, silence, tension/release and long-term development rather than constant busy notes.

## Audio input, sampling and voice-to-ambient design

**Important hardware distinction:** April OS captures via a computer/laptop or compatible phone microphone **bridged to the FM1 over USB**. A normal bus-powered USB-C microphone cannot simply be plugged into the FM1. April documents 32 short sample positions of about three seconds, but we must not assume that allocation coexists with JIANT synthesis or our eight save slots. JIANT also has a version-specific USB audio IN+OUT / EXT path worth evaluating.

**V1 sampling goal:** one or two short mono sample slots (e.g. a spoken syllable or sung vowel) captured via a host microphone bridge or verified USB-audio input, then played by an explicitly assigned **SAMPLE layer**. Each of the four layers independently stores its selected source slot and SOURCE/PLAYBACK/CHARACTER settings; the sample memory bank may be shared for efficiency. A saved composition should reference stable sample IDs (with version or invalidation checks), rather than duplicating sample bytes or silently repointing to overwritten audio. Replacing a source referenced by a saved composition must warn the user and preserve referential integrity. Verify audio persistence and power-off restoration *before* promising more slots or live effects.

**Beginner-friendly SAMPLE HOME macros:** `PITCH`, `BLUR` (fade/overlap), `MOTION` (loop drift), `SPACE` (shared FX send). They must map to the **same** state editable in SAMPLE/PLAYBACK and SAMPLE/CHARACTER, not create a second audio path. Vowel → choir-like pad; whispered consonant → airy noise texture; humming → deep drone. SAMPLE/PLAYBACK exposes four styles: Pad, Drone, Texture, One-shot; these are **sub-options**, not extra top-level modes. Preserve voice identity when desired; avoid heavy real-time FFT/vocoder at first.

**Later options:** smooth crossfaded microloops, one lightweight grain engine, pitch following shared harmony, resampling, live input-through effects. Live processing requires separate latency/CPU validation from capturing a few seconds and replaying them.

**Contextual REC:** After assigning SAMPLE in EDIT/MODE (ALGORITHM previews, REC commits), enter EDIT/SOUND/SOURCE. **Only on SAMPLE/SOURCE** does REC start/stop audio capture. If its source slot already holds audio, the first REC arms replacement; the second starts capture; REC then stops it. SELECT changing sources or HOME must cancel an armed replacement. Outside SAMPLE/SOURCE, REC keeps the established meaning (step recording on PATTERN, visible proposal commit, SAVE confirmation). Capture needs input-level/clipping feedback and safe bounded writes; ensure the firmware does not lose an ongoing capture silently when changing pages. No direct USB mic is assumed, and the offline HTML does **not record audio bytes**.

## Hardware safety: mandatory before any custom image

The FM1 has **one flash bank**. A failed/interrupted installation can leave the device unable to boot normal firmware. There are known ROM/download-mode recovery paths, but recovery is not a guarantee. Newer late-2026 FM1 units may have a **UC25HQ80** flash chip instead of the older Puya family; code and save logic must support the user's actual chip, and not all third-party bootloaders have been tested on both.

**Do not flash AMB+ until all are true:**
1. Record FM1 serial/model label, stock firmware version, current bootloader and JEDEC flash ID (Puya vs UC25HQ80). Verify exact board compatibility.
2. Take **two full-flash read-only backups** with a compatible utility (e.g. M-Flasher), compare hashes/reads and keep at least two separate copies. Export current user presets/settings too. Do not share flash dumps publicly.
3. Secure the official compatible M-VAVE `.fwsc` and a known-good recovery procedure appropriate to that chip. Verify that the recovery tool can **detect the device** and that the backup is readable. Ideally first restore a known-good firmware and verify normal boot before experimenting.
4. Pin exact JIANT/AMB/April/ZEN revisions and check license notices. Create a flash memory map; **protect bootloader/config/device-data sectors** and avoid unnecessary writes to them.
5. Confirm all build checks pass: package identity, size/region constraints, RAM/stack/large buffer/heap ceilings, supported flash-chip commands, save-write bounds, boot checksums, watchdog and no accidental reserved-sector erase.
6. Use a stable USB data cable directly to the computer, no hub, no other app holding its MIDI/USB port, uninterrupted power. Never disconnect mid-write.
7. On **unknown/unsupported flash ID, failed backup or missing recovery path: STOP.** Continue testing only in the emulator. Do not install an alternative bootloader just to make development convenient.

**Suggested sources for preflight:**
- FM1 developers' recovery ladder: https://sbegueria.github.io/fm1-dev-wiki/Recovery.html
- M-Flasher project: https://github.com/jvitkauskas/m-flasher
- FM1 emulator newer flash-chip support: https://github.com/joakimeriksson/fm1-emu/releases/tag/v0.5.1
- Official stock package: https://www.m-vave.com/download (choose the verified FM1 firmware appropriate to your device)

## Milestones and HUMAN owner acceptance gates

At each stage Codex must hand over: (1) exact source commit + build artifact hash, (2) emulator and host test results, (3) memory/CPU and binary-size changes, (4) any new flash regions touched, (5) one-page expected physical controls and a rollback method. The human should **not** flash on the strength of an HTML UI demo or browser emulation alone.

### 0 — Inventory / backup / recovery (no custom flashing)
- **Codex:** identify actual board/flash, setup toolchain, map protected sectors, document safe restore and backup steps, pin repositories/licenses.
- **You:** confirm model/chip via tool output; make two matching full backups; export sounds/projects; locate official factory firmware; confirm the known-good image can be restored; store files safely.
- **Pass:** backup verified, restore plan specific to *your* unit. **Otherwise stop all physical flashing.**

### 1 — Baseline reference and measurement
- **Codex:** build one unchanged JIANT revision and confirm native host/emulator; record working engine, effects, load meter, save; test it in the emulator with the correct flash-chip model.
- **You:** audition JIANT and AMB-1 in the emulator (same headphone/speaker level), write down favourite patches, echoes, reverb tails and disliked sounds; optionally install **one known-good** firmware only after Phase 0.
- **Pass:** reproducible baseline, boot/recovery verified, source audio and load profile recorded.

### 2 — Four-layer shell and actual UI
- **Codex:** four dynamically assignable layers with seven eligible types (six synth roles plus SAMPLE) plus MASTER control target; five-item HOME selector; context-dependent four macros; MASTER pot soft takeover and safe gain smoothing; locked control map; consistent 240×240 screen bands/helpers; only actionable LEDs; correct keyboard scale LEDs; MODE preview/REC commit; SOUND SELECT immediate.
- **You:** on actual device if safe, cycle MASTER and all four layers with SELECT, test physical MASTER pickup on each target and emergency fade, switch Pad→Drone and Pad→SAMPLE via ALGORITHM then REC, check synth SOUND has two banks and SAMPLE has three, press SEL through pages, test Latch HOME/other shortcuts; ensure nothing overlaps and no surprise actions.
- **Pass:** every hardware input and LED has exactly one contextually clear meaning; HOME focus changes do not change saved levels; EDIT on MASTER reaches GLO/MASTER FX; returning HOME does not change or restart music.

### 3 — Preset-first audio and forgiving sound macros
- **Codex:** ship a small but polished starter bank (start with two excellent sounds per synthesized role), map four musical macros with smoothing/compensation; build toward eight patches per synthesized role and curated factory pieces; SAMPLE is source-slot based, not an eighth synth preset bank.
- **You:** play each sound, sweep each of the selected-layer and MASTER macros end-to-end, switch patches live, use 3–4 contrasting layers. Note any unpleasant harshness, clicks, silence or drastic volume jumps. Can you make 3 different attractive atmospheres without opening advanced EDIT?
- **Pass:** every starter sound is immediately enjoyable; any macro turn has understandable, useful effect; no crackle, clipping, effect-state duplication or physical volume jumps.

### 4 — Evolution/harmony and manual rhythms
- **Codex:** adapt AMB progression logic, unequal-length motifs and Euclidean rhythms; implement the unified GLO / GENERATE scheduling strategies and their non-destructive manual-pattern protection; add per-layer harmonic following, manual drum steps, REC step recording, interval auto evolution, preview with mood descriptors and REC commit.
- **You:** play for 15–20 minutes; move manual Cm7→compatible chord, reverse/advance, toggle 4/8/16 bar auto, verify chord remains within configured key; test AMB DRIFT, Euclidean and Odd Loops, plus slow phrase and interplay changes, keeping some manually recorded kick/hat steps locked. Confirm generated events change but local manual patterns and sound edits are untouched; compare the saved configurations before and after.
- **Pass:** no jarring note cutoffs, tempo drift or random-incompatible notes; global changes land at phrase boundaries, do not overwrite per-layer state, recall correctly and keep a self-playing piece interesting.

### 5 — Ambient effects, EQ, save/recall
- **Codex:** select one light shared reverb, tape/echo, chorus where affordable, four per-layer sends, master EQ and headroom management, preserve tails and full state. Finalize user save slots after storage audit (eight target; show owner if sampling forces a trade-off).
- **You:** mute/solo layers and switch patches without chopping wet tails; test very wet pads + bells + sparse drums for 20 minutes. Save to occupied slot (REC twice), cancel overwrite by turning SELECT, power-cycle, recall and compare arrangement. Record baseline and worst-case CPU/load.
- **Pass:** no audible dropouts, usable save after power-off, no accidental overwrite, stable headroom. Define a quantitative peak-load threshold from hardware measurements and keep a safety margin, not just 'it worked once'.

### 6 — Voice/microphone sampling, small and reliable (DEFERRED after first full build)
- **Codex:** evaluate JIANT USB input vs April-style mic bridge; enable a SAMPLE layer using one captured 1–3 s mono source, with SOURCE → PLAYBACK → CHARACTER banks, style (Pad/Drone/Texture/One-shot), trim, crossfade, pitch, blur, motion and dry/wet; guard all flash writes and document sample storage.
- **You:** choose a layer on HOME with SELECT, EDIT → MODE → preview SAMPLE using ALGORITHM → REC to commit. Then EDIT → SOUND / SOURCE; use SELECT to choose an empty source slot, connect the supported computer/phone mic bridge, hum 1–3 seconds and REC start/stop. EDIT cycles PLAYBACK and CHARACTER; turn PITCH/BLUR/MOTION/SPACE on HOME and check they match detail values. Test a second SAMPLE layer referencing the same/different source, then leave SAMPLE and return without disturbing another layer. Try recording into an occupied slot: it must require explicit replacement confirmation. Power-cycle and check sample persistence only after safe storage support is proven.
- **Pass:** reproducible recording and playback, no clipping, bounded memory, correct persistence, no sample bank corrupting piece saves, no accidental sample replacement, correct per-layer sample settings. Only then try optional granulation or live-through effects.

### 7 — Stress, recovery drills, and release candidate
- **Codex:** automate long-running tests, all mode transitions, persistent saves with new/old flash chips, input bursts, key LED test, high polyphony and CPU-based fallback. Ensure crash/watchdog route and known-good restore, release checksum/source mapping.
- **You:** 30–60 min continuous playback; repeatedly change patches, modes, FX, AUTO intervals, REC, save, USB connection; power-cycle with saved state; verify output over wired headphones. Only if safe, rehearse recovery using a *known-good firmware* rather than intentionally bricking the unit.
- **Pass:** zero dropouts under agreed test scenarios; no loss of user data; controlled rollback path documented; do not release to friends until stable on your exact hardware variant.

### 8 — **Much later / DEFERRED**: pure generative VISUAL performance mode
- **Reference:** ZEN-1's music-linked scenes and testing infrastructure, but **our audio engine stays active and authoritative**. The visuals are a view of music, not a substitute music engine.
- **Entry/exit proposal:** long-press HOME (distinct from short HOME and HOME+SELECT; test the gesture on hardware). No text, numbers, values, overlays or page badges while visual mode is active.
- **Themes:** (a) scenic landscape—moving clouds, mist, stars, horizon and rain; (b) abstract—constellations, luminous fields, orbiting threads, drifting particles. Both should be low-cost pixel/primitive art, not shader-heavy artwork.
- **Music-to-visual mappings:** chord/tension -> colour palette / sky; note attacks -> ripples or particles; bass -> horizon movement; tempo/beat -> subtle oscillation; reverb tail -> mist; timbre/brightness -> light/texture; layer activity -> several visual agents. The four HOME knobs should continue editing the **selected HOME target's** macros (MASTER or any layer), reflected immediately in the landscape. ALGORITHM previews a destination with a gentle visual shift; REC's **physical LED** pulses for commit, rather than drawing an on-screen prompt.
- **Codex:** test drawn pixels per frame, 8–12 fps initial target, dirty rectangles, load-based frame skipping, no music changes on entry/exit, no allocation or flash writes per frame. ZEN shows the 240×240 device can do compelling scenes, but our additional synth/FX burden must be profiled separately.
- **You:** leave it in visual mode for 30 min, evolve chords and change knobs, confirm exact same music output as standard mode, no screen stutter causing audio dropouts, REC commit visible via hardware LED, and easily return HOME.
- **Pass:** visuals enhance playing without ever competing with audio. If they reduce audio reliability, lower frames/detail or disable visual mode under extreme load.

### Scope boundaries: what we deliberately postpone

- Full multi-layer arbitrary heavy reverbs; unlimited polyphony; deep FM6 operator editing in the primary UX; sophisticated real-time vocoder/FFT; live wireless audio; audio export; hundreds of writable sample slots; polished graphics before audio stability; an untested replacement bootloader.

## Definition of success

A person who knows little synthesis can switch the FM1 on, select a lovingly curated ambient piece, choose **MASTER or any one of four instruments** with SELECT, use the four knobs to shape the target, use the physical MASTER pot for its selected target volume without a jump, evolve chords intentionally or automatically, add gentle custom rhythms, assign a SAMPLE layer and record a small voice fragment as an atmospheric source, save safely, and listen for twenty minutes without needing a DAW. Later, one hold of HOME turns the screen into a peaceful, music-reactive window with no numbers.

## Decision flags for the owner before Codex final implementation

**Locked UX in v1.3:** SAMPLE is the seventh selectable layer type. Synth roles only have PATCH/DETAIL; SAMPLE alone has SOURCE/PLAYBACK/CHARACTER. MODE assignment requires ALGORITHM preview + REC commit, synth PATCH changes via SELECT are immediate, and REC on SAMPLE/SOURCE performs guarded capture. Four active audio layers remain the maximum planned UI count, regardless of role.

1. **Flash chip/bootloader** — must be identified before any flash; unsupported combination blocks the hardware milestone.
2. **Save slots versus sample storage** — aim eight writable pieces, but latest JIANT branches may allocate flash for samples by reducing from eight to six songs. Needs measured trade-off and explicit owner choice.
3. **Mic path** — April browser mic bridge vs JIANT IN+OUT/EXT: profile and choose, do not assume they work interchangeably.
4. **Granular FX** — ambitious extra; only enable if ambient benefit outweighs measured CPU/large buffer cost.
5. **Visual renderer** — late-stage, independently performance-gated.
6. **Fixed-position MASTER pot interaction** — the *firmware viability* of reassigning the ADC input is distinct from whether its soft-takeover UX feels good. Test pickup range, analog jitter, safety fade and control access on the exact hardware. Never promise a motorized/illuminated potentiometer.
7. **Four MASTER macros and shared FX coupling** — calibrate buses, sends, EQ and macro mapping to produce a consistent, musical result; do not duplicate effect state.

**The owner starts the complete Codex implementation workflow by saying “Read START_HERE.md and execute the complete project.” This does not authorise hardware flashing.**

## Prototype/reference contract

The companion **AMB+ FINAL Interaction Reference** HTML is a **browser interaction specification**, not firmware or proof of hardware compatibility. It uses a native-sized 240×240 canvas and models the actual controls, but its graphics/fonts/LED pulse/CPU do not constitute an FM1 driver benchmark. Codex should port and unit-test each state transition and the on-screen page regions in native C, verify the correct FM1 hardware revision, and retain this roadmap as the authority for sound, safety and user acceptance gates. If the demo and this plan disagree, resolve explicitly with the owner before implementing a behavioural change.
