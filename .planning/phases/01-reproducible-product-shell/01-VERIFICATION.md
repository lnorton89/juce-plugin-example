---
phase: 01-reproducible-product-shell
verified: 2026-06-23T01:52:37Z
status: passed
score: 18/18 must-haves verified
overrides_applied: 0
approved_deviations:
  - must_have: "Vite development mode uses the approved canonical loopback URL"
    reason: "Port 5173 was owned by unrelated bluetti-monitor / PID 13256; approved canonical LumaScope Vite URL is http://127.0.0.1:5174 and verifier did not touch that process."
    accepted_by: "user"
    accepted_at: "2026-06-23T01:41:36Z"
warnings:
  - "Phase is marked mode:mvp, but roadmap goal is not in canonical user-story format. Product success criteria were verified directly."
  - "VS2019 build output still prints a non-fatal pwsh.exe missing line from generated helper output; test-all.ps1 exits 0 under Windows PowerShell 5.1."
---

# Phase 1: Reproducible Product Shell Verification Report

**Phase Goal:** A developer can build the VST3 and standalone applications from a clean checkout and see the same packaged React/MUI interface in both.  
**Verified:** 2026-06-23T01:52:37Z  
**Status:** passed  
**Re-verification:** No - initial verification

## Goal Achievement

Phase 1 is genuinely complete against the roadmap contract. The codebase builds both JUCE targets from pinned dependencies, embeds the React/TypeScript/MUI bundle by default, supports explicit Vite mode at the approved `http://127.0.0.1:5174` loopback URL, validates a typed protocol-v1 bridge, and records one Context7 MCP endpoint with the three required library IDs.

## User Flow Coverage

`mode: mvp` is present in ROADMAP.md, but the phase goal is not a canonical `As a..., I want..., so that...` user story. I verified the observable developer flow implied by the goal and roadmap success criteria.

| Step | Expected | Evidence | Status |
| --- | --- | --- | --- |
| Configure clean Windows build | Explicit CMake presets use VS2019 x64 and pinned dependency cache | `CMakePresets.json` defines `vs2019-debug`, `vs2019-release`, `ninja-debug`, `ninja-release`; `scripts/check-environment.ps1 -Json` reported Windows 10 x64, VS2019 16.11, CMake, Node/npm, Ninja, WebView2 SDK/runtime | VERIFIED |
| Build both products | Standalone EXE and VST3 binary are produced | `scripts/test-all.ps1` built `LumaScope_Standalone`, `LumaScope_VST3`, and `LumaScopeNativeTests`; artifacts exist under `build/vs2019-debug/plugin/LumaScope_artefacts/Debug/` | VERIFIED |
| Open packaged shell | Embedded standalone smoke reaches bridge ready without Vite | `scripts/test-all.ps1` reported `PASS embedded-ready: {"status":"ready","protocolVersion":1,"uiSource":"embedded"}` | VERIFIED |
| Use dev mode | Vite mode is explicit, fixed to approved loopback, and protocol-compatible | `scripts/test-all.ps1` reported `PASS vite-ready` on `http://127.0.0.1:5174`; hostile URL and Release-dev-server cases are rejected | VERIFIED |
| Use project guidance | Context7 has one server and three library IDs | `.codex/config.toml`, `AGENTS.md`, `docs/development.md`, and `scripts/verify-project.ps1 -SelfTest` verify this | VERIFIED |

## Observable Truths

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Developer can run documented CMake presets on clean Windows and produce VST3 and standalone binaries with pinned dependencies. | VERIFIED | `CMakeLists.txt` pins C++20; `cmake/Dependencies.cmake` pins JUCE `8.0.14` and WebView2 SDK `1.0.4022.49`; `test-all.ps1` built both formats successfully. |
| 2 | Both binaries open the packaged React/TypeScript Material UI shell without a development server. | VERIFIED | Embedded mode is default; `PluginEditor.cpp` navigates to `getResourceProviderRoot()` when not in Vite mode; embedded smoke returned `status=ready`, `uiSource=embedded`. |
| 3 | Frontend development mode reloads rapidly while release mode uses embedded assets through a versioned typed bridge. | VERIFIED | `vs2019-vite` uses only `http://127.0.0.1:5174`; Release rejects any dev URL; bridge emits `ui.ready` and validates `host.info` protocol version 1 on native and React sides. |
| 4 | Project guidance configures one Context7 MCP endpoint and identifies all three requested documentation libraries. | VERIFIED | `.codex/config.toml` contains one `[mcp_servers.context7]`; `AGENTS.md` and `docs/development.md` list `/websites/juce_master`, `/janwilczek/juce-webview-tutorial`, and `/websites/mui_material-ui`; verifier self-test passed. |
| 5 | Product identity is LumaScope by Signal Foundry Audio with JUCE codes `SgFd` and `LmSc`. | VERIFIED | `plugin/CMakeLists.txt` contains company/product metadata and manufacturer/plugin codes. |
| 6 | Locked Windows 10 x64, VS2019, and Evergreen WebView2 prerequisites are checked without silently selecting VS2022. | VERIFIED | `check-environment.ps1 -Json` reports VS2019 16.11 and errors if VS2019 C++ workload is missing; presets specify `Visual Studio 16 2019`. |
| 7 | Clean checkout has PowerShell preflight, npm lockfile, embedded-default build, and explicit VS2019/Ninja presets. | VERIFIED | `scripts/bootstrap.ps1`, `ui/package-lock.json`, embedded default cache variable, and presets all exist and are exercised by `test-all.ps1`. |
| 8 | Standalone Debug loads embedded React shell and transitions from connecting to ready after protocol-v1 handshake. | VERIFIED | `BridgeProvider.tsx` emits `ui.ready`; `HostBridge.cpp` returns `host.info`; UI and native tests use fixtures; embedded smoke returns ready. |
| 9 | VST3 and Standalone artifacts are produced from the same shared LumaScope target. | VERIFIED | `juce_add_plugin(LumaScope ... FORMATS VST3 Standalone)` plus shared `target_sources`; both artifacts built in the same preset. |
| 10 | LumaScope presents the approved dark instrument-panel shell without later-phase controls or fake spectrum data. | VERIFIED | `AppShell`, `AnalyzerStage`, `BridgeStatus`, and tests assert shell regions/copy; anti-pattern scan found no fake source/settings/license controls in source. Human visual checkpoint recorded approved. |
| 11 | At default/minimum sizes, connecting, ready, and typed error states remain readable without essential overlap. | VERIFIED | RTL tests cover semantic states and long diagnostics; native `setResizeLimits(560, 360, 1920, 1200)` enforces minimum. Human visual checkpoint recorded approved. |
| 12 | Production UI contains no network font, CDN, analytics, or remote image dependency. | VERIFIED | `npm run check:bundle` passed; `BundleContract.test.ts` and `verify-project.ps1` scan for forbidden URLs/assets. |
| 13 | Keyboard focus, reduced motion, status text, and semantic regions meet UI-SPEC. | VERIFIED | `Accessibility.test.tsx` passed; source uses semantic section `aria-label="Spectrum display"`, MUI buttons, and reduced-motion CSS. |
| 14 | Vite hot reload is opt-in, fixed to loopback, visibly labeled, protocol-compatible, and reversible without source edits. | VERIFIED | `CMakePresets.json` has `vs2019-vite`; native and CMake guards accept only `127.0.0.1:5174`; `StatusFooter` labels Vite vs embedded; smokes prove both modes. |
| 15 | New developer can run one bootstrap/check entry point and explicit documented CMake/npm commands. | VERIFIED | README and `docs/development.md` document preflight, bootstrap, tests, embedded launch, and Vite launch; `bootstrap.ps1` invokes preflight, npm ci, configure, and build. |
| 16 | One project `.codex/config.toml` configures Context7; guidance records all three library IDs. | VERIFIED | Code and docs verified by `verify-project.ps1 -SelfTest`. |
| 17 | Missing Vite server, protocol mismatch, packaged resource failure, and WebView2/runtime failure produce diagnostics instead of blank editor. | VERIFIED | `PluginEditor.cpp` owns native fallbacks; `BridgeStatus.tsx` owns typed React errors; `test-web-modes.ps1` passed unavailable-server, WebView2, resource, and handshake-timeout simulations. |
| 18 | Final verification script proves both native formats, frontend/native tests, embedded bundle provenance, and forbidden URL/secret checks. | VERIFIED | `scripts/test-all.ps1` chains npm ci, Vitest, UI build, bundle check, CMake configure/build, CTest, web-mode smokes, and `verify-project.ps1 -SelfTest`; it exited 0. |

**Score:** 18/18 truths verified

## Required Artifacts

| Artifact | Expected | Status | Details |
| --- | --- | --- | --- |
| `CMakePresets.json` | Explicit VS2019 and Ninja configure/build/test presets | VERIFIED | Contains `Visual Studio 16 2019`, `x64`, Debug/Release, and `vs2019-vite` with approved `5174` URL. |
| `cmake/Dependencies.cmake` | Pinned JUCE/WebView2 dependencies | VERIFIED | JUCE `8.0.14`; WebView2 SDK `1.0.4022.49`; repository-local `.deps` cache. |
| `cmake/WebBundle.cmake` | Deterministic embedded web archive and BinaryData target | VERIFIED | Runs `npm ci` on lock/package changes, builds Vite, creates deterministic ZIP, feeds `juce_add_binary_data`. |
| `plugin/CMakeLists.txt` | JUCE VST3/Standalone with WebView2 and embedded assets | VERIFIED | `juce_add_plugin(LumaScope ...)`, `FORMATS VST3 Standalone`, WebView flags, shared target dependency on `LumaScopeWebBundle`. |
| `plugin/source/PluginEditor.cpp` | WebView2 runtime, resource provider, Vite/embedded selection, smoke/fallback hooks | VERIFIED | Uses WebView2 backend, native integration, resource provider, canonical URL validation, fallbacks, Debug-only smoke output. |
| `plugin/source/HostBridge.cpp` | Protocol-v1 native handshake implementation | VERIFIED | Validates closed `ui.ready` schema, emits bounded `host.info`, typed error envelope. |
| `plugin/source/WebResources.cpp` | Path-safe embedded resource serving | VERIFIED | Normalizes paths, strips query/fragment, rejects traversal/absolute/unknown MIME/missing files. |
| `ui/package-lock.json` | Locked frontend dependency graph | VERIFIED | Present; `npm ci` passed during `test-all.ps1`. |
| `ui/src/bridge/BridgeProvider.tsx` | Typed browser bridge adapter and connection state | VERIFIED | Registers `host.info`/`bridge.error`, emits `ui.ready`, updates React status. |
| `ui/src/app/AppShell.tsx` | Three-row product shell | VERIFIED | SDK artifact check flagged 36 lines vs plan hint of 40, but source is substantive and wired to bridge, header, analyzer stage, and footer. False-positive line-count warning only. |
| `ui/src/components/AnalyzerStage.tsx` | Stable Phase 2 renderer mount and honest Phase 1 state | VERIFIED | Region named `Spectrum display`; `spectrum-renderer-mount`; no fabricated spectrum values. |
| `.codex/config.toml` | One Context7 MCP endpoint | VERIFIED | Single `[mcp_servers.context7]` with `https://mcp.context7.com/mcp`. |
| `scripts/bootstrap.ps1` | Reproducible setup entry point | VERIFIED | Runs preflight, npm ci, configure, and selected build without installing system software. |
| `scripts/test-web-modes.ps1` | Embedded/Vite/fallback smoke orchestration | VERIFIED | Builds and launches owned processes only; checks 5174 availability; stops only owned Vite child. |
| `scripts/verify-project.ps1` | Artifact/config/secret/remote asset self-verifier | VERIFIED | Passed normal mode and five seeded negative probes. |
| `README.md`, `docs/development.md`, `docs/bridge-protocol.md`, `docs/troubleshooting.md` | Developer workflow, protocol, and diagnostics docs | VERIFIED | Commands and protocol fields documented; Context7 IDs present. |

## Key Link Verification

| From | To | Via | Status | Details |
| --- | --- | --- | --- | --- |
| `ui/src/bridge/BridgeProvider.tsx` | `plugin/source/HostBridge.cpp` | `ui.ready` / `host.info` protocol-v1 fixture contract | VERIFIED | SDK literal-pattern check missed escaped constants, but manual trace found constants in both files and fixture tests in native and Vitest. |
| `plugin/source/PluginEditor.cpp` | `plugin/source/WebResources.cpp` | WebBrowserComponent ResourceProvider | VERIFIED | `PluginEditor.cpp` calls `withResourceProvider` and navigates embedded mode to JUCE resource-provider root. |
| `cmake/WebBundle.cmake` | `plugin/CMakeLists.txt` | Generated web archive and BinaryData dependency | VERIFIED | `juce_add_binary_data(LumaScopeWebBundle)` is linked and depended on by `LumaScope`. |
| `ui/src/app/AppShell.tsx` | `ui/src/bridge/BridgeProvider.tsx` | Bridge state selects status and recovery presentation | VERIFIED | `AppShell` uses `useBridgeStatus()` and passes bridge state to `AnalyzerStage` and `StatusFooter`. |
| `ui/src/App.tsx` | `ui/src/app/theme.ts` | ThemeProvider and CssBaseline | VERIFIED | React app wraps shell in MUI theme and baseline. |
| `CMakePresets.json` | `plugin/source/PluginEditor.cpp` | `LUMASCOPE_WEBVIEW_DEV_SERVER` cache variable | VERIFIED | Preset value compiles into Debug only; native validates canonical URL and selects `uiSource`. |
| `.codex/config.toml` | `AGENTS.md` | Context7 server plus library-ID guidance | VERIFIED | Both files contain the required Context7 endpoint/library routing. |
| `scripts/bootstrap.ps1` | `scripts/check-environment.ps1` | Preflight before dependency/configure operations | VERIFIED | Bootstrap invokes preflight before npm/CMake work. |

## Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
| --- | --- | --- | --- | --- |
| `AppShell` / `StatusFooter` | `bridge.hostInfo.hostMode`, `bridge.hostInfo.uiSource` | Native `HostBridge::makeHostInfo()` via JUCE `host.info` event | Yes - smoke JSON proves `embedded` and `vite`; tests assert Standalone/VST3 labels | FLOWING |
| `AnalyzerStage` | `bridge.state` | `BridgeProvider` state from `host.info` / `bridge.error` events | Yes - valid fixture reaches ready; invalid fixtures/errors render typed states | FLOWING |
| `PluginEditor` | `uiSource` | Debug-only CMake cache variable validated against canonical URL | Yes - embedded and Vite smokes return distinct `uiSource` values | FLOWING |
| `WebResources` | requested resource bytes/MIME | Embedded ZIP generated by Vite build and BinaryData | Yes - native tests serve fixture HTML/JS/CSS and reject unsafe/missing resources | FLOWING |

## Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| --- | --- | --- | --- |
| Environment preflight reports locked baseline | `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/check-environment.ps1 -Json` | `ok: true`; Windows 10 build 19045 x64, VS2019 16.11, CMake 4.0.3, Node 22.18.0, npm 10.9.3, Ninja 1.13.1, WebView2 SDK/runtime present | PASS |
| Frontend tests | `npm.cmd --prefix ui run test:run` | 6 files, 17 tests passed | PASS |
| Native CTest | `ctest --preset vs2019-debug --output-on-failure` | 1/1 native test passed | PASS |
| Full phase verification | `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/test-all.ps1` | Passed npm ci, Vitest, UI build, bundle scan, VS2019 Debug Standalone/VST3/native build, CTest, web-mode smokes, and project self-verifier | PASS |
| Built product artifacts | `Get-ChildItem build/vs2019-debug/...` | `LumaScope.exe` and `VST3/.../LumaScope.vst3` present after full build | PASS |

## Probe Execution

| Probe | Command | Result | Status |
| --- | --- | --- | --- |
| Project self-verifier negative probes | `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/verify-project.ps1 -SelfTest` | All five seeded negative probes failed as expected; normal project verification passed | PASS |
| Web mode smoke probes | Included in `scripts/test-all.ps1` via `scripts/test-web-modes.ps1` | `vite-ready`, `vite-unavailable`, `embedded-ready`, `webview2-fallback`, `resource-fallback`, and `handshake-fallback` all passed | PASS |

## Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| --- | --- | --- | --- | --- |
| BUILD-01 | 01-01, 01-03 | Configure/build Windows VST3 and standalone from clean checkout using CMake presets | SATISFIED | Presets exist; bootstrap/test-all build Standalone and VST3. |
| BUILD-02 | 01-01, 01-03 | Pinned dependencies without global project libraries | SATISFIED | JUCE/WebView2 pinned in CMake; npm lockfile; `npm ci`; local caches. |
| BUILD-03 | 01-01, 01-02 | Release targets embed compiled React frontend and open without dev server | SATISFIED | Embedded default, BinaryData ZIP, resource provider, embedded smoke ready. |
| BUILD-04 | 01-03 | Development mode supports rapid frontend iteration without changing release packaging | SATISFIED | Explicit `vs2019-vite` and 5174 URL; release dev URL rejected; Vite and embedded smokes pass. |
| BUILD-05 | 01-03 | One Context7 endpoint and exact library IDs recorded | SATISFIED | `.codex/config.toml`, `AGENTS.md`, `docs/development.md`, and verifier self-test. |
| UI-01 | 01-01, 01-02 | Responsive React/TypeScript MUI application hosted by JUCE WebView2 in both targets | SATISFIED | MUI shell source/tests; WebView2 native editor; Standalone smoke; VST3 target builds from same plugin editor. |
| UI-03 | 01-01, 01-02, 01-03 | Versioned typed protocol with validated shapes, request identifiers, stable errors | SATISFIED | Protocol version 1, `ui.ready`, `host.info`, `bridge.error`, bounded validation, native/Vitest fixture coverage. Request identifiers are not applicable to this event-only Phase 1 handshake; later request/response flows can extend v1. |

## Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| --- | --- | --- | --- | --- |
| `ui/scripts/check-bundle.mjs` | 34 | `console.log` | INFO | CLI success output only, not a stub implementation. |
| `ui/src/bridge/protocol.ts` | 24, 35 | `return null` | INFO | Parser rejection path for invalid payloads, covered by tests. |
| `plugin/include/LumaScope/PluginProcessor.h` | 25 | `return {}` | INFO | JUCE program-name override for empty program names, not user-visible stub. |
| `ui/src/components/BridgeStatus.tsx` | 32 | `return null` for ready state | INFO | Intended because ready status is rendered in footer/stage; not hollow. |

No unreferenced `TODO`, `FIXME`, or `XXX` blockers were found in Phase 1 source/doc/script files.

## Human Verification Evidence

The plan required two blocking human checkpoints:

1. Embedded visual/accessibility shell approval: `01-02-PLAN.md` Task 3 required the real VS2019 Debug standalone at 960x600, 560x360, and Windows scaling. `01-02-SUMMARY.md` records user approval on 2026-06-22.
2. Complete embedded/Vite workflow approval: `01-03-PLAN.md` Task 3 required fresh-terminal bootstrap, embedded launch, Vite hot reload, unavailable-server state, native fallback, and Context7 configuration. `01-03-SUMMARY.md` records user approval on 2026-06-23.

As a verifier, I did not rely on the summary text for code behavior. I independently re-ran the automated smoke paths that overlap those checkpoints. The remaining purely visual/DPI judgment is accepted as already satisfied by the recorded blocking checkpoint.

## Notes

- Approved deviation preserved: canonical Vite URL is `http://127.0.0.1:5174`; verifier did not terminate or touch `bluetti-monitor` / PID 13256.
- MVP mode process warning: ROADMAP.md marks Phase 1 as `mode: mvp`, but `gsd-sdk query user-story.validate` returned `false` for the roadmap goal. This is workflow metadata debt, not a product-shell completion gap.
- Non-fatal output noise: full VS2019 build prints `'pwsh.exe' is not recognized...` from generated helper output, but `scripts/test-all.ps1` exits 0 and the repository documents Windows PowerShell 5.1 as the verified automation host.

## Gaps Summary

No blocking gaps found. Phase 1 satisfies the goal and is ready for the orchestrator to decide next workflow state.

---

_Verified: 2026-06-23T01:52:37Z_  
_Verifier: the agent (gsd-verifier)_
