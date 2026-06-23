# Phase 2: End-to-End VST3 Analyzer - Context

**Gathered:** 2026-06-23
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 2 turns the Phase 1 LumaScope shell into a real VST3 spectrum analyzer. A user can insert the VST3 on host audio, hear unchanged output, and see a correct, smooth logarithmic spectrum in the existing React/MUI WebView stage.

This phase owns the analyzer DSP core, deterministic FFT/spectrum tests, real-time-safe audio ingress, latest-snapshot handoff, efficient WebView rendering, VST3 passthrough behavior, editor lifecycle safety, plugin validation, and one honest real-DAW smoke path.

This phase does **not** add standalone device selection or WASAPI loopback capture; those remain Phase 3. It does **not** add activation/licensing UI; that remains Phase 6. It does **not** ship full user-facing analyzer controls; v2 control requirements remain deferred, though the core should expose preset/configuration extension points.

</domain>

<decisions>
## Implementation Decisions

### Analyzer Presets and Defaults
- **D-01:** Implement three analyzer preset profiles in the DSP/model layer: `Musical`, `Measurement`, and `Fast`.
- **D-02:** The default visible Phase 2 behavior is `Musical`.
- **D-03:** `Musical` defaults should be a balanced mixing-oriented analyzer: 4096-point FFT, Hann window, logarithmic 20 Hz-20 kHz display, bounded ~30 FPS UI snapshots, and moderate smoothing/decay so the display is stable rather than twitchy.
- **D-04:** `Measurement` and `Fast` must be real preset profiles/configuration paths in the core, not just names, but full user-facing mode switching may remain an extension point unless it is trivial and does not displace the Phase 2 MVP.

### Real-Time Audio and Snapshot Handoff
- **D-05:** Use a latest-snapshot-wins handoff. If UI rendering lags, stale frames are dropped rather than queued.
- **D-06:** Audio callbacks must never call WebView, perform JSON work, allocate dynamically, block, perform disk/network I/O, or rely on locks that can contend with the UI.
- **D-07:** Analyzer processing should publish complete snapshots through a preallocated real-time-safe boundary. The UI/editor consumes the newest complete snapshot at its bounded display rate.
- **D-08:** The analyzer must continue running while the editor is closed. Reopening the editor should catch up quickly from the latest available snapshot without requiring audio processing to restart.

### Spectrum Rendering
- **D-09:** Render the first real spectrum as a smooth filled spectral curve with subtle glow/fill, matching the existing dark LumaScope instrument-panel look.
- **D-10:** Rendering must be efficient: no DOM/MUI element per FFT bin. Prefer canvas, SVG path, or another bounded-node strategy suitable for WebView2.
- **D-11:** Keep the existing central analyzer stage as the visual home for the renderer and preserve room for future labels/controls without implementing v2 controls in this phase.

### VST3 Behavior and Host Proof
- **D-12:** The VST3 is an analyzer/effect passthrough. Output must be sample-equivalent to input within floating-point tolerance for supported mono/stereo layouts.
- **D-13:** The analyzer must not alter gain, latency, channel count, or samples; it only observes audio.
- **D-14:** Automated tests should cover silence, deterministic tones, mono/stereo layouts, sample-rate changes, block-size variation, passthrough equivalence, editor-open/editor-closed operation, and malformed/edge conditions.
- **D-15:** Ableton Live is the preferred manual DAW smoke target for Phase 2. If Ableton is unavailable or cannot be verified enough in the environment, the executor must record that honestly and use the best available fallback without claiming Ableton passed.
- **D-16:** Automated plugin validation remains required where practical, with pluginval/VST3 validation preferred and any environmental limitations documented explicitly.

### Agent's Discretion
- Exact `Measurement` and `Fast` preset numerical values, provided they are meaningfully distinct from `Musical` and covered by tests.
- Exact FFT overlap/hop strategy, smoothing coefficient model, dB floor/ceiling, interpolation, and log-bin count, provided deterministic tone tests verify frequency placement and level tolerance.
- Exact snapshot payload shape and renderer implementation technology, provided the protocol remains typed/versioned, bounded, and efficient.
- Exact DAW fallback order if Ableton Live is not available.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project Scope and Phase Contract
- `.planning/PROJECT.md` — Product scope, constraints, current validated decisions, and phase-transition state.
- `.planning/REQUIREMENTS.md` — Phase 2 requirements `DSP-01` through `DSP-06`, `VST3-01` through `VST3-04`, and `UI-02`.
- `.planning/ROADMAP.md` — Phase 2 goal, success criteria, planned work breakdown, and dependency on Phase 1.
- `.planning/STATE.md` — Current project position, accumulated Phase 1 decisions, and known baseline concerns.

### Prior Phase Foundation
- `.planning/phases/01-reproducible-product-shell/01-CONTEXT.md` — Locked product identity, visual shell direction, developer workflow, and WebView bridge decisions.
- `.planning/phases/01-reproducible-product-shell/01-VERIFICATION.md` — Evidence that the shell, bridge, embedded/Vite modes, and Context7 guidance are already validated.
- `.planning/phases/01-reproducible-product-shell/01-01-SUMMARY.md` — Walking skeleton and protocol-v1 handshake summary.
- `.planning/phases/01-reproducible-product-shell/01-02-SUMMARY.md` — Approved LumaScope visual shell summary.
- `.planning/phases/01-reproducible-product-shell/01-03-SUMMARY.md` — Developer workflow, diagnostics, and Vite `5174` summary.

### Native/Web Protocol and Implementation Points
- `docs/bridge-protocol.md` — Current protocol-v1 event model and versioning rules; Phase 2 snapshot messages must extend this intentionally.
- `docs/development.md` — Embedded and Vite workflow expectations, including canonical Vite URL `http://127.0.0.1:5174`.
- `docs/troubleshooting.md` — Existing diagnostics model that Phase 2 must not break.
- `plugin/include/LumaScope/PluginProcessor.h` — Current processor interface and process lifecycle hooks.
- `plugin/source/PluginProcessor.cpp` — Current transparent processor implementation and VST3 bus support baseline.
- `plugin/include/LumaScope/PluginEditor.h` — Current editor/browser/timer boundary where UI polling or snapshot consumption can attach.
- `plugin/source/PluginEditor.cpp` — Existing WebView2 setup, bridge handshake, fallback handling, and smoke-result behavior.
- `plugin/include/LumaScope/HostBridge.h` and `plugin/source/HostBridge.cpp` — Current native bridge contract and validation pattern.
- `ui/src/bridge/protocol.ts` and `ui/src/bridge/BridgeProvider.tsx` — Current TypeScript protocol parsing and bridge state model.
- `ui/src/components/AnalyzerStage.tsx` — Existing central stage and `#spectrum-renderer-mount` reserved for the Phase 2 renderer.

### Build and Test Integration
- `plugin/CMakeLists.txt` — Native target source registration and JUCE module linkage.
- `tests/native/CMakeLists.txt` — Native test target pattern to extend for DSP, passthrough, and bridge/snapshot tests.
- `scripts/test-all.ps1` and `scripts/verify-project.ps1` — Verification script entry points that Phase 2 should extend.
- `README.md` — User-facing setup and run documentation that should evolve once the analyzer is real.
- `AGENTS.md` — Project guidance, Context7 source IDs, and workflow rules.

### External Documentation Sources
- Context7 library `/websites/juce_master` — Use for current JUCE AudioProcessor, FFT/DSP, WebBrowserComponent, and VST3 guidance.
- Context7 library `/janwilczek/juce-webview-tutorial` — Use for WebView packaging and native/web communication patterns.
- Context7 library `/websites/mui_material-ui` — Use when changing Material UI components or theming.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `LumaScopeAudioProcessor` already supports mono/stereo bus-layout checks and has the correct `prepareToPlay`, `processBlock`, and editor creation hooks for attaching analyzer state.
- `processBlock` is currently empty, so transparent passthrough is implicit but untested; Phase 2 should make passthrough intent explicit and test it.
- `LumaScopeAudioProcessorEditor` already owns a timer, WebView bridge, and fallback model. This is the natural place to consume snapshots at a bounded UI/display cadence, as long as audio-thread safety is preserved.
- `AnalyzerStage` already exposes a dedicated renderer mount and visual shell that can host a canvas/SVG renderer without redesigning the Phase 1 UI.
- Existing native and frontend test harnesses are in place and should be extended rather than replaced.

### Established Patterns
- Native/web communication uses JUCE native events with closed, typed, versioned payloads; no string-built JavaScript evaluation.
- Web payloads are bounded and validated on both sides. Snapshot additions should preserve stable error envelopes and parser tests.
- Embedded assets are authoritative for normal builds; Vite is explicit development-only at `http://127.0.0.1:5174`.
- Phase 1 intentionally reserved the analyzer stage for Phase 2; do not redesign the whole app shell to fit the renderer.

### Integration Points
- DSP core should live behind processor-owned state that is independent of editor lifetime.
- Real-time ingress should connect from `processBlock` to analyzer/snapshot state without allocation, locks, or WebView calls.
- UI snapshot delivery should extend the bridge/protocol layer and frontend `BridgeProvider`/renderer state in a versioned way.
- Renderer should replace the placeholder copy only when live or test spectrum data is available; existing bridge error and connecting states should remain visible.
- Test and verification scripts should add deterministic DSP tests, passthrough tests, renderer/protocol tests, plugin validation, and documented DAW smoke instructions.

</code_context>

<specifics>
## Specific Ideas

- The analyzer should feel like a polished musical tool by default, while the underlying core is extensible enough for measurement-style and fast visual profiles.
- The filled spectral curve should feel luminous and instrument-like, not like a stock chart.
- Ableton Live is the preferred manual DAW smoke host for this phase.
- If local DAW/pluginval availability prevents a full host proof, documentation must say exactly what was and was not verified.

</specifics>

<deferred>
## Deferred Ideas

- Full user-facing analyzer controls for FFT size, windows, smoothing, decay, hold/freeze, and cursor inspection remain v2 `CTRL-*` scope.
- Standalone audio input and WASAPI loopback source selection remain Phase 3.
- Activation/licensing UI remains Phase 6.

</deferred>

---

*Phase: 2-End-to-End VST3 Analyzer*
*Context gathered: 2026-06-23*
