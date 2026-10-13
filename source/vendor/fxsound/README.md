# FxSound EQ integration

The H hub EQ uses FxSound's parametric coefficient design from the pinned
upstream revision in SOURCE.json. FxSoundFilters.h retains its algorithm and
copyright notice, with double precision and a small standalone adapter.
The original source files are included under upstream for comparison.

H hub retains twelve adjustable bands, custom frequencies, presets, per-app
and per-output settings, automatic preamp, bypass, smooth parameter changes,
meters, host limiter, and route restoration. Its existing audio routing is
retained. The five FxSound enhancement processors are included under full and run in independent per-application instances. The FxSound GUI and driver are not bundled. All five controls use the original 0-10 range and are saved with presets.
Frequency response is calculated from the same coefficients used for audio.

License: AGPL-3.0-or-later. Distribution of the combined derived program must
comply with AGPL, including corresponding source and retained notices. The
release source archive includes the H hub sources and this adaptation.

