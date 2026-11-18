# Transitions and harmonic variety

Base: e0dc1fcc35cfc1fc8bdd3a3b8aff24884f99727b (v0.2 code f87a437b).
Scope: real wet-tail bridge with source fade-out/fade-in for committed harmony and GENERATE changes; eliminate AUTO's 0↔3 cycle; diversify Pad voicing/phrasing; make MANUAL↔AUTO obvious and retain manual notes.

Transitions fade only changing generated sources before the dry/send split. Existing shared reverb/delay returns are never faded or reset. At zero source gain, old source voices are retired and fresh notes fade in; this avoids doubling the DSP voice budget. Dry parts have source fades, not invented wet tails. Manual locked layers are unaffected by global changes. No flash/loader/layout/hardware changes; FLASH STATUS: NOT AUTHORISED.
