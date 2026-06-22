# Phase 1: Reproducible Product Shell - Research

**Researched:** 2026-06-22
**Domain:** JUCE 8 Windows VST3/Standalone shell with embedded React/Material UI WebView2 frontend
**Confidence:** HIGH, with one toolchain compatibility risk to prove in the first plan

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

- Product identity is **LumaScope** by **Signal Foundry Audio**, using CMake target `LumaScope`, JUCE manufacturer code `SgFd`, and plug-in code `LmSc`.
- Initial verified baseline is Windows 10 Pro 22H2 x64 build 19045, Visual Studio Community 2019 16.11, Evergreen WebView2, CMake 4.0.3, Node 22.18.0, npm 10.9.3, and Ninja 1.13.1.
- Use a polished dark instrument-panel UI with restrained cyan/lime accents, compact branded header, dominant analyzer-stage placeholder, and quiet status footer.
- Provide one documented PowerShell environment/bootstrap entry point, npm lockfile, explicit CMake presets, embedded frontend assets by default, and opt-in Vite hot reload.
- Native/web communication is a versioned, typed, validated protocol. Phase 1 establishes only handshake/status/diagnostics.
- Do not implement analyzer DSP, source selection, analyzer controls, or licensing behavior in this phase.

### Agent's Discretion

- Repository layout, exact preset/script names, CMake dependency-fetch mechanism, bridge serialization strategy, theme token implementation, and fallback diagnostic presentation.

### Deferred Ideas (OUT OF SCOPE)

- Live spectrum data and controls (Phase 2), standalone device/loopback controls (Phase 3), and activation UI (Phase 6).
</user_constraints>

<architectural_responsibility_map>
## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|--------------|----------------|-----------|
| VST3/Standalone product shell | Native client | Build system | JUCE owns process/host lifecycle; CMake emits both formats |
| Embedded interface | Browser client | Native resource provider | React renders UI while JUCE serves trusted bundled assets |
| Handshake/status protocol | Native client + browser client | Test fixtures | The boundary must be versioned and testable on both sides |
| Frontend packaging | Build system | Native resource provider | npm/Vite produces web assets; CMake packages one deterministic archive |
| Hot reload | Browser client | Native configuration | Explicit development mode navigates to localhost; embedded mode remains authoritative |
| Environment checks | Developer tooling | Documentation | PowerShell reports prerequisites before expensive configuration/build work |
</architectural_responsibility_map>

<research_summary>
## Summary

JUCE 8.0.14 officially requires CMake 3.22+ and supports WebView2 through `juce_add_plugin(... NEEDS_WEB_BROWSER TRUE NEEDS_WEBVIEW2 TRUE)`, the `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` compile definition, and `WebBrowserComponent::Options::Backend::webview2`. Current JUCE exposes native functions, event listeners, initialization data, and a `ResourceProvider`; these should form the bridge rather than ad hoc `evaluateJavascript` calls.

The reference tutorial packages its web directory into a zip, embeds the zip through `juce_add_binary_data`, opens it through a JUCE `ZipFile`, and serves files with a `ResourceProvider`. That pattern avoids CMake configure-time problems with Vite's generated asset list and can remain deterministic if Vite emits a clean `dist/`, the archive has a fixed root, and MIME handling is centralized. The tutorial uses static WebView2 loading and a localhost hot-reload option, but its pinned JUCE 8.0.6, WebView2 SDK 1.0.1901.177, vanilla-JS UI, and VS2022 preset must be updated rather than copied.

The largest uncertainty is Visual Studio 2019. The user's machine has the required C++ workload, and JUCE's published CMake docs say MSVC without naming VS2022 as mandatory, but JUCE 8.0.14 does not explicitly promise VS2019 in the checked sources. Therefore the first plan must run a minimal C++20/JUCE configure-and-build probe under VS2019 before the scaffold grows. If JUCE 8.0.14 fails due to the compiler rather than project code, stop and surface the constraint instead of silently switching toolchains or downgrading JUCE.

**Primary recommendation:** Build a thin end-to-end walking skeleton first: clean CMake configure → VST3 and standalone compile → embedded React shell loads → browser sends `ui.ready@1` → native responds with `host.info@1` → UI visibly shows bridge-ready status. Then add visual polish and developer-mode/diagnostic hardening.
</research_summary>

<standard_stack>
## Standard Stack

### Core

| Library/Tool | Version | Purpose | Why Standard |
|--------------|---------|---------|--------------|
| JUCE | 8.0.14 exact tag | VST3/Standalone targets and WebView2 integration | Current approved JUCE release and official CMake helpers |
| CMake | Minimum 3.22; baseline 4.0.3 | Native dependency and target graph | JUCE's supported build API |
| MSVC | VS2019 16.11 baseline | Windows C++20 build | Locked to current machine; prove compatibility immediately |
| React | Pin current stable during execution | Browser UI | Locked product decision; ecosystem-standard component model |
| TypeScript | Pin current stable during execution | Typed UI/bridge schema | Prevents protocol drift and improves fixture testing |
| Vite | Pin current stable during execution | UI development and production bundle | Fast explicit hot reload and deterministic build hooks |
| Material UI | Pin current stable during execution | Theme and accessible shell components | Locked UI system and approved UI-SPEC |
| WebView2 SDK | Pin a tested current stable NuGet version | WebView2 loader/header/library | Required by JUCE CMake's `NEEDS_WEBVIEW2` path |

### Supporting

| Library/Tool | Version | Purpose | When to Use |
|--------------|---------|---------|-------------|
| CPM.cmake | Pinned release/commit | Fetch JUCE into repository-local cache | Keep checkout reproducible without a JUCE submodule |
| Vitest | Pin current stable | UI and bridge-adapter tests | Browser-side protocol/state tests |
| React Testing Library | Pin current stable | Accessible shell behavior | Status/error/recovery assertions |
| CTest + small native test target | CMake-provided | Native protocol/resource tests | Validate serialization, MIME mapping, and archive lookup |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| CPM-pinned JUCE | Git submodule | Submodule is explicit/offline-friendly but adds contributor setup friction |
| Zip archive + ResourceProvider | Enumerate Vite dist files in `juce_add_binary_data` | Direct embedding is simpler only when output filenames are known at CMake configure time |
| JUCE native integration | Manual JS evaluation/string messages | Manual calls are fragile, untyped, and harder to validate |
| npm | pnpm/yarn | No benefit here; npm is locked by context and already installed |

**Installation shape:** use committed `package-lock.json`, a pinned CPM bootstrap, a pinned WebView2 download step, and CMake presets. Do not depend on global npm packages or a globally installed JUCE.
</standard_stack>

<architecture_patterns>
## Architecture Patterns

### System Architecture Diagram

```text
PowerShell environment check
        |
        v
CMake preset -> pinned JUCE + WebView2 SDK -> LumaScope shared code
        |                                      |             |
        |                                      v             v
        |                                VST3 target    Standalone target
        |
npm ci -> Vite build -> deterministic dist/ -> zip -> juce_add_binary_data
                                                      |
                                                      v
WebBrowserComponent <- ResourceProvider <- embedded zip/MIME map
        |
        +-- embedded mode: resource-provider root
        +-- explicit dev mode: http://127.0.0.1:<fixed-port>
        |
        v
React BridgeProvider -- ui.ready@1 --> native HostBridge
                     <-- host.info@1 --
        |
        v
Visible `Bridge ready` status
```

### Recommended Project Structure

```text
/
├── CMakeLists.txt
├── CMakePresets.json
├── cmake/                     # CPM, warnings, web bundle helpers
├── scripts/                   # environment check/bootstrap
├── src/
│   ├── plugin/                # processor/editor shell
│   ├── bridge/                # protocol identifiers, validation, host adapter
│   └── web/                   # resource provider, MIME/archive helpers
├── tests/native/              # bridge/resource tests
└── ui/
    ├── src/app/               # AppShell and theme
    ├── src/bridge/            # typed transport/provider + mock
    ├── src/components/        # header, stage, footer, status
    └── test/                  # UI/bridge fixtures
```

### Pattern 1: ResourceProvider-backed trusted assets

Use `WebBrowserComponent::Options{}.withBackend(webview2).withNativeIntegrationEnabled().withResourceProvider(...)`. Embedded mode navigates to `WebBrowserComponent::getResourceProviderRoot()`. The provider maps `/` to `index.html`, normalizes safe relative paths, reads the zip entry, and returns bytes with a centralized MIME lookup.

### Pattern 2: Versioned handshake, not arbitrary JavaScript

Define protocol version `1` and stable message/event names in one native header and one generated/shared TypeScript module or fixture. Browser emits `ui.ready` with version; native validates and returns `host.info` containing protocol version, product `LumaScope`, company `Signal Foundry Audio`, mode (`VST3`/`Standalone`), product version, and UI source (`embedded`/`vite`). A mismatch becomes a typed error state.

### Pattern 3: Explicit asset source selection

Use a CMake cache option such as `LUMASCOPE_WEBVIEW_DEV_SERVER` (empty by default). Empty means embedded resource root. A validated localhost URL enables Vite mode and is reflected in `host.info`; failure shows actionable diagnostics. Never probe remote/local URLs automatically in release builds.

### Anti-Patterns to Avoid

- Copying the tutorial's JUCE/WebView2 versions or VS2022 preset without reconciling the locked baseline.
- Running `npm install` or downloading WebView2 on every incremental C++ compile without a stamp/dependency check.
- Treating a visible React page in a normal browser as proof that the packaged WebView works.
- Allowing Vite hashed asset names to escape the archive manifest or serving unknown MIME types as empty strings.
- Falling back to legacy IE if WebView2 fails; Phase 1 requires visible native diagnostics rather than a silently different engine.
- Giving React direct access to raw `window.__JUCE__` throughout the component tree; wrap it in one typed adapter.
</architecture_patterns>

<dont_hand_roll>
## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Plug-in target generation | Custom VST3 bundle rules | `juce_add_plugin` | JUCE handles format artifacts and metadata |
| Browser/native plumbing | Custom window-message bridge | JUCE native integration/events/functions | Correct backend integration and lifecycle |
| WebView2 package discovery | Hard-coded user paths | JUCE `NEEDS_WEBVIEW2` plus `JUCE_WEBVIEW2_PACKAGE_LOCATION` | Works with CMake's expected package layout |
| UI components/theme reset | Bespoke widget library | Material UI ThemeProvider/CssBaseline | Accessibility and consistency |
| Test harness | Watch-mode/manual browser checks only | CTest + Vitest/RTL | Deterministic executor-friendly commands |

**Key insight:** the valuable custom work is the LumaScope protocol, shell, and build contract; platform target generation, WebView plumbing, and accessible primitive components are solved problems.
</dont_hand_roll>

<common_pitfalls>
## Common Pitfalls

### Pitfall 1: VS2019 incompatibility discovered after scaffold growth
**What goes wrong:** Many files are created before the compiler fails on JUCE/C++20 support.
**How to avoid:** First implementation task performs a minimal configure/build under the exact VS2019 generator/toolset and records the outcome.
**Warning signs:** CMake selects VS2022 implicitly, or `cl` is unavailable outside a developer environment.

### Pitfall 2: Frontend bundle is stale or missing from native output
**What goes wrong:** Native build succeeds but ships an old/empty UI.
**How to avoid:** Make the embedded archive an explicit CMake dependency of binary data and shared code; clean `dist/`; expose build provenance in `host.info`; test a known marker string/resource.
**Warning signs:** UI changes require manual copy steps, or touching UI files does not rebuild the archive.

### Pitfall 3: Development mode masks packaged-mode failures
**What goes wrong:** Hot reload works while release assets, MIME types, base URLs, or CSP fail.
**How to avoid:** Embedded mode is default and tested in every wave; dev mode has separate smoke tests and never changes protocol semantics.
**Warning signs:** tests launch only Vite, or release build still requests localhost/CDN assets.

### Pitfall 4: Protocol drift
**What goes wrong:** Native and UI compile independently but disagree at runtime.
**How to avoid:** Shared fixture/version, runtime validation, handshake timeout, mismatch state, and tests on both sides.
**Warning signs:** message names or payload properties are duplicated as unrelated string literals.

### Pitfall 5: WebView2 failure becomes a blank editor
**What goes wrong:** React cannot render the diagnostics intended to explain why React cannot render.
**How to avoid:** Native editor owns a minimal fallback panel and only overlays the WebView after backend creation/navigation succeeds.
**Warning signs:** all error UX is implemented solely in React.
</common_pitfalls>

<code_examples>
## Code Examples

### JUCE target and WebView2 requirements

```cmake
# Source: JUCE 8 CMake API and Jan Wilczek tutorial, adapted to locked scope
juce_add_plugin(LumaScope
  COMPANY_NAME "Signal Foundry Audio"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  PLUGIN_MANUFACTURER_CODE SgFd
  PLUGIN_CODE LmSc
  FORMATS VST3 Standalone
  PRODUCT_NAME "LumaScope"
  NEEDS_WEB_BROWSER TRUE
  NEEDS_WEBVIEW2 TRUE)
```

### Current JUCE WebView options

```cpp
// Source: JUCE WebBrowserComponent official API
juce::WebBrowserComponent::Options{}
  .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
  .withNativeIntegrationEnabled()
  .withResourceProvider(resourceProvider, allowedDevOrigin)
  .withInitialisationData("protocolVersion", 1)
  .withEventListener("ui.ready", onUiReady);
```

### Embedded and development navigation

```cpp
// Source: JUCE official API + tutorial pattern
webView.goToURL(devServerUrl.isEmpty()
  ? juce::WebBrowserComponent::getResourceProviderRoot()
  : devServerUrl);
```
</code_examples>

<sota_updates>
## State of the Art (2026)

| Old/Reference Approach | Current Project Approach | Impact |
|------------------------|--------------------------|--------|
| Tutorial JUCE 8.0.6 | Pin JUCE 8.0.14 | Pull current fixes, including reduced Windows build-tree depth |
| Tutorial VS2022 preset | Prove locked VS2019 16.11 baseline first | Avoid quietly changing the user's environment contract |
| Tutorial vanilla files under `public/` | React/TypeScript/MUI built by Vite into archived `dist/` | Adds a deterministic frontend build boundary |
| Ad hoc examples/buttons | Versioned handshake/status protocol | Establishes the contract later phases extend |
| UI-only errors | Native fallback + React diagnostics | Prevents blank-window dead ends |

**Deprecated/outdated for this project:** tutorial product IDs, AU target, WebView2 SDK 1.0.1901.177, VS2022-only preset, and demo parameter relay controls.
</sota_updates>

<open_questions>
## Open Questions

1. **Does JUCE 8.0.14 build cleanly with VS2019 16.11 on this exact machine?**
   - Known: JUCE requires CMake 3.22+ and an MSVC toolchain; the machine has VS2019 C++ workload.
   - Unclear: checked sources do not explicitly guarantee VS2019 for 8.0.14.
   - Recommendation: make the minimal compiler probe the first blocking task; do not silently substitute VS2022 or older JUCE.

2. **Which current WebView2 SDK version should be pinned?**
   - Known: Evergreen runtime 149 is installed; SDK and runtime versions need not match exactly.
   - Recommendation: select a current stable SDK during execution, record it in dependency configuration, and verify static loader compatibility under VS2019 before locking.
</open_questions>

<validation_architecture>
## Validation Architecture

### Test Layers

| Layer | Tool | Fast Signal | Full Signal |
|-------|------|-------------|-------------|
| Frontend | Vitest + React Testing Library | Bridge adapter/state/component unit tests | UI build + complete Vitest suite |
| Native | CTest with focused JUCE-free/JUCE-light tests | Protocol and MIME/resource helper tests | Configure/build native tests + CTest |
| Build graph | CMake presets | Configure + build one test/shared target | Build VST3 and Standalone Debug targets |
| Packaged integration | Standalone executable/manual harness | Resource archive contains marker and index | Launch embedded standalone; handshake reports `embedded` |
| Development integration | Vite + standalone | Vite build/start health | Launch dev-mode standalone; handshake reports `vite` |

### Sampling Strategy

- Every implementation task runs its nearest frontend or native fast command.
- No three consecutive tasks may rely only on source inspection/manual verification.
- Each plan ends with clean frontend build/tests plus the relevant CMake configure/build/CTest command.
- Phase completion requires both VST3 and standalone artifacts plus manual embedded-mode and dev-mode handshake evidence.
- Avoid watch modes in verification commands.

### Required Fixtures

- Shared protocol v1 JSON fixtures for valid `ui.ready`, valid `host.info`, unsupported version, malformed payload, and runtime error.
- Small embedded asset fixture with `index.html`, JS, CSS, SVG, JSON, and an unknown extension to verify MIME/not-found behavior.
- Browser bridge mock implementing the same TypeScript adapter used by React.

### Manual-only Checks

- WebView2 rendering and native fallback in the real standalone window.
- VST3 editor opening inside at least one host is Phase 2's final validation; Phase 1 may verify the VST3 artifact exists and optionally use a local host if available.
- 150%/200% DPI and Windows forced-colors behavior require visual/manual inspection.
</validation_architecture>

<sources>
## Sources

### Primary (HIGH confidence)

- Context7 `/websites/juce_master` — current WebBrowserComponent options, backend, resource provider, native integration, events/functions.
- Context7 `/janwilczek/juce-webview-tutorial` — CMake, WebView2, zip/BinaryData, ResourceProvider, and hot-reload patterns.
- [JUCE 8.0.14 CMakeLists](https://github.com/juce-framework/JUCE/blob/8.0.14/CMakeLists.txt) — exact version and CMake minimum.
- [JUCE CMake API](https://github.com/juce-framework/JUCE/blob/8.0.14/docs/CMake%20API.md) — target flags and WebView2 package location.
- [JUCE WebBrowserComponent API](https://docs.juce.com/master/classjuce_1_1WebBrowserComponent.html) — backend and resource/native APIs.
- [Jan Wilczek JUCE WebView tutorial](https://github.com/janwilczek/juce-webview-tutorial) — inspected current CMake, editor/resource provider, presets, and SDK download script.
- `.planning/phases/01-reproducible-product-shell/01-UI-SPEC.md` — approved visual and interaction contract.

### Secondary (MEDIUM confidence)

- VS2019 compatibility inference from JUCE's generic MSVC documentation; must be confirmed by the planned build probe.
</sources>

<metadata>
## Metadata

**Research scope:** JUCE target setup, Windows toolchain, WebView2 SDK/runtime, frontend archive pipeline, bridge handshake, testing architecture, and failure modes.

**Confidence breakdown:**
- Standard stack: HIGH — locked and verified against current primary sources.
- Architecture: HIGH — JUCE APIs and reference tutorial inspected directly.
- Pitfalls: HIGH — grounded in the boundary and build graph.
- VS2019 compatibility: MEDIUM — intentionally moved into an early blocking proof.

**Research date:** 2026-06-22
**Valid until:** 2026-07-22 for JUCE/build patterns; recheck npm/WebView2 package versions at execution.
</metadata>

---

*Phase: 01-reproducible-product-shell*
*Research completed: 2026-06-22*
*Ready for planning: yes*
