# Phase 3: Standalone Windows Monitoring - Context

**Gathered:** 2026-06-23
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 3 turns the existing standalone LumaScope shell into a real Windows monitoring application. A standalone user can choose either a normal Windows/JUCE-managed input device or a Windows render/output endpoint captured through shared-mode WASAPI loopback, then inspect that source through the same Phase 2 analyzer, snapshot, bridge, and canvas renderer path.

This phase owns standalone source selection, source lifecycle state, input-device capture, WASAPI loopback capture, conversion into the analyzer ingress contract, source switching, invalidation/removal handling, restart behavior, visible source/error/silence states, and persistence of the last valid source preference.

This phase does **not** add licensing or activation UI; those remain Phase 6. It does **not** add advanced analyzer controls; v2 control requirements remain deferred. It does **not** change the VST3 into a device-capture host; VST3 continues to analyze host-supplied audio only.

</domain>

<decisions>
## Implementation Decisions

### Source Model and Picker
- **D-01:** Standalone source selection uses two clear modes: `Input Device` and `System Output`.
- **D-02:** `Input Device` represents ordinary JUCE-managed input devices such as microphones and audio interfaces.
- **D-03:** `System Output` represents Windows render endpoints captured through shared-mode WASAPI loopback without requiring vendor “Stereo Mix” devices.
- **D-04:** Do not combine input devices and render endpoints into one ambiguous list. The user should understand whether they are monitoring a recording input or the system speaker/output mix.

### Failure, Removal, and Recovery
- **D-05:** If the selected source disappears, fails, or becomes invalid, standalone capture stops and the UI clearly asks the user to choose a source again.
- **D-06:** Do not silently auto-fallback to the default source after a selected source fails. Avoid surprising capture of a different microphone or output device.
- **D-07:** Automatic retry may be used internally only as part of bounded recovery for the same selected source, but any persistent failure must surface as a stopped/error state requiring user action.

### Silence and Idle State
- **D-08:** If the selected source is valid but currently silent, keep capture active and show a subtle “No signal detected” status.
- **D-09:** Treat silence as distinct from failure. The analyzer may settle to an empty/low spectrum, but the UI must not imply the device is broken.

### Standalone UI Surface
- **D-10:** Standalone source controls live in a compact control strip above the analyzer stage.
- **D-11:** The strip should include mode selection, source selection, and concise source/silence/error status without turning the app into a mixer or settings dashboard.
- **D-12:** Standalone-only controls must be omitted in VST3. The React/MUI app should branch from `host.info.hostMode` or an equivalent typed protocol signal rather than duplicating shells.

### Persistence and Startup Defaults
- **D-13:** On standalone startup, restore the last valid source if it is still available.
- **D-14:** If the saved source is unavailable, start stopped with a clear “Choose source” state.
- **D-15:** Do not auto-select a different default input/output on startup when the saved source is missing; the user must choose again.

### the agent's Discretion
- The planner may choose the exact native class boundaries for shared source lifecycle, JUCE input capture, and WASAPI loopback capture.
- The planner may choose the exact persisted identifier format, as long as it avoids secrets, survives normal Windows device naming churn where practical, and fails visibly when no confident match exists.
- The planner may choose precise wording and visual severity for source states, as long as selected failure, stopped/no-source, and valid-but-silent are visibly distinct.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project Scope and Phase Contract
- `.planning/PROJECT.md` — Product scope, Windows-only constraint, validated Phase 2 analyzer state, and active standalone monitoring requirement.
- `.planning/REQUIREMENTS.md` — Phase 3 requirements `CAP-01` through `CAP-05` and `UI-04`.
- `.planning/ROADMAP.md` — Phase 3 goal, success criteria, planned work breakdown, and dependency on Phase 2.
- `.planning/STATE.md` — Current project position, accumulated decisions, and baseline concerns.

### Prior Phase Foundation
- `.planning/phases/01-reproducible-product-shell/01-CONTEXT.md` — Product identity, standalone target baseline, visual shell direction, WebView bridge decisions, and Vite/embedded workflow constraints.
- `.planning/phases/02-end-to-end-vst3-analyzer/02-CONTEXT.md` — Analyzer defaults, snapshot handoff, bridge/renderer decisions, and explicit deferral of standalone capture to Phase 3.
- `.planning/phases/02-end-to-end-vst3-analyzer/02-VERIFICATION.md` — Evidence that the analyzer path, VST3 host behavior, pluginval validation, and Ableton smoke retest passed.

### Native Analyzer and Bridge Implementation
- `plugin/include/LumaScope/PluginProcessor.h` — Current processor-owned analyzer and latest-snapshot API.
- `plugin/source/PluginProcessor.cpp` — Current analyzer ingress from audio blocks and mailbox publication.
- `plugin/include/LumaScope/PluginEditor.h` — Editor snapshot polling boundary, WebView bridge ownership, and host-mode bridge setup.
- `plugin/source/PluginEditor.cpp` — WebView2 setup, bridge readiness, timer callback, snapshot event emission, embedded/Vite behavior, and fallback diagnostics.
- `plugin/include/LumaScope/HostBridge.h` and `plugin/source/HostBridge.cpp` — Current typed protocol-v1 bridge contract and `Standalone` host mode support.
- `plugin/include/LumaScope/Analyzer/SpectrumAnalyzer.h` and `plugin/source/Analyzer/SpectrumAnalyzer.cpp` — Reusable analyzer core that standalone capture must feed without changing analyzer behavior.
- `plugin/include/LumaScope/SnapshotMailbox.h` — Latest-snapshot-wins handoff model to preserve bounded UI behavior.
- `ui/src/bridge/protocol.ts` and `ui/src/bridge/BridgeProvider.tsx` — TypeScript protocol parsing, `hostMode`, and spectrum snapshot state.
- `ui/src/app/AppShell.tsx`, `ui/src/components/AnalyzerStage.tsx`, and `ui/src/components/StatusFooter.tsx` — Existing shell regions where the compact standalone source strip and source status should integrate.

### Build, Test, and Documentation Integration
- `plugin/CMakeLists.txt` — Native target source registration and JUCE module linkage.
- `tests/native/CMakeLists.txt` — Native test target pattern for standalone source lifecycle, conversion, and recovery tests.
- `scripts/test-all.ps1` and `scripts/verify-project.ps1` — Verification entry points that Phase 3 should extend.
- `docs/bridge-protocol.md` — Current protocol-v1 event model and versioning rules; source-state events must extend it intentionally.
- `docs/development.md` and `docs/troubleshooting.md` — Developer workflow and diagnostics model that Phase 3 must preserve.
- `README.md` — User-facing setup/run documentation that should gain standalone monitoring instructions during or after this phase.

### External Documentation Sources
- Context7 library `/websites/juce_master` — Use for current JUCE `AudioDeviceManager`, standalone plugin holder/device setup, `AudioProcessorPlayer`/callback guidance, and WASAPI audio device behavior.
- Context7 library `/janwilczek/juce-webview-tutorial` — Use when changing WebView packaging or native/web communication patterns.
- Context7 library `/websites/mui_material-ui` — Use when adding Material UI source controls and accessible status presentations.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `lumascope::SpectrumAnalyzer` already accepts audio blocks and produces bounded display-ready snapshots; standalone capture should feed this same contract rather than adding a parallel analyzer.
- `LumaScopeAudioProcessor` already owns analyzer state and a `SnapshotMailbox`; planning should decide whether standalone capture reuses processor ownership directly or extracts a shared monitoring engine around the same analyzer/mailbox model.
- `lumascope::EditorSnapshotPoller` already enforces bounded latest-snapshot emission from native to WebView; Phase 3 should preserve that message-thread polling boundary.
- `HostBridge` already validates `hostMode` as `VST3` or `Standalone`; source-state protocol additions can branch cleanly from this existing host mode.
- `AnalyzerStage` and `SpectrumCanvas` already render live snapshots in the central stage; Phase 3 should add source controls/status around the stage instead of redesigning the renderer.

### Established Patterns
- Native/web communication uses JUCE native events with closed, typed, versioned payloads; no string-built JavaScript evaluation.
- Web payloads are bounded and validated on both sides. Source-list and source-state additions should preserve stable error envelopes and parser tests.
- Audio/thread boundaries are strict: no allocations, disk I/O, network I/O, or WebView work on real-time capture callbacks.
- Latest complete snapshot wins; stale frames are dropped rather than queued.
- Vite development remains explicit and local at the approved loopback URL; embedded assets remain the normal packaged path.

### Integration Points
- Standalone source lifecycle likely needs a native boundary that can be tested without constructing a WebView.
- JUCE-managed input capture should integrate with the standalone app/device manager path while preserving the processor/analyzer ingress contract.
- WASAPI loopback capture will need a Windows-specific adapter for render endpoints, format conversion, invalidation/default-device notifications where practical, and restart/teardown safety.
- React should receive source list/state/error updates through protocol-v1 additions and render the compact standalone-only strip when host mode is `Standalone`.

</code_context>

<specifics>
## Specific Ideas

- The standalone source picker should feel obvious: choose `Input Device` or `System Output`, then choose the concrete device/endpoint.
- A disappeared or failed source should not silently become another source; stop and make the user choose again.
- A valid but silent source should remain active and show “No signal detected” quietly.
- The source UI belongs in a compact strip above the spectrum, close to the analyzer but below the brand header.
- Startup should restore the last valid source only when it is still available; otherwise it should start in a clear “Choose source” state.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within Phase 3 scope.

</deferred>

---

*Phase: 3-Standalone Windows Monitoring*
*Context gathered: 2026-06-23*
