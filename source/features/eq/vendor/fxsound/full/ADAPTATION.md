# H hub adaptation
Pinned FxSound source: d8e7a23d37ed5939c2a3090a1c1756c7f2500b17, AGPL-3.0-or-later.
Uses original Fidelity, Ambience, Surround, DynamicBoost and Bass processors.
Each stream owns a separate DSP object. Session settings use a fixed per-handle
memory table; registry APIs are disabled. No FxSound GUI or driver is included.
Registry refresh is removed, and upstream wrapper pointers are initialized.
H hub presents twelve editable bands. Neutral outer bands use the original ten-band EQ; active outer bands use twelve bands. Band counts and validation are per handle, not a mutable process-wide count. H hub owns presets, preamp protection, limiter and routing; EQ and five enhancements are processed by the original DSP.

