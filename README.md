# LumaScope

**LumaScope** is a production-minded example project demonstrating how to build a Windows spectrum-analyzer audio product with [JUCE 8](https://juce.com). It ships as a VST3 plug-in and a standalone application, uses a React/TypeScript [Material UI](https://mui.com) interface hosted in JUCE's WebView2, and will demonstrate a portable [Lemon Squeezy](https://lemonsqueezy.com) and [Cloudflare](https://cloudflare.com) Workers-based activation system.

**Core value:** A developer can clone the project and build, understand, provision, and run the complete analyzer and licensing stack without reconstructing hidden infrastructure or architecture decisions.

---

## Current status

| Phase | Status | What it delivers |
|-------|--------|------------------|
| 1 | ✅ Complete | Reproducible Windows build shell, embedded React/MUI WebView, typed native/web bridge |
| 2 | ✅ Complete | End-to-end VST3 spectrum analyzer — windowed FFT, log-mapped bins, smoothing, canvas renderer |
| 3 | 🔜 Planned | Standalone Windows device capture (input + WASAPI loopback) |
| 4–6 | 🔜 Planned | Lemon Squeezy cloud activation, one-machine licensing, offline grace |
| 7 | 🔜 Planned | Release hardening, CI, documentation, handoff proof |

The VST3 path is fully functional: insert it on an audio track, hear unchanged passthrough, and see a smooth logarithmic spectrum rendered in the WebView at a bounded frame rate. Standalone device capture and licensing remain in development.

---

## Architecture overview

```
Host DAW / Standalone shell
  │
  └── LumaScopeAudioProcessor
        │  Audio callback (real-time safe)
        │    ├── Windowed FFT (juce::dsp)
        │    ├── Log-frequency bin mapping
        │    ├── One-pole smoothing/decay
        │    └── Lock-free SPSC mailbox
        │
        └── LumaScopeAudioProcessorEditor
              Message-thread poller ──→ WebView (React/MUI canvas)
                    Typed JSON bridge (protocol v1)
```

- **Real-time safety:** The audio callback never allocates, blocks, performs I/O, or touches the WebView. Analyzer data crosses threads through a bounded lock-free atomic mailbox.
- **Snapshots:** The processor owns the analyzer and publishes complete `SpectrumSnapshot` structs. The editor polls these on a timer (not the audio thread) and converts them to closed JSON bridge events.
- **Bridge protocol:** Versioned, typed, closed-schema JSON events over JUCE native events. No string-built JavaScript evaluation. See [bridge protocol](docs/bridge-protocol.md).

---

## Technology stack

### Core
| Technology | Version | Purpose |
|------------|---------|---------|
| JUCE | 8.0.14, pinned | VST3/standalone shell, audio, DSP, WebView |
| C++ | C++20 | Real-time DSP, host integration, licensing |
| CMake | 3.22+ | Reproducible native builds |
| MSVC | VS 2022 toolset | Windows compiler |
| React + TypeScript | Pinned | Embedded application UI |
| Material UI | Current `@mui/material` | Accessible controls, layout, theming |
| Vite | Pinned | Frontend dev/build |
| WebView2 | Pinned SDK, Evergreen runtime | Windows WebView backend |

### Future (Phases 4–6)
| Technology | Purpose |
|------------|---------|
| Cloudflare Workers + D1 | Webhook ingestion, activation, validation, entitlement storage |
| Wrangler | Worker deployment, migrations, secrets |
| Lemon Squeezy | Purchase and entitlement authority |
| Ed25519 (Web Crypto) | Signed local entitlement tokens |

### Documentation sources
The project uses one [Context7 MCP](https://github.com/upstash/context7) server with three pinned library IDs:
- `/websites/juce_master` — AudioDeviceManager, FFT/windowing, WebView, thread handoff
- `/janwilczek/juce-webview-tutorial` — CMake, WebView2, embedded assets, JUCE modules
- `/websites/mui_material-ui` — TypeScript, theming, responsiveness, CSP

---

## Repository structure

```
├── plugin/            JUCE VST3 + Standalone target (CMakeLists.txt, sources)
│   ├── include/       Public headers (Processor, Editor, Analyzer, Bridge, WebResources)
│   ├── source/        Implementation files
├── ui/                React/TypeScript/Vite frontend workspace
├── cmake/             CMake modules (WebBundle, dependency resolution)
├── scripts/           Build, test, validation, and packaging scripts
├── docs/              Design docs (DSP contract, bridge protocol, development workflow, smoke test)
├── tests/native/      C++ native unit tests (CTest)
├── worker/            Cloudflare Worker source (future phases)
└── .planning/         GSD phase plans, state, and roadmap
```

---

## Quick start

From a fresh PowerShell terminal at the repository root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/check-environment.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/bootstrap.ps1 -Preset vs2019-debug
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/test-all.ps1
```

The bootstrap checks prerequisites, runs `npm ci`, downloads pinned JUCE 8.0.14 and WebView2 SDK 1.0.4022.49, configures CMake, and builds the Standalone, VST3, and native tests. It does not install system software. Pinned dependencies are cached in ignored `.deps/` and `ui/node_modules/` directories.

Launch the authoritative embedded Debug application:

```powershell
& .\build\vs2019-debug\plugin\LumaScope_artefacts\Debug\Standalone\LumaScope.exe
```

Expected footer: `Embedded UI · Bridge ready`.

### Vite hot-reload development

Start the fixed loopback server in one terminal:

```powershell
npm.cmd --prefix ui run dev -- --host 127.0.0.1 --port 5174 --strictPort
```

Then configure, build, and launch the explicit development preset in another terminal:

```powershell
cmake --preset vs2019-vite
cmake --build --preset vs2019-vite --target LumaScope_Standalone --parallel 4
& .\build\vs2019-vite\plugin\LumaScope_artefacts\Debug\Standalone\LumaScope.exe
```

Expected footer: `Vite development · Bridge ready`. Edit `ui/src/components/AnalyzerStage.tsx` while both processes run to verify hot reload.

See [development](docs/development.md) for the full workflow and [troubleshooting](docs/troubleshooting.md) for diagnostics.

---

## Build targets

| Target | Format | Description |
|--------|--------|-------------|
| `LumaScope_Standalone` | Standalone EXE | Analyzer as a desktop app — owns device capture |
| `LumaScope_VST3` | VST3 DLL | Analyzer as a DAW plug-in — consumes host audio |
| `LumaScope_All` | Both | Convenience target for building everything |
| `LumaScope_Tests` | Executable | Native CTest suite for DSP and analyzer core |

### CMake presets

| Preset | Build type | Dev server | Use case |
|--------|-----------|------------|----------|
| `vs2019-debug` | Debug | None (embedded) | Default development and testing |
| `vs2019-vite` | Debug | `http://127.0.0.1:5174` | Frontend hot-reload development |
| `vs2019-release` | Release | None (embedded) | Production builds |

---

## VST3 validation and host smoke

Phase 2 VST3 validation uses the automated suite plus [pluginval](https://github.com/Tracktion/pluginval) when available:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/test-all.ps1
cmake --build --preset vs2019-debug --target LumaScope_VST3 --parallel 4
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/validate-plugin.ps1
```

The test suite covers:
- **DSP correctness:** Deterministic tone frequencies within expected bins, full-scale level within ±1.5 dB, silence at configured floor, no NaN/infinity output
- **Robustness:** Zero-sized blocks, sample-rate changes, channel-layout changes, denormals, overlapping hops
- **Profiles:** Distinct behavior across Musical (default), Measurement, and Fast profiles
- **Lifecycle:** Audio continues correctly while editor is closed, reopened, resized, or destroyed
- **pluginval:** Automated VST3 validation with headless CI support

`scripts/test-all.ps1` allows missing pluginval for local development but prints that validation was skipped, not passed. For release evidence, provide pluginval with `-PluginvalPath` or `PLUGINVAL_EXE`, then complete the Ableton-preferred smoke in [VST3 smoke test](docs/vst3-smoke-test.md).

---

## Key design decisions

| Decision | Rationale |
|----------|-----------|
| **Windows-only v1** | Reduces host, device, WebView, installer, and licensing test scope |
| **VST3 + Standalone** | Plug-in consumes host audio; standalone owns device capture |
| **React/MUI in WebView2** | Modern UI stack with JUCE 8 native integration |
| **CMake (not Projucer)** | Reviewable, automatable, CI-friendly builds |
| **Lock-free SPSC mailbox** | Bounded audio-thread → UI-thread handoff without allocation or blocking |
| **Canvas spectrum renderer** | Bounded renderer nodes instead of hundreds of DOM elements per frame |
| **Ed25519-signed local entitlements** | Offline verification without embedding server secrets |
| **Cloudflare Workers + D1** | Small operational footprint, portable account provisioning |
| **Pinned dependencies** | Reproducible builds — no floating `master`/`latest` |
| **No secrets in Git** | Account IDs, credentials, signing keys stay server-side |

For implementation details see [analyzer DSP contract](docs/analyzer-dsp.md), [bridge protocol](docs/bridge-protocol.md), and [development workflow](docs/development.md).

---

## Phase roadmap

| Phase | What it delivers | Depends on | Status |
|-------|-----------------|------------|--------|
| 1 | Reproducible build shell, embedded WebView2, typed bridge | — | ✅ Complete |
| 2 | End-to-end VST3 spectrum analyzer | Phase 1 | ✅ Complete |
| 3 | Standalone Windows device capture (input + WASAPI loopback) | Phase 2 | 🔜 Planned |
| 4 | Cloudflare Worker + D1 provisioning, Lemon webhook ingestion | Phase 1 | 🔜 Planned |
| 5 | One-machine activation service (signed entitlements) | Phase 4 | 🔜 Planned |
| 6 | Native offline licensing with 7-day grace | Phase 5 | 🔜 Planned |
| 7 | Release hardening, CI, documentation, handoff proof | Phases 3, 6 | 🔜 Planned |

See [roadmap](.planning/ROADMAP.md) and [requirements](.planning/REQUIREMENTS.md) for complete details.

---

## License

This example project is provided for educational and reference purposes. The JUCE framework is subject to its own licensing terms.
