<!-- GSD:project-start source:PROJECT.md -->
## Project

**JUCE Spectrum Analyzer Example**

A production-minded example project showing how to build a Windows spectrum-analyzer audio product with JUCE 8. It ships as a VST3 plug-in and a standalone application, uses a React/TypeScript Material UI interface hosted in JUCE's WebView, and demonstrates a portable Lemon Squeezy and Cloudflare-based activation system.

The VST3 analyzes audio supplied by its host. The standalone application can analyze either a selected Windows input device or system output captured through WASAPI loopback.

**Core Value:** A developer can clone the project and build, understand, provision, and run the complete analyzer and licensing stack without reconstructing hidden infrastructure or architecture decisions.

### Constraints

- **Platform**: Windows only for v1 — reduces host, device, WebView, installer, and licensing test scope.
- **Formats**: VST3 and standalone — the plug-in consumes host audio while standalone owns device capture.
- **Real-time safety**: The audio callback must not allocate, block, perform network or disk I/O, or communicate directly with the WebView — analyzer data must cross threads through bounded lock-free or wait-free handoff.
- **UI stack**: React, TypeScript, Material UI, JUCE WebBrowserComponent, and Microsoft WebView2 — follows the selected tutorial architecture.
- **Build system**: Modern CMake with deterministic dependency versions — a fresh checkout must be reproducible.
- **Licensing**: Lemon Squeezy purchase authority, one active machine, self-service transfer, Ed25519-signed local entitlement, and seven-day offline grace.
- **Security**: Private signing keys and service credentials remain server-side; webhook signatures, request validation, replay resistance, rate limiting, constant-time comparisons where relevant, and least-privilege API tokens are required.
- **Privacy**: Persist only a derived machine identifier suitable for binding and abuse prevention; document its inputs and limitations without collecting raw hardware identifiers server-side.
- **Infrastructure portability**: Cloudflare Worker, D1 schema/migrations, bindings, routes, variables, and secret contracts must be represented in versioned configuration and provisioned as far as Cloudflare permits through Wrangler or API calls.
- **Secrets**: No secrets or account-specific values in Git; checked-in example environment files and setup validation must make missing configuration obvious.
- **Documentation**: Implementation decisions should be grounded in the requested Context7 sources and current first-party documentation where security or platform behavior matters.
<!-- GSD:project-end -->

<!-- GSD:stack-start source:research/STACK.md -->
## Technology Stack

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
| Context7 MCP | Current hosted server | Version-aware implementation references | One server at `https://mcp.context7.com/mcp`; query pinned library IDs |
## Installation Shape
# Native configuration (exact preset names finalized during implementation)
# Frontend and Worker dependencies
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
<!-- GSD:stack-end -->

<!-- GSD:conventions-start source:CONVENTIONS.md -->
## Conventions

Conventions not yet established. Will populate as patterns emerge during development.
<!-- GSD:conventions-end -->

<!-- GSD:architecture-start source:ARCHITECTURE.md -->
## Architecture

Architecture not yet mapped. Follow existing patterns found in the codebase.
<!-- GSD:architecture-end -->

<!-- GSD:skills-start source:skills/ -->
## Project Skills

No project skills found. Add skills to any of: `.claude/skills/`, `.agents/skills/`, `.cursor/skills/`, `.github/skills/`, or `.codex/skills/` with a `SKILL.md` index file.
<!-- GSD:skills-end -->

## Context7 Documentation

Use the single project Context7 server for library/API setup and version-sensitive implementation details. Query `/websites/juce_master` for JUCE, `/janwilczek/juce-webview-tutorial` for the WebView tutorial architecture, and `/websites/mui_material-ui` for Material UI. Do not create per-library MCP servers or commit API keys.

<!-- GSD:workflow-start source:GSD defaults -->
## GSD Workflow Enforcement

Before using Edit, Write, or other file-changing tools, start work through a GSD command so planning artifacts and execution context stay in sync.

Use these entry points:
- `/gsd-quick` for small fixes, doc updates, and ad-hoc tasks
- `/gsd-debug` for investigation and bug fixing
- `/gsd-execute-phase` for planned phase work

Do not make direct repo edits outside a GSD workflow unless the user explicitly asks to bypass it.
<!-- GSD:workflow-end -->



<!-- GSD:profile-start -->
## Developer Profile

> Profile not yet configured. Run `/gsd-profile-user` to generate your developer profile.
> This section is managed by `generate-claude-profile` -- do not edit manually.
<!-- GSD:profile-end -->
