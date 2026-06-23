---
phase: 03-standalone-windows-monitoring
plan: 03
type: execute
wave: 3
subsystem: "standalone source UI"
tags:
  - "source-protocol"
  - "react"
  - "material-ui"
  - "source-preferences"
  - "standalone-diagnostics"
requires:
  - "03-01"
  - "03-02"
provides:
  - "CAP-01 (standalone source selection UI)"
  - "CAP-02 (system-output selection UI)"
  - "CAP-04 (visible source states and request handling)"
  - "CAP-05 (last valid source preference persistence)"
  - "UI-04 (standalone-only source controls)"
affects:
  - "BridgeProvider"
  - "PluginEditor"
  - "StandaloneSourceController"
  - "Activation UI planning"
tech-stack:
  added:
    - "@testing-library/user-event for source-strip interaction tests"
  patterns:
    - "Closed protocol-v1 source parsers in TypeScript"
    - "Standalone-only React controls gated by host.info.hostMode"
    - "Native source request dispatch on the message thread only"
    - "Non-secret source preference restore after enumeration confirms availability"
key-files:
  created:
    - "ui/src/bridge/SourceProtocol.test.ts"
    - "ui/src/components/StandaloneSourceStrip.tsx"
    - "ui/src/components/StandaloneSourceStrip.test.tsx"
    - "plugin/include/LumaScope/Standalone/SourcePreferenceStore.h"
    - "plugin/source/Standalone/SourcePreferenceStore.cpp"
    - "tests/native/SourcePreferenceStoreTests.cpp"
  modified:
    - "plugin/include/LumaScope/HostBridge.h"
    - "plugin/include/LumaScope/PluginEditor.h"
    - "plugin/include/LumaScope/PluginProcessor.h"
    - "plugin/source/HostBridge.cpp"
    - "plugin/source/PluginEditor.cpp"
    - "plugin/source/PluginProcessor.cpp"
    - "plugin/source/Standalone/StandaloneSourceController.cpp"
    - "ui/src/bridge/protocol.ts"
    - "ui/src/bridge/BridgeProvider.tsx"
    - "ui/src/app/AppShell.tsx"
    - "tests/native/CMakeLists.txt"
    - "docs/standalone/device-test-matrix.md"
    - "README.md"
key-decisions:
  - "Gate source controls from typed hostMode so VST3 never renders standalone capture UI."
  - "Keep source.select/source.stop dispatch on the message thread and outside capture callbacks."
  - "Persist only successful user-selected source activations, not failed restores or fallback guesses."
  - "Restore a saved source only if it is still present after enumeration; otherwise start stopped and ask the user to choose."
  - "Record the Windows device matrix as a documentation template pending real-device evidence."
patterns-established:
  - "Bridge provider stores source list/state alongside analyzer snapshots without replacing spectrum data."
  - "Standalone source UI uses a compact MUI strip above the analyzer stage."
  - "Native source preferences are non-secret app-data JSON with schema/version validation."
requirements-completed:
  - "CAP-01"
  - "CAP-02"
  - "CAP-03"
  - "CAP-04"
  - "CAP-05"
  - "UI-04"
duration: "unknown (backfilled)"
completed: "2026-06-23"
---

# Phase 3 Plan 03: Source Controls, Preferences, and Diagnostics Summary

**Standalone-only React source controls, typed source request protocol, native preference restore, and diagnostic documentation for input and WASAPI loopback monitoring**

## Performance

- **Duration:** Unknown; summary backfilled from commit history on 2026-06-23
- **Started:** Not recorded
- **Completed:** 2026-06-23
- **Tasks:** 3 planned tasks; 2 implementation commits plus 1 docs/state follow-up found
- **Files modified:** 24 planned files; implementation evidence spans native, UI, tests, docs, and planning state

## Accomplishments

- Added closed protocol-v1 TypeScript source parsing and request helpers for `source.list`, `source.state`, `source.select`, and `source.stop`.
- Wired native `source.select` and `source.stop` request handling through `PluginEditor`/`HostBridge`, with standalone-only dispatch and bounded VST3 unsupported-mode errors.
- Built `StandaloneSourceStrip` as a compact Material UI control strip above the analyzer, rendering only when `host.info.hostMode` is `Standalone`.
- Added source list/state storage to `BridgeProvider` without discarding existing analyzer spectrum snapshots.
- Added `SourcePreferenceStore` and controller restore behavior for last-valid source selection, including no automatic fallback when the saved source is unavailable.
- Added UI/native tests covering source protocol parsing, source strip rendering/statuses, VST3 omission, source selection, stopping, and preference persistence.
- Added standalone documentation including startup, diagnostics, source preference setup, and a device test matrix template.

## Task Commits

1. **Task 1: Add typed source protocol state and request handling** - `b0b59a8` (`feat(03-03): typed source protocol, provider state, and native request dispatch`)
2. **Task 2: Build standalone source strip and preference restore** - `ad15ba3` (`feat(03-03): standalone source strip UI, preference persistence, and AppShell integration`)
3. **Task 3: Diagnostics, scripts, docs, and real Windows device matrix** - partially represented by `f9a2a51` (`docs: update README and state for Phase 3 completion`)

## Files Created/Modified

- `ui/src/bridge/protocol.ts` - Defines source event IDs, payload types, parsers, and request message shape.
- `ui/src/bridge/BridgeProvider.tsx` - Stores latest source list/state and exposes source select/stop request helpers.
- `ui/src/bridge/SourceProtocol.test.ts` - Covers source protocol parsing and malformed payload rejection.
- `ui/src/components/StandaloneSourceStrip.tsx` - Renders standalone-only mode/source/status controls.
- `ui/src/components/StandaloneSourceStrip.test.tsx` - Covers standalone rendering, VST3 omission, source states, and request interactions.
- `ui/src/app/AppShell.tsx` - Places the source strip above the analyzer stage for standalone hosts.
- `plugin/source/PluginEditor.cpp` - Handles source request messages on the message thread and emits source list/state updates.
- `plugin/source/HostBridge.cpp` - Adds source select/stop event identifiers and request/payload support.
- `plugin/source/Standalone/SourcePreferenceStore.cpp` - Persists and restores non-secret source selections.
- `plugin/source/Standalone/StandaloneSourceController.cpp` - Integrates preference restore and save-on-success behavior.
- `tests/native/SourcePreferenceStoreTests.cpp` - Covers source preference persistence, invalid data, and restore behavior.
- `docs/standalone/device-test-matrix.md` - Provides the manual Windows device coverage template.
- `docs/standalone/startup-guide.md`, `docs/standalone/diagnostics.md`, `docs/standalone/source-preference-setup.md` - Explain standalone monitoring, state diagnostics, and source preference behavior.

## Decisions Made

- Source controls are derived from typed `host.info.hostMode`; the VST3 UI omits the strip rather than rendering disabled controls.
- Source request handlers validate protocol version, mode, request ID, and source ID before native action.
- Source request dispatch stays on the JUCE message side; capture callbacks do not communicate directly with WebView or perform preference I/O.
- Saved source restore is conservative: restore only when enumeration finds the same source, otherwise remain stopped with a choose-source state.
- The checked-in device matrix is currently a reusable template, not completed human evidence.

## Deviations from Plan

### Manual Matrix Gap

The plan called for `.planning/phases/03-standalone-windows-monitoring/03-DEVICE-MATRIX.md` with dated real-device evidence. That file was not created. A reusable matrix template exists at `docs/standalone/device-test-matrix.md`, but it does not contain completed device names, commands, observed results, or pass/fail evidence.

**Impact on plan:** Automated and code-level coverage support Phase 3 behavior, but the real Windows input/render/silence/removal/restart/VST3 omission matrix remains a release-proof gap for Phase 7 or a follow-up verification pass.

## Issues Encountered

- The original summary artifact was missing, which blocked `/gsd-next` prior-phase completeness checks. This file backfills the execution record from commit history and current repo evidence.
- No dated manual device evidence was found in `.planning/phases/03-standalone-windows-monitoring/`; this summary records that limitation explicitly.

## User Setup Required

None for the implemented source UI and preference code. Real-device verification still requires running the built standalone on Windows with available input and render endpoints and recording observed behavior.

## Verification Results

Previously recorded evidence in Phase 3 and subsequent project verification indicates:

- Native and UI source protocol/tests were added in `b0b59a8` and `ad15ba3`.
- `scripts/verify-project.ps1 -SelfTest` passed after the current repository configuration cleanup.
- Manual Windows device matrix evidence is not present and should not be claimed as passed.

## Next Phase Readiness

Phase 5 activation planning can proceed with the standalone source UI and native bridge patterns in place. The main residual Phase 3 concern is human device-matrix evidence, which should be completed before release hardening claims in Phase 7.

---
*Phase: 03-standalone-windows-monitoring*
*Completed: 2026-06-23*
