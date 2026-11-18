# AMB+ implementation status

Latest v0.6 playing checkpoint, branch ambplus/uniform-transition-time, implementation 596d2d151132921da807cb3fb44b5ff7df66c1fe. Uniform active-synth transition duration extends short releases as well as shortening long ones. Current envelope level is captured without a boost; FM6 operator/pitch envelopes are held while oscillators continue. Full-/half-level regression guards fixed-point precision. No extra incoming fader, voice or audio buffer. Realtime overload protection remains.

Queued conductor styles transition generated layers at four bars; complete factory/user recalls coordinate old sources/master/EQ/FX until release completion before entering the new piece. Normal AUTO chords keep their ordinary envelopes. HOME shows selected-layer gain temporarily while MASTER moves, returning to MASTER after 1200 ms without a gain change.

Fresh Texture recipes are NOISE, sustained and AUTO STEADY, ignoring sparse policy events by default. Manual empty slots release the part; ARP clears the selected manual pitched slot. Other Texture modes permit gated activity. New High Air/Distant Hiss replace pitched factory Glass Wind/Distant Voices. Explicit compatibility for saved old PHASE textures preserves their parameters and names. Save schema remains v4/1420 B; older v0.5 may reject new Noise Texture recipe IDs 2/4. No sector/driver/layout changes.

See PLAYING_CHECKPOINT_VALIDATION_REPORT.md, TESTING_CADENCE.md and ../../release/ambplus-0.6-transitions-textures/. This is a short-checked incremental candidate, not a new long-soak milestone. No Shrub material or physical FM1 actions. Human acceptance and hardware gates remain open. FLASH STATUS: NOT AUTHORISED.
