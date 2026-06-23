---
phase: 01-reproducible-product-shell
plan: 03
subsystem: developer-workflow
tags: [juce, webview2, vite, context7, cmake, diagnostics]
requires:
  - phase: 01-reproducible-product-shell
    plan: 01
    provides: Pinned JUCE/CMake native targets, presets, WebView2 SDK cache, and native test harness
  - phase: 01-reproducible-product-shell
    plan: 02
    provides: React/TypeScript/Material UI shell, deterministic embedded bundle, and protocol-v1 bridge UX
provides:
  - Safe explicit embedded and Vite WebView modes
  - Native-owned diagnostics for WebView2, packaged-resource, development-server, and handshake failures
  - Reproducible bootstrap, full verification, and project-scoped Context7 guidance
  - Documented bridge protocol, troubleshooting, and developer workflow
affects: [phase-02-analyzer, phase-07-release-handoff, developer-onboarding]
tech-stack:
  added: [context7-mcp]
  patterns: [debug-only-dev-server, native-fallback-smoke-json, single-context7-server, repository-self-verification]
key-files:
  created:
    - .codex/config.toml
    - scripts/bootstrap.ps1
    - scripts/test-web-modes.ps1
    - scripts/verify-project.ps1
    - README.md
    - docs/development.md
    - docs/bridge-protocol.md
    - docs/troubleshooting.md
  modified:
    - CMakeLists.txt
    - CMakePresets.json
    - plugin/CMakeLists.txt
    - plugin/include/LumaScope/HostBridge.h
    - plugin/include/LumaScope/PluginEditor.h
    - plugin/source/HostBridge.cpp
    - plugin/source/PluginEditor.cpp
    - scripts/check-environment.ps1
    - scripts/test-all.ps1
    - AGENTS.md
    - ui/src/bridge/protocol.ts
key-decisions:
  - "Use http://127.0.0.1:5174 as the canonical LumaScope Vite development URL because 5173 was already owned by unrelated bluetti-monitor/PID 13256 and the user approved leaving that process untouched."
  - "Validate development-server origins at root CMake configure time so unsafe Debug or Release values fail before native target generation."
  - "Keep native failure simulation and smoke-result diagnostics Debug-only so Release builds cannot expose test hooks."
  - "Configure one optional project Context7 MCP server and route documentation lookups through the three approved library IDs."
patterns-established:
  - "Vite mode is opt-in through an explicit Debug preset and strict origin validation; embedded mode remains the default and Release-safe path."
  - "Native-owned diagnostics cover failures that can occur before React loads; React owns protocol/runtime presentation only after bundle startup."
  - "Repository verification includes negative probes for tutorial identities, extra Context7 servers, remote assets, private keys, and Release dev-server leakage."
requirements-completed: [BUILD-01, BUILD-02, BUILD-04, BUILD-05, UI-03]
metrics:
  duration: 6h 35m wall-clock including blocking human checkpoint
  completed: 2026-06-23
  tasks: 3
  files: 24
---

# Phase 1 Plan 3: Reproducible Developer Workflow Summary

Safe embedded and Vite WebView workflows with native diagnostics, one-command bootstrap/full verification, and project-scoped Context7 guidance.

## Performance

- **Duration:** 6h 35m wall-clock including the blocking human checkpoint
- **Started:** 2026-06-22T19:06:24Z
- **Completed:** 2026-06-23T01:41:36Z
- **Tasks:** 3
- **Files modified:** 24

## Accomplishments

- Added explicit embedded and Vite WebView modes with strict Debug-only validation for the canonical LumaScope development URL, `http://127.0.0.1:5174`.
- Implemented native fallback/smoke diagnostics for unavailable Vite, unavailable WebView2, packaged-resource failure, and handshake timeout cases.
- Added reproducible setup and verification entry points covering preflight, npm install/test/build, native configure/build/test, web-mode smokes, self-verification, and negative probes.
- Configured one project Context7 MCP server and documented the exact library IDs `/websites/juce_master`, `/janwilczek/juce-webview-tutorial`, and `/websites/mui_material-ui`.
- Documented quickstart, development modes, bridge protocol v1, and troubleshooting/recovery paths for WebView2, assets, Vite, and toolchain issues.
- Received explicit human approval of the fresh-terminal, embedded, Vite, unavailable-server, fallback, and Context7 checkpoint.

## Task Commits

1. **Task 1: Add explicit Vite mode and native-owned diagnostics** - `c175283`, `57807fd`, `9ca04ae`
2. **Task 2: Complete reproducible setup, Context7 guidance, and project verification** - `6e18267`
3. **Task 3: Approve the complete embedded and Vite developer workflow** - Human approval on 2026-06-23; no code commit

**Plan metadata:** recorded in the final close-out commit.

## Files Created/Modified

- `.codex/config.toml` - One optional Context7 MCP server at `https://mcp.context7.com/mcp`.
- `CMakeLists.txt`, `CMakePresets.json`, `plugin/CMakeLists.txt` - Debug-only Vite preset/configuration, Release guard, and strict dev-server validation.
- `plugin/include/LumaScope/PluginEditor.h`, `plugin/source/PluginEditor.cpp` - WebView mode selection, origin validation, native fallback pages, and smoke-result output.
- `plugin/include/LumaScope/HostBridge.h`, `plugin/source/HostBridge.cpp` - Host info includes UI source and build marker.
- `scripts/bootstrap.ps1` - Fresh-checkout bootstrap/preflight/configure/build entry point.
- `scripts/check-environment.ps1` - Separate diagnostics for Visual Studio, developer shell, Ninja, WebView2 SDK cache, and WebView2 runtime.
- `scripts/test-web-modes.ps1` - Embedded/Vite/unavailable/fallback smoke orchestration and hostile URL checks without touching unrelated processes.
- `scripts/test-all.ps1` - Full repository verification wrapper.
- `scripts/verify-project.ps1` - Artifact/config/Context7/secret/tutorial-identity/release-dev-server verification with negative probes.
- `README.md`, `docs/development.md`, `docs/bridge-protocol.md`, `docs/troubleshooting.md`, `AGENTS.md` - Developer workflow, protocol, troubleshooting, and Context7 guidance.
- `ui/src/bridge/protocol.ts`, `tests/fixtures/bridge/host-info-v1.json`, `tests/native/HostBridgeTests.cpp`, `ui/src/test/*.test.*` - Protocol/build-marker test updates.
- `tests/native/SmokeTests.cpp` - Removed unused native smoke-test stub.

## Decisions Made

- **Canonical Vite URL:** `http://127.0.0.1:5174` is the approved LumaScope development URL for this plan. Port `5173` was occupied by unrelated `bluetti-monitor`/PID 13256, and the user explicitly approved not terminating or altering that process.
- **Strict native allowlist:** Development-server mode accepts only `http`, host `127.0.0.1`, port `5174`, and root path with no credentials, query, or fragment.
- **Release boundary:** Release configuration rejects any non-empty development-server URL and diagnostic simulation hooks are Debug-only.
- **Context7 shape:** The repository uses exactly one optional Context7 MCP server and records the three approved library IDs in AGENTS.md and development docs.

## Deviations from Plan

### User-approved Plan Deviation

**1. Canonical Vite port changed from 5173 to 5174**
- **Found during:** Task 1 startup/port verification
- **Issue:** `127.0.0.1:5173` was already owned by unrelated `bluetti-monitor`/PID 13256.
- **User decision:** Do not terminate or alter that process. Use `http://127.0.0.1:5174` after verifying it was available.
- **Files modified:** `CMakeLists.txt`, `CMakePresets.json`, `plugin/CMakeLists.txt`, `plugin/source/PluginEditor.cpp`, `scripts/test-web-modes.ps1`, `README.md`, `docs/development.md`, `docs/troubleshooting.md`
- **Verification:** Web-mode smoke tests used port 5174 consistently; port 5174 was free after smoke cleanup.
- **Commit:** `c175283`, `57807fd`, `6e18267`

### Auto-fixed Issues

**2. [Rule 3 - Blocking] Move unsafe URL validation earlier than the original plugin-only location**
- **Found during:** Task 1 hostile configure cases
- **Issue:** Validating only inside `plugin/CMakeLists.txt` let unsafe values survive too far into configuration.
- **Fix:** Added root configure-time validation in `CMakeLists.txt` so invalid URLs and Release dev-server values fail early.
- **Files modified:** `CMakeLists.txt`, `plugin/CMakeLists.txt`
- **Verification:** `scripts/test-web-modes.ps1` rejects localhost, remote hosts, HTTPS, old port 5173, alternate ports, credentials, paths, queries, fragments, malformed URLs, and Release dev-server configuration.
- **Commit:** `57807fd`

**3. [Rule 1 - Bug] Restrict diagnostic simulation hooks to Debug**
- **Found during:** Task 1 release verification
- **Issue:** The diagnostic simulation environment hook behavior and documentation were intended for Debug smoke testing only.
- **Fix:** Wrapped simulation behavior in Debug-only native definitions.
- **Files modified:** `plugin/CMakeLists.txt`, `plugin/source/PluginEditor.cpp`
- **Verification:** Release configure/build completed without dev-server or diagnostic-hook leakage.
- **Commit:** `9ca04ae`

**4. [Rule 3 - Blocking] Use Windows PowerShell 5.1 for repository automation on the baseline machine**
- **Found during:** Task 1 and Task 2 verification
- **Issue:** `pwsh.exe` is not installed on the baseline machine; some generated MSBuild helper output still mentions it, but builds exit successfully.
- **Fix:** Ran checked-in automation with `powershell.exe -NoProfile -ExecutionPolicy Bypass`; scripts themselves remain compatible with Windows PowerShell.
- **Files modified:** Documentation and scripts use Windows PowerShell-compatible commands.
- **Verification:** `scripts/bootstrap.ps1`, `scripts/test-web-modes.ps1`, `scripts/test-all.ps1`, and `scripts/verify-project.ps1 -SelfTest` all passed.
- **Commit:** `6e18267`

---

**Total deviations:** 1 user-approved plan deviation and 3 auto-fixed issues (2 blocking, 1 bug). **Impact on plan:** The approved 5174 URL preserves unrelated local processes while satisfying the same loopback-only security boundary. Auto-fixes tightened validation and Release safety without adding future analyzer or licensing scope.

## Verification Evidence

- `npm.cmd --prefix ui run test:run` passed: 6 files, 17 tests.
- `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/test-web-modes.ps1` passed:
  - Vite ready smoke JSON: `status=ready`, `protocolVersion=1`, `uiSource=vite`
  - Embedded ready smoke JSON: `status=ready`, `protocolVersion=1`, `uiSource=embedded`
  - Unavailable Vite smoke JSON: `status=error`, `errorCode=development_server_unavailable`
  - Simulated fallback JSON for `webview2_unavailable`, `embedded_resource_unavailable`, and `handshake_timeout`
- `cmake --preset vs2019-release` passed.
- `cmake --build --preset vs2019-release --target LumaScope_Standalone LumaScope_VST3 --parallel 4` passed.
- `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/bootstrap.ps1 -Preset vs2019-debug` passed.
- `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/test-all.ps1` passed, including UI tests/build, native Debug Standalone/VST3/tests, CTest, web-mode smokes, and verifier self-test.
- `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/verify-project.ps1 -SelfTest` passed, proving all five negative probes fail as expected.

## Human Checkpoint Approval

- Task 3 checkpoint was presented with fresh-terminal bootstrap, embedded launch, Vite hot reload, unavailable-server, native fallback, and Context7 verification steps.
- User approved the checkpoint on 2026-06-23.
- No final SUMMARY or state/roadmap/requirements updates were created before approval.

## Issues Encountered

- PowerShell 7 is not installed on the baseline machine. Repository scripts pass under Windows PowerShell 5.1; generated MSBuild helper output can mention missing `pwsh.exe` while successful builds still exit 0.
- Port `5173` remains owned by unrelated `bluetti-monitor`/PID 13256 and was intentionally not touched.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Phase 1 is complete: a developer can build, test, launch, hot-reload, diagnose, and inspect the product shell from repository instructions.
- Phase 2 can build on the stable protocol-v1 bridge and shell mount without changing the approved embedded/Vite workflow.
- Remaining release-packaging details, including WebView2 runtime distribution policy, stay deferred to later release/handoff work.

## Self-Check: PASSED

- Verified the working tree was clean before close-out.
- Verified Task 1 and Task 2 commits exist in git history.
- Verified Task 3 approval was explicit before creating this summary.
- Verified no SUMMARY was created before the blocking checkpoint was approved.
- Verified the approved canonical Vite URL is `http://127.0.0.1:5174` and no instruction required touching `bluetti-monitor`/PID 13256.
