---
phase: 01-reproducible-product-shell
plan: 01
subsystem: product-shell
tags: [juce, cmake, webview2, react, typescript, vite, material-ui, vitest]

requires: []
provides:
  - Pinned VS2019 JUCE 8.0.14 VST3 and standalone build targets
  - Deterministic offline React bundle embedded through JUCE BinaryData
  - Path-safe resource provider and typed protocol-v1 native/web handshake
affects: [02-end-to-end-vst3-analyzer, 03-standalone-windows-monitoring, native-web-bridge]

tech-stack:
  added: [JUCE 8.0.14, WebView2 SDK 1.0.4022.49, React 19.2.7, TypeScript 6.0.3, Vite 8.0.16, Material UI 9.1.1, Vitest 4.1.9]
  patterns: [pinned repository-local dependencies, deterministic embedded web archive, allowlisted resource provider, versioned native events]

key-files:
  created: [CMakePresets.json, cmake/WebBundle.cmake, plugin/source/HostBridge.cpp, plugin/source/WebResources.cpp, ui/src/bridge/BridgeProvider.tsx, scripts/test-all.ps1]
  modified: [CMakeLists.txt, plugin/CMakeLists.txt, plugin/source/PluginEditor.cpp, tests/native/CMakeLists.txt]

key-decisions:
  - "Use the pinned WebView2 NuGet layout expected by JUCE and enable static WebView2 loading explicitly."
  - "Generate web ZIPs with sorted paths and fixed entry times so identical frontend inputs produce identical archive bytes."
  - "Use JUCE native events for ui.ready/host.info; do not evaluate string-built JavaScript."

patterns-established:
  - "Build graph: npm lock inputs gate npm ci; UI sources gate Vite build/archive; archive gates BinaryData and both product formats."
  - "Boundary validation: reject unsafe resource paths and malformed or unsupported protocol payloads before serving/responding."

requirements-completed: [BUILD-01, BUILD-02, BUILD-03, UI-01, UI-03]

duration: 30min
completed: 2026-06-22
---

# Phase 1 Plan 1: Reproducible Product Shell Summary

**Pinned VS2019/JUCE products with deterministic embedded React assets and a tested protocol-v1 WebView2 handshake**

## Performance

- **Duration:** 30 min
- **Started:** 2026-06-22T17:41:00Z
- **Completed:** 2026-06-22T18:11:29Z
- **Tasks:** 3
- **Files modified:** 45

## Accomplishments

- Proved JUCE 8.0.14 and WebView2 SDK 1.0.4022.49 compile under VS2019 16.11 into LumaScope Standalone and VST3 artifacts.
- Added an exact-version React/TypeScript/Material UI workspace whose offline Vite output is reproducibly archived and embedded before native linking.
- Implemented and tested path-safe resource serving plus the validated `ui.ready`/`host.info` protocol-v1 handshake; the real Debug standalone smoke reported embedded bridge readiness.

## Task Commits

Each task was committed atomically:

1. **Task 1: Prove the locked Windows toolchain and scaffold JUCE targets** - `293dcec` (chore)
2. **Task 2: Build and embed the minimal typed React shell** - `cb276e1` (feat)
3. **Task 3: Complete the protocol-v1 native/web handshake** - `03b430f` (feat)

## Files Created/Modified

- `CMakePresets.json` - Explicit VS2019 and Ninja Debug/Release configure, build, and test presets.
- `cmake/Dependencies.cmake` - Exact JUCE and WebView2 dependency resolution in an ignored local cache.
- `cmake/WebBundle.cmake` - Incremental npm/Vite/archive/BinaryData dependency graph.
- `plugin/source/WebResources.cpp` - Traversal-resistant archive lookup with MIME allowlisting and bounded reads.
- `plugin/source/HostBridge.cpp` - Protocol-v1 validation and bounded host/error envelopes.
- `plugin/source/PluginEditor.cpp` - WebView2, embedded resource provider, native event wiring, and Debug smoke hook.
- `ui/src/bridge/BridgeProvider.tsx` - Typed JUCE transport adapter and connecting/ready/error state.
- `tests/fixtures/bridge/` - Shared valid, unsupported, and malformed protocol fixtures.
- `scripts/test-quick.ps1` / `scripts/test-all.ps1` - Non-watch warm and full verification entry points.

## Decisions Made

- Kept embedded resources authoritative and allowed native integration only on JUCE's resource-provider origin.
- Used a deterministic .NET ZIP writer because CMake's ZIP output changed bytes across identical builds despite fixed entry mtimes.
- Used Windows PowerShell 5.1 as the local command host because PowerShell 7 (`pwsh`) is not installed; scripts remain PowerShell-compatible and behavior is unchanged.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Corrected JUCE WebView2 package discovery**
- **Found during:** Task 1
- **Issue:** JUCE expects `JUCE_WEBVIEW2_PACKAGE_LOCATION` to contain a NuGet-style `Microsoft.Web.WebView2.<version>` directory.
- **Fix:** Corrected the cache variable and extracted package layout; added a valid bundle identifier.
- **Files modified:** `cmake/Dependencies.cmake`, `plugin/CMakeLists.txt`
- **Verification:** VS2019 configure found WebView2 1.0.4022.49 and both products linked.
- **Committed in:** `293dcec`

**2. [Rule 3 - Blocking] Used the available Windows PowerShell host**
- **Found during:** Task 1
- **Issue:** The baseline machine does not have `pwsh` on PATH.
- **Fix:** Ran repository scripts with `powershell.exe -NoProfile`; scripts avoid PowerShell 7-only syntax.
- **Files modified:** None
- **Verification:** Environment JSON and quick/full scripts exited 0 under Windows PowerShell 5.1.
- **Committed in:** N/A (execution-host adjustment)

**3. [Rule 1 - Bug] Made the embedded archive byte-for-byte deterministic**
- **Found during:** Task 2
- **Issue:** CMake ZIP archives changed hashes across identical Vite builds.
- **Fix:** Added a sorted-path ZIP writer with fixed per-entry timestamps.
- **Files modified:** `scripts/New-DeterministicZip.ps1`, `cmake/WebBundle.cmake`
- **Verification:** Two forced rebuilds produced SHA-256 `609FEAFC0BDD9A049F9D626C1DA74B2E2294E10B6982A44F032C208E4E61517C`.
- **Committed in:** `cb276e1`

**4. [Rule 3 - Blocking] Enabled JUCE's static WebView2 feature macro**
- **Found during:** Task 3
- **Issue:** The resource-provider API is compiled out on Windows unless JUCE WebView2 support is enabled explicitly.
- **Fix:** Added `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` to shared-code definitions.
- **Files modified:** `plugin/CMakeLists.txt`
- **Verification:** Plugin editor compiled with `withResourceProvider`, and standalone smoke completed the handshake.
- **Committed in:** `03b430f`

---

**Total deviations:** 4 auto-fixed (1 bug, 3 blocking issues).  
**Impact on plan:** All changes were required for deterministic output or successful execution on the locked machine; no later-phase behavior was added.

## Issues Encountered

- The first npm prefix invocation resolved from the repository root under the PowerShell shim; verification uses `npm.cmd --prefix ui`, which is stable on this machine.
- Initial test/build iterations exposed missing Vitest imports, a JUCE instance API call, BinaryData symbol mangling, and test cleanup/linkage errors; each was corrected before task commits.

## Known Stubs

None. The analyzer stage intentionally contains honest Phase 1 ready-state copy; live spectrum data belongs to Phase 2 and does not block this plan's goal.

## Verification Evidence

- Environment JSON: Windows 10 build 19045 x64, VS2019 16.11.37327.17 C++ workload, CMake 4.0.3, Node 22.18.0, npm 10.9.3, Ninja 1.13.1, WebView2 149.0.4022.80.
- `scripts/test-quick.ps1`: 2 Vitest files / 3 tests and native CTest passed.
- `scripts/test-all.ps1`: npm clean install, frontend tests/build/offline check, VS2019 Standalone/VST3/native-test build, and CTest passed.
- Debug standalone smoke: `{"status":"ready","protocolVersion":1,"uiSource":"embedded"}`.
- Threat controls: dependency versions pinned; hostile resource paths/MIME types tested; malformed/unsupported bridge fixtures rejected in native and browser suites.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- The shared product shell and protocol boundary are ready for Phase 2 analyzer snapshots without changing the audio callback or asset-delivery architecture.
- PowerShell 7 remains absent; current scripts work under Windows PowerShell 5.1.

## Self-Check: PASSED

- All key files exist.
- Task commits `293dcec`, `cb276e1`, and `03b430f` exist.
- All task acceptance criteria and plan-level verification commands passed.

---
*Phase: 01-reproducible-product-shell*
*Completed: 2026-06-22*
