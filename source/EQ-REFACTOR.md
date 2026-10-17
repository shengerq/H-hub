# General EQ refactor

The original FxSound EQ and five enhancement processors remain the only live
tone engine. Their source and existing patches are unchanged. EqDsp now only
estimates the displayed response and automatic headroom. EqProcessor applies
one smoothed output gain and one final safety ceiling after FxSound; the former
extra input filter pass and intermediate clipping stage have been removed.

EqBuffer separates legitimate WASAPI silent capture packets from render
starvation. A render request is capped to both device space and captured frame
count. Reads never pad missing frames. The existing start threshold, underrun
stop/re-prime behavior, recovery fade, RAW output fallback and route journals
are retained. The former missing-frame branch was unreachable because needed
was already bounded to queue.size(). Audit recording remains opt-in and now
reserves large capture arrays only when enabled.

All 13 imported FxSound factory presets are unchanged. Hhub's flat/FPS/music
examples are removed from the menu. Saved curves retain their numerical
parameters and become custom when their former example preset is retired.
User-created presets remain intact.

EqNode uses the PolicyConfig interface already used by FxSound to name render
and capture endpoints Hhub Audio and enable/disable them. Streams restore their
application routes before node shutdown. Physical render defaults are retained
per role across activation and installation; virtual nodes are never deliberately
made the system default. A running FxSound instance prevents shared-node changes.
The UI reports endpoint lifecycle failures instead of silently hiding them.

General EQ only owns applications already playing on its selected output, or
applications it previously routed to its own source. Processes simultaneously
using another output are excluded. Explicit application/scene bindings retain
priority. This avoids moving unrelated speakers/headphones into the same curve.

Regression coverage includes exact queued frame counts, untouched output tails,
null silent packets, irregular DSP blocks, direct FxSound comparisons for every
factory preset, final ceiling/bypass, migration persistence and output isolation.
Current validation environment exposes only Remote Audio; physical FxSound
endpoint naming and enable/disable cannot be exercised in this session.
