# Stack Research

**Domain:** Windows audio plug-in/standalone analyzer with WebView UI and serverless licensing
**Researched:** 2026-06-22
**Confidence:** HIGH

## Recommended Stack

### Core Technologies

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| JUCE | 8.0.14, pinned | VST3/standalone shell, audio devices, DSP, WebView | Current JUCE 8 release; one codebase for both targets with official CMake support |
| C++ | C++20 | Real-time DSP, host integration, local licensing | Strong JUCE/toolchain support without requiring bleeding-edge language mode |
| CMake | 3.22+ | Reproducible native builds | Minimum documented by JUCE and used by the WebView tutorial |
| MSVC | Visual Studio 2022 current toolset | Windows compiler and debugger | JUCE's primary Windows toolchain and best WebView2/Windows SDK integration |
| React + TypeScript | Pinned stable releases at implementation | Embedded application UI | Matches tutorial architecture while keeping bridge contracts typed |
| Material UI | Current stable `@mui/material` | Accessible controls, layout, theming | Requested UI system with mature TypeScript support |
| Vite | Pinned stable release | Frontend dev/build | Fast local iteration and deterministic production asset output |
| Cloudflare Workers + D1 | Current GA APIs | Webhook, activation, deactivation, validation, entitlement storage | Small operational footprint and portable account provisioning |
| Wrangler | Current stable v4, pinned | Local Worker development, D1 migrations, secrets, deploy | Cloudflare's canonical project/deployment tool |

### Supporting Libraries

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `juce_dsp` | From pinned JUCE | FFT and windowing | Analyzer processing and test reference |
| WebView2 SDK/runtime | Pinned SDK; Evergreen runtime prerequisite | Windows WebView backend | React editor in both VST3 and standalone |
| `juce_audio_utils` | From pinned JUCE | Plug-in/standalone device utilities | Common audio shell and device selection |
| Windows Core Audio APIs | Target supported Windows SDK | Native WASAPI loopback | System-output capture where JUCE's ordinary input abstraction is insufficient |
| Web Crypto / `node:crypto` in Workers | Worker runtime | HMAC verification and Ed25519 signing | Lemon webhook validation and offline token issuance |
| Vitest + React Testing Library | Pinned stable | Frontend unit/component tests | Bridge adapters, activation states, and UI behavior |
| Cloudflare Workers test tooling | Current supported | Worker integration tests | D1 queries, webhook idempotency, activation races |

### Development Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| CMake presets | Repeatable Debug/Release/MSVC configuration | Include build, test, and packaging presets |
| CPM.cmake or FetchContent | Fetch pinned dependencies | Pin immutable tags/commits and support an offline cache |
| CTest | Native unit/integration test runner | Test DSP and licensing core independently of hosts |
| pluginval | VST3 validation | Run headless validation in CI where feasible |
| ESLint/Prettier | Frontend quality | Keep WebView code independent from generated artifacts |
| Wrangler environments | Local/preview/production separation | Never reuse production D1/secrets in local tests |
| Context7 MCP | Current hosted server via stdio package | Version-aware implementation references | One project server configured with `npx -y @upstash/context7-mcp`; query pinned library IDs |

## Installation Shape

```powershell
# Native configuration (exact preset names finalized during implementation)
cmake --preset vs
cmake --build --preset vs-debug

# Frontend and Worker dependencies
npm ci
npx wrangler d1 migrations apply licenses --local
```

Use checked-in lockfiles. Do not rely on globally installed JUCE, Node packages, Wrangler, or WebView2 SDK files.

## Alternatives Considered

| Recommended | Alternative | When to Use Alternative |
|-------------|-------------|-------------------------|
| JUCE `dsp::FFT` | FFTW/KissFFT | Only after profiling proves JUCE FFT inadequate or licensing/size demands differ |
| Native WASAPI loopback adapter | Vendor “Stereo Mix” input | Never as the primary path; vendor devices are inconsistent and may be disabled |
| Canvas/WebGL spectrum renderer | Hundreds of React/MUI nodes | DOM may work for coarse bars, but not continuous high-bin rendering |
| Wrangler + small bootstrap script | Full Terraform estate | Terraform becomes useful if the example grows into multiple Cloudflare services/accounts |
| Worker-issued signed entitlement | Online check every launch | Online-only enforcement is simpler but violates the seven-day offline requirement |

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| Projucer as project source of truth | Harder to review/automate and diverges from requested reference | CMake |
| Floating `master`/`latest` dependencies | Breaks reproducible handoff | Pinned versions plus intentional update workflow |
| Network/disk/UI calls on the audio thread | Dropouts, deadlocks, host instability | Preallocated SPSC handoff and non-real-time workers |
| Direct Lemon Squeezy calls from the plug-in | Exposes policy to tampering and complicates provider changes | Narrow Cloudflare activation API |
| A separate Context7 process per documentation URL | Context7 exposes libraries through one MCP server | One MCP server, three recorded library IDs |
| Account IDs or D1 UUIDs hard-coded in committed config | Prevents account portability | Generated untracked deployment overlay/state |

## Version Compatibility

| Package A | Compatible With | Notes |
|-----------|-----------------|-------|
| JUCE 8.0.14 | CMake 3.22+, VS 2022 | Exact Windows SDK baseline validated in build phase |
| JUCE WebBrowserComponent | WebView2 | Set `NEEDS_WEB_BROWSER`, `NEEDS_WEBVIEW2`, and JUCE WebView2 definitions |
| React/TypeScript/MUI/Vite | WebView2 Evergreen | Target browser capabilities supported by the chosen minimum Windows baseline |
| Wrangler v4 | Workers + D1 | Pin in Worker package and use `npx`/package scripts |

## Sources

- `/websites/juce_master` — AudioDeviceManager, FFT/windowing, WebView, and thread handoff guidance
- `/janwilczek/juce-webview-tutorial` — CMake, WebView2, embedded assets, and JUCE modules
- `/websites/mui_material-ui` — TypeScript, theming, responsiveness, accessibility, CSP
- [JUCE 8.0.14 release](https://github.com/juce-framework/JUCE/releases/tag/8.0.14) — current release verification
- [JUCE CMake API](https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md) — supported build configuration
- [Context7 MCP](https://github.com/upstash/context7) — one-server installation and library-ID model
- [Cloudflare Wrangler configuration](https://developers.cloudflare.com/workers/wrangler/configuration/) — bindings and deployment config

---
*Stack research for: JUCE Spectrum Analyzer Example*
*Researched: 2026-06-22*
