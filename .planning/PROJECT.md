# JUCE Spectrum Analyzer Example

## What This Is

A production-minded example project showing how to build a Windows spectrum-analyzer audio product with JUCE 8. It ships as a VST3 plug-in and a standalone application, uses a React/TypeScript Material UI interface hosted in JUCE's WebView, and demonstrates a portable Lemon Squeezy and Cloudflare-based activation system.

The VST3 analyzes audio supplied by its host. The standalone application can analyze either a selected Windows input device or system output captured through WASAPI loopback.

## Core Value

A developer can clone the project and build, understand, provision, and run the complete analyzer and licensing stack without reconstructing hidden infrastructure or architecture decisions.

## Requirements

### Validated

- [x] Phase 1 validated a reproducible Windows build shell for VST3 and standalone targets using JUCE 8, CMake presets, pinned dependencies, and automated checks.
- [x] Phase 1 validated the embedded React/TypeScript Material UI WebView2 shell with a typed protocol-v1 bridge and accessible bridge-state UX.
- [x] Phase 1 validated project-level Context7 MCP configuration and recorded JUCE master, Jan Wilczek's JUCE WebView tutorial, and Material UI source identifiers.
- [x] Phase 2 validated a responsive, stable, non-mutating VST3 spectrum analyzer path from host audio through native FFT/smoothing analysis to the React/WebView canvas renderer.
- [x] Phase 2 validated analyzer defaults behind extensible DSP, snapshot, bridge, and UI contracts so FFT size, smoothing, decay, and display controls can expand later without redesigning the real-time path.
- [x] Phase 6 implemented versioned Windows machine identity, Ed25519 token parser/verifier with public-key rotation, DPAPI-protected local entitlement store, 7-day grace model with clock rollback detection, atomic cross-thread licensing state machine, queue-based activation HTTP client, React/MUI activation/deactivation/grace-warning UI, and both-target integration with HostBridge protocol extension.
- [x] Phase 3 validated standalone Windows monitoring with selectable JUCE input devices and WASAPI loopback capture, source preference persistence, device notifications, and diagnostics.
- [x] Phase 4 validated portable Cloudflare Worker/D1 provisioning with Lemon Squeezy webhook ingestion, idempotent event processing, and fresh-account deployment guide.
- [x] Phase 5 validated one-machine activation service with Ed25519 signing, concurrency-safe activate/validate/deactivate, rate limiting, key rotation, and automated Worker tests.
- [x] Phase 6.5 validated centralized project configuration with project-config.json, JSON Schema, CMake integration, C++/TypeScript/Worker/PowerShell consumers, validation scripts, and one-command regeneration.

### Active

- [ ] Phase 7: Release and Handoff Proof — validate hosts, devices, security, CI, diagnostics, and clean-account deployment end to end.

### Out of Scope

- macOS, Linux, AU, AAX, and mobile builds — initial release is Windows-only.
- Audio processing or sound modification — this release is an analyzer, not an effect.
- Multiple concurrent machine activations — one active machine is the product policy.
- A bespoke licensing dashboard — operational tooling should initially use Lemon Squeezy, Cloudflare, and narrowly scoped scripts.
- Fully adjustable analyzer controls — v1 ships strong defaults and extension points; user-facing tuning can follow later.
- Manual click-ops as the primary deployment path — unavoidable account bootstrap steps may be documented, but infrastructure must otherwise be reproducible.

## Context

- This is a greenfield example intended to demonstrate maintainable boundaries between real-time DSP, the JUCE host layer, the WebView frontend, local license verification, and cloud activation services.
- The JUCE WebView tutorial by Jan Wilczek is the starting reference for CMake packaging, WebView2, JUCE-to-JavaScript communication, and embedded frontend assets.
- Required documentation sources are Context7's JUCE master library, Jan Wilczek's JUCE WebView tutorial library, and Material UI library.
- Purchase flow: Lemon Squeezy sends a signed webhook to a Cloudflare Worker, which creates or updates the license record in D1.
- Activation flow: the application submits the license key and a privacy-conscious machine identifier hash; the Worker validates entitlement and activation count, records the machine, and returns a signed token.
- Local flow: the application verifies the token with an embedded public key, binds it to the local machine, stores it under the user's application-data directory, and supports a seven-day offline grace period.
- Transfer flow: the user can deactivate the current machine, freeing the single activation for another machine. Replay, concurrency, and lost-machine recovery need explicit treatment during licensing design.
- Cloudflare deployment should be portable between accounts: declarative configuration, D1 migrations, secret-name contracts, setup scripts, verification, and teardown guidance belong in the repository.
- Context7 should be configured once as a project MCP server; the three requested Context7 library URLs are recorded as authoritative source identifiers for research and implementation.
- Current analyzer state: the VST3 path has passed unit/integration checks, strict non-GUI pluginval validation, and an approved Ableton host smoke test after lag/accuracy tuning.

## Constraints

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

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Target Windows, VST3, and standalone first | Keeps the example focused while covering host and device-capture use cases | Validated in Phase 1 for shell/build artifacts and Phase 2 for VST3 analyzer behavior; standalone audio capture remains active |
| Use React/TypeScript and Material UI in a JUCE WebView2 editor | Enables modern UI development while demonstrating JUCE 8 native integration | Validated in Phase 1 as the embedded shell and Vite development workflow |
| Support WASAPI input and loopback capture in standalone | Covers microphones/interfaces and system-output monitoring | Validated in Phase 3 |
| Ship analyzer defaults behind an extensible DSP/UI model | Produces a useful v1 without prematurely designing every control | Validated in Phase 2 through native analyzer profiles, snapshots, bridge events, and canvas rendering |
| Use Lemon Squeezy, Cloudflare Workers, and D1 for activation | Matches the requested purchase and serverless deployment model | Validated in Phase 4-5 |
| Enforce one active machine with self-service transfer | Provides a clear commercial policy without permanent lock-in | Validated in Phase 5 |
| Verify Ed25519-signed machine-bound entitlements locally | Allows offline verification without embedding server secrets | Validated in Phase 6 |
| Use a seven-day offline grace period | Balances offline use with entitlement enforcement | Validated in Phase 6 |
| Treat Cloudflare infrastructure as code and automate account provisioning | Makes the example transferable and avoids undocumented dashboard state | Validated in Phase 4 |
| Configure one Context7 MCP server with three pinned source references | Context7 is the MCP provider; JUCE, tutorial, and MUI are libraries within it | Validated in Phase 1 |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition**:
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone**:
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-06-27 — reconciled Active/Validated/Key Decisions with full codebase audit*
