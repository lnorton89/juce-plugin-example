# Project Research Summary

**Project:** JUCE Spectrum Analyzer Example
**Domain:** Windows real-time audio product with embedded web UI and portable serverless licensing
**Researched:** 2026-06-22
**Confidence:** HIGH

## Executive Summary

This project is best built as several small, testable cores joined by adapters: an allocation-free analyzer ingress and DSP library, separate VST3 and standalone audio-source shells, a versioned JUCE/WebView bridge, an offline-verifiable entitlement library, and a narrow Cloudflare Worker/D1 service. The UI can follow Jan Wilczek's JUCE 8 WebView packaging pattern, but production use needs bounded snapshot delivery, contract tests, packaged-asset diagnostics, and Canvas/WebGL rendering inside an MUI application shell.

The standalone requirement changes the architecture materially. Ordinary input capture can use JUCE device management, while system-output monitoring should live behind a native shared-mode WASAPI-loopback adapter with device-invalidation and format-change handling. Both feed the same analyzer core so the VST3 and standalone paths cannot drift.

Licensing should treat Lemon Squeezy as purchase/entitlement authority and Cloudflare as policy/signing authority. Lemon webhooks are verified over raw bytes and deduplicated; D1 enforces the one-machine invariant; the Worker signs a versioned Ed25519 entitlement; the application embeds only public keys and supports seven days since successful validation. Portability requires a logical infrastructure manifest, migrations, scripts, generated untracked account bindings, secret contracts, and a clean-account rehearsal.

## Key Findings

### Recommended Stack

- **JUCE 8.0.14 + C++20 + CMake/MSVC:** current Windows native foundation, pinned for reproducibility.
- **React/TypeScript/Vite + Material UI + WebView2:** modern embedded UI with typed bridge boundaries.
- **JUCE DSP + native WASAPI loopback:** shared analyzer core with platform adapter only where required.
- **Cloudflare Workers/D1 + Wrangler v4:** small backend with versioned schema and portable deployment.
- **Context7 MCP:** one hosted server with explicit JUCE, tutorial, and MUI library IDs.

### Expected Features

**Must have:** correct smooth spectrum, stereo-safe transparent VST3, selectable input and system-output standalone capture, resizable packaged WebView UI, activation/deactivation/status UX, signed seven-day offline entitlement, and reproducible Cloudflare setup.

**Defer:** advanced analyzer controls, installer/signing pipeline, account portal, macOS/AU, and multi-seat tiers.

### Architecture Approach

1. **Analyzer core** — deterministic window/FFT/smoothing and immutable snapshots.
2. **Audio adapters** — host, JUCE input, and native loopback feed one ingress contract.
3. **Web bridge/UI** — versioned typed messages and bounded latest-wins display frames.
4. **Entitlement core** — machine binding, signature verification, storage, grace logic.
5. **Worker/D1 backend** — verified webhooks, policy, activation records, token signing.
6. **Infrastructure tooling** — manifest, bootstrap, migrations, secrets, deploy, verification.

### Critical Pitfalls

1. **Audio-thread blocking/allocation** — prevent with preallocation and bounded lock-free handoff.
2. **Plausible-but-wrong FFT display** — prevent with documented normalization and deterministic signal tests.
3. **Loopback treated like a normal input** — isolate native shared-mode WASAPI lifecycle.
4. **Bridge flooding/schema drift** — bounded snapshot rate, typed protocol, contract fixtures.
5. **Licensing races/forged events** — database invariants, raw-body HMAC, idempotency, rate limits.
6. **Offline token bypass** — canonical signed claims, versioned machine hash, key rotation, clock tests.
7. **Hidden Cloudflare click-ops** — clean-account deployment is a release criterion.

## Implications for Roadmap

### Phase 1: Reproducible Native and Web Foundation
**Rationale:** Every later slice depends on pinned JUCE/WebView2/frontend builds and test targets.
**Delivers:** VST3/standalone skeleton, packaged React/MUI shell, Context7 project guidance, CI baseline.

### Phase 2: End-to-End VST3 Analyzer
**Rationale:** Validates the core user value through the simpler host-provided audio path.
**Delivers:** tested FFT/smoothing core, transparent processing, bounded WebView visualization.

### Phase 3: Standalone Device and Loopback Monitoring
**Rationale:** Reuses validated analyzer/UI while adding Windows-specific source complexity.
**Delivers:** input selection, WASAPI loopback, lifecycle/recovery UX.

### Phase 4: Portable Cloudflare/Lemon Foundation
**Rationale:** Provisioning and schema must exist before client activation integration.
**Delivers:** logical YAML manifest, bootstrap/deploy scripts, D1 migrations, signed/idempotent webhook ingestion.

### Phase 5: One-Machine Activation Backend
**Rationale:** Builds policy on verified purchases and reproducible infrastructure.
**Delivers:** activate/validate/deactivate routes, concurrency-safe seat enforcement, Ed25519 token signing, rate limits/audit tests.

### Phase 6: Client Activation and Offline Grace
**Rationale:** Integrates a stable backend into a tested native/UI product.
**Delivers:** privacy-conscious machine binding, local verification/storage, transfer UX, seven-day grace and failure modes.

### Phase 7: Release Hardening and Handoff
**Rationale:** Cross-component failures only surface under host/device/outage/fresh-account testing.
**Delivers:** pluginval/DAW/device matrix, threat-model checks, clean-account deployment rehearsal, complete build/deploy docs.

### Phase Ordering Rationale

- Prove build and UI packaging before DSP or licensing complexity.
- Deliver the core analyzer vertically in VST3 before adding platform-specific capture.
- Establish declarative cloud resources and webhook truth before activation policy.
- Integrate client licensing only after signed token fixtures and backend contracts stabilize.
- Reserve a final phase for system-level validation rather than pretending unit tests prove host/device/account portability.

### Research Flags

- **Standalone capture:** deeper planning research on JUCE Windows backend versus a focused COM/WASAPI adapter and device notification model.
- **Cloud backend:** deeper research on D1 concurrency semantics, exact Worker rate-limit binding, and current Cloudflare secret/key options.
- **Client licensing:** threat model for machine fingerprint inputs, secure Windows file location/ACL/DPAPI trade-offs, and clock rollback behavior.
- **Release:** exact WebView2 distribution and Windows code-signing/installer expectations, even if installer remains v1.x.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | Current JUCE release and first-party build/runtime docs checked |
| Features | HIGH | User scope is explicit and table stakes are established |
| Architecture | HIGH | Boundaries follow real-time audio and server-authoritative licensing constraints |
| Pitfalls | HIGH | Major risks are documented in Microsoft, Lemon, Cloudflare, and JUCE behavior |

**Overall confidence:** HIGH

### Gaps to Address

- Exact product/company/plugin identifiers should be chosen before packaging.
- Minimum Windows version and WebView2 runtime distribution policy need explicit acceptance criteria.
- Lost-machine recovery beyond self-service deactivation needs a documented operator policy.
- Cloudflare jurisdiction/region choice should remain configurable at first database creation.
- Final machine fingerprint must be tested against ordinary Windows hardware/OS updates.

## Sources

### Primary

- Context7 `/websites/juce_master`, `/janwilczek/juce-webview-tutorial`, `/websites/mui_material-ui`
- [JUCE CMake API](https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md)
- [Microsoft WASAPI loopback](https://learn.microsoft.com/windows/win32/coreaudio/loopback-recording)
- [Lemon webhook signing](https://docs.lemonsqueezy.com/help/webhooks/signing-requests)
- [Lemon License API](https://docs.lemonsqueezy.com/api/license-api)
- [Cloudflare D1 migrations](https://developers.cloudflare.com/d1/reference/migrations/)
- [Cloudflare Wrangler configuration](https://developers.cloudflare.com/workers/wrangler/configuration/)
- [Context7 MCP repository](https://github.com/upstash/context7)

### Secondary

- [Jan Wilczek JUCE WebView tutorial](https://github.com/janwilczek/juce-webview-tutorial) — starting implementation pattern; adapt rather than copy blindly

---
*Research completed: 2026-06-22*
*Ready for roadmap: yes*
