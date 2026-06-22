# Architecture Research

**Domain:** Real-time desktop audio plus WebView UI and cloud entitlement
**Researched:** 2026-06-22
**Confidence:** HIGH

## System Overview

```text
VST3 host audio ─┐
                 ├─> Audio source -> preallocated sample FIFO -> FFT worker -> display snapshots
Standalone input ┤                                                        |
WASAPI loopback ─┘                                                        v
                                                                  JUCE bridge adapter
                                                                          |
                                                                   React/MUI WebView

Lemon Squeezy -> signed webhook -> Cloudflare Worker -> D1 licenses/activations
                                          ^                    |
Plugin activation/deactivation ----------|                    v
                                signed machine entitlement -> local encrypted/ACL'd file
```

### Component Responsibilities

| Component | Responsibility | Typical Implementation |
|-----------|----------------|------------------------|
| Audio source adapters | Normalize host, JUCE input, and WASAPI loopback samples | Thin adapters into a shared analyzer ingress |
| Analyzer core | Window, FFT, magnitudes, smoothing, snapshots | UI-agnostic C++ library with deterministic tests |
| Real-time handoff | Move bounded sample/snapshot data across threads | Preallocated SPSC ring buffers and atomics |
| Plug-in shell | Buses, transparent audio pass-through, state, editor | JUCE AudioProcessor target |
| Standalone shell | Device selection and loopback lifecycle | JUCE app plus Windows-specific capture adapter |
| Native/Web bridge | Typed commands, events, snapshots, protocol version | JUCE WebBrowserComponent options/listeners |
| React UI | Spectrum presentation, source selection, activation UX | React/TypeScript/MUI; Canvas/WebGL for continuous graph |
| Entitlement core | Machine fingerprint, token verify, local state/grace | UI-independent C++ module; embedded public key only |
| Activation Worker | Webhook, activate, validate, deactivate, signing | TypeScript Worker with narrow routes |
| D1 repository | License, activation, webhook idempotency, audit records | Prepared statements, migrations, transactional/batched writes |
| Provisioner | Create resources, fill account-specific overlay, migrate/deploy/verify | Versioned manifest plus idempotent script/API/Wrangler calls |

## Recommended Project Structure

```text
/
├── CMakeLists.txt
├── CMakePresets.json
├── cmake/                         # dependency and packaging helpers
├── src/
│   ├── analyzer/                  # DSP core and snapshot model
│   ├── audio/                     # source abstraction and lock-free handoff
│   ├── plugin/                    # AudioProcessor/editor shell
│   ├── standalone/                # app/device orchestration
│   ├── platform/windows/          # WASAPI loopback and machine fingerprint
│   ├── bridge/                    # native/web protocol
│   └── licensing/                 # entitlement verification/storage/client
├── ui/                            # React/TypeScript/MUI/Vite
├── worker/
│   ├── src/                       # Worker routes/services/repositories
│   ├── migrations/                # D1 SQL migrations
│   ├── wrangler.jsonc             # portable logical config
│   └── test/
├── infra/
│   ├── project.yaml               # logical resource/secret contract
│   ├── scripts/                   # bootstrap/deploy/verify/teardown
│   └── generated/                 # gitignored account-specific IDs/state
├── tests/                         # native integration and fixtures
└── docs/                          # setup, threat model, protocols, runbooks
```

## Architectural Patterns

### Hexagonal Boundaries Around Audio Sources

The analyzer accepts a small sample-source contract. Host blocks, JUCE capture callbacks, and native loopback all adapt into it. This prevents Windows capture details from contaminating DSP or plug-in code.

### Snapshot, Never Live-Pull

The audio callback copies bounded samples to a preallocated queue. A worker computes FFT frames and publishes immutable/latest-wins snapshots. UI refresh reads at display cadence; it never calls into processing state or forces every FFT frame through JavaScript.

### Versioned Native/Web Protocol

Bridge messages have explicit names, schemas, protocol version, request IDs, and error envelopes. JavaScript talks to a mockable adapter so browser tests do not require a plug-in host.

### Server-Authoritative, Offline-Verifiable Entitlement

The Worker owns activation policy and signs canonical entitlement payloads. The application embeds only the public key, verifies signature/machine/product/expiry locally, and tracks the last successful validation for grace calculations. The file is evidence, not a shared secret.

### Declarative Logical Infrastructure + Generated Physical State

Commit logical names, bindings, migrations, routes, required secrets, and policies. Bootstrap creates D1/API resources and writes account-specific IDs to a gitignored overlay consumed by deployment. This keeps one repository portable without pretending Cloudflare resource IDs are portable.

## Key Data Flows

1. **VST3 analysis:** Host block is passed through unchanged; samples are copied to the analyzer queue; snapshots travel to WebView at a bounded frame rate.
2. **Standalone capture:** User selects input or output-loopback source; source adapter normalizes channel/sample format; analyzer path stays identical.
3. **Purchase:** Worker verifies Lemon's `X-Signature` against the exact raw body, deduplicates event ID, and updates entitlement state in D1.
4. **Activation:** Client sends license key plus derived machine ID over HTTPS; Worker validates entitlement and a single-seat invariant, records activation, signs canonical token, and returns it.
5. **Offline load:** Client verifies token signature and machine binding without network; after seven days since successful validation it shows a blocking revalidation state.
6. **Transfer:** Authenticated deactivation removes/disables the matching activation and invalidates future validation; local token is removed after server success.
7. **Provisioning:** Script authenticates with least-privilege token, resolves account, creates/finds D1, writes generated binding overlay, applies migrations, uploads secrets interactively/from environment, deploys Worker, and runs smoke checks.

## Scaling Considerations

| Scale | Architecture Adjustments |
|-------|--------------------------|
| Example/0–1k licenses | One Worker + one D1 database; indexed tables and request rate limits |
| 1k–100k licenses | Add observability, stronger per-key/IP rate limiting, archival policy, concurrency load tests |
| 100k+ licenses | Reassess D1 contention, audit retention, regional requirements, and queue webhook work if needed |

The first likely bottleneck is concurrent activation/deactivation policy enforcement, not FFT or static UI. Design D1 writes so the one-seat invariant cannot be violated by races.

## Anti-Patterns

- **UI as DSP scheduler:** render cadence is nondeterministic; DSP timing must be audio/worker driven.
- **One giant AudioProcessor:** makes standalone capture, licensing, and tests host-dependent.
- **Trusting a local “activated=true” flag:** trivial to copy/edit and cannot represent expiry or machine binding.
- **Mirroring Lemon calls from clients:** increases attack surface and prevents policy centralization.
- **Committing deployed Cloudflare IDs:** silently points forks at the wrong account and breaks handoff.

## Integration Points

| Service | Integration Pattern | Notes |
|---------|---------------------|-------|
| Lemon Squeezy | Signed raw-body webhook plus server-side License/API verification as needed | HMAC-SHA256; API documented at 60 requests/minute |
| Cloudflare D1 | Worker binding and checked-in SQL migrations | Prepared statements; index license hashes and machine IDs |
| Cloudflare deploy | Wrangler v4/package scripts, optionally TypeScript SDK/API bootstrap | API token scopes documented and minimized |
| WebView2 | JUCE WebBrowserComponent with embedded production assets | Development URL only in explicit dev mode |
| Context7 | One hosted MCP endpoint, explicit library IDs | JUCE `/websites/juce_master`; tutorial `/janwilczek/juce-webview-tutorial`; MUI `/websites/mui_material-ui` |

## Sources

- Context7 JUCE, WebView tutorial, and Material UI libraries
- [Microsoft WASAPI loopback](https://learn.microsoft.com/windows/win32/coreaudio/loopback-recording)
- [Cloudflare D1 migrations](https://developers.cloudflare.com/d1/reference/migrations/)
- [Cloudflare infrastructure as code](https://developers.cloudflare.com/workers/platform/infrastructure-as-code/)
- [Lemon Squeezy signing requests](https://docs.lemonsqueezy.com/help/webhooks/signing-requests)
- [Lemon Squeezy License API](https://docs.lemonsqueezy.com/api/license-api)

---
*Architecture research for: JUCE Spectrum Analyzer Example*
*Researched: 2026-06-22*
