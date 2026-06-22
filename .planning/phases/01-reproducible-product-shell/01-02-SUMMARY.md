---
phase: 01-reproducible-product-shell
plan: 02
subsystem: ui-shell
tags: [react, material-ui, webview2, accessibility, vitest]
requires:
  - phase: 01-reproducible-product-shell
    plan: 01
    provides: Embedded React/JUCE WebView shell and protocol-v1 bridge
provides:
  - Semantic LumaScope Material UI theme and responsive three-row shell
  - Honest analyzer placeholder with stable Phase 2 renderer mount
  - Typed accessible bridge-state and diagnostic presentations
  - Offline bundle, state, and accessibility regression coverage
affects: [01-03, phase-02-analyzer]
tech-stack:
  added: []
  patterns: [semantic-theme-tokens, typed-state-presentation, bounded-text-diagnostics]
key-files:
  created:
    - ui/src/app/AppShell.tsx
    - ui/src/app/theme.ts
    - ui/src/components/AnalyzerStage.tsx
    - ui/src/components/BridgeStatus.tsx
    - ui/src/test/Accessibility.test.tsx
    - ui/src/test/BridgeStates.test.tsx
  modified:
    - ui/src/App.tsx
    - ui/src/components/StatusFooter.tsx
    - plugin/source/PluginEditor.cpp
key-decisions:
  - "Reserve the central stage as a stable Phase 2 renderer mount while Phase 1 displays only honest readiness copy."
  - "Normalize bridge error codes into five approved presentations and render bounded native diagnostics only as React text."
metrics:
  duration: 46 min
  completed: 2026-06-22
  tasks: 3
  files: 17
---

# Phase 1 Plan 2: LumaScope Visual Shell Summary

Semantic dark instrument-panel shell with typed bridge recovery UX, bounded diagnostics, responsive layout, and accessible offline-safe presentation.

## Performance

- **Duration:** 46 min
- **Started:** 2026-06-22T18:19:17Z
- **Completed:** 2026-06-22T19:05:00Z
- **Tasks:** 3
- **Files modified:** 17

## Accomplishments

- Centralized the approved palette, typography, spacing, radius, elevation, focus, and forced-color behavior in one CSS-variable-enabled MUI theme.
- Built the exact 52px/minmax/32px responsive shell with LumaScope identity, a single honest analyzer stage, quiet footer, and stable future renderer boundary.
- Added deterministic connecting, embedded/vite ready, development-server, protocol-mismatch, and runtime-error presentations with bounded literal diagnostics.
- Verified semantic regions, keyboard actions, polite status announcements, reduced motion, long diagnostic containment, and forbidden remote bundle references.
- Received explicit human approval of the embedded VS2019 Debug standalone at the blocking visual checkpoint.

## Task Commits

1. **Task 1: Implement the semantic LumaScope theme and shell hierarchy** - `53a0db2`
2. **Task 2: Implement typed connection/error UX and accessibility verification** - `f845ddb`
3. **Task 3: Approve the embedded LumaScope visual shell** - Human approval; no code commit

## Files Created/Modified

- `ui/src/app/theme.ts` - Semantic palette, typography, spacing, focus, and component tokens.
- `ui/src/app/AppShell.tsx` - Viewport-filling three-row shell and bridge integration.
- `ui/src/components/AnalyzerStage.tsx` - Accessible honest stage and Phase 2 renderer mount.
- `ui/src/components/BridgeStatus.tsx` - Typed approved state/error copy and retry behavior.
- `ui/src/components/StatusFooter.tsx` - Build source status and diagnostic-copy action.
- `ui/src/styles/spectral-motif.css` - Decorative grid/glow with reduced-motion suppression.
- `ui/src/test/*.test.*` - Shell, bridge-state, accessibility, and offline bundle contracts.
- `plugin/source/PluginEditor.cpp` - Theme-aligned native WebView failure fallback.

## Decisions Made

- Kept all Phase 2 analyzer data, standalone source selection, settings, and licensing behavior outside this shell.
- Limited transport retry to the development-server-unavailable state; protocol and runtime failures provide diagnostics and explicit corrective guidance.
- Used path imports for Material UI components/icons and local system fonts only.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Added Vite client declarations for CSS module compilation**
- **Found during:** Task 1 production build
- **Issue:** TypeScript rejected the spectral motif side-effect CSS import without Vite client declarations.
- **Fix:** Added `ui/src/vite-env.d.ts` and re-ran the focused test/build gate.
- **Files modified:** `ui/src/vite-env.d.ts`
- **Verification:** `npm --prefix ui run build`
- **Commit:** `53a0db2`

**2. [Rule 3 - Blocking] Used the supported Windows PowerShell baseline for quick verification**
- **Found during:** Task 2 verification
- **Issue:** `pwsh.exe` is not installed on the baseline machine.
- **Fix:** Ran the same checked-in quick-test script under Windows PowerShell 5.1; frontend tests and native CTest passed.
- **Files modified:** None
- **Verification:** `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/test-quick.ps1`
- **Commit:** N/A

**Total deviations:** 2 auto-fixed blocking issues. **Impact:** No product scope or architecture change.

## Known Stubs

None. The empty analyzer presentation is intentional Phase 1 behavior with a wired Phase 2 mount boundary.

## Issues Encountered

- The baseline still lacks PowerShell 7; Windows PowerShell 5.1 remains the verified script host.

## User Approval

- Task 3 embedded visual and accessibility checkpoint approved by the user on 2026-06-22.

## Next Phase Readiness

- Plan 01-03 can document and validate the completed build/developer workflow.
- The analyzer stage is ready for Phase 2 snapshot rendering without shell-layout changes.

## Self-Check: PASSED

- Verified all 17 changed source/test files exist.
- Verified task commits `53a0db2` and `f845ddb` exist in git history.
- Verified 17/17 Vitest tests, production UI build, and local-only bundle scan pass after approval.
