# Requirements: JUCE Spectrum Analyzer Example

**Defined:** 2026-06-22
**Core Value:** A developer can clone the project and build, understand, provision, and run the complete analyzer and licensing stack without reconstructing hidden infrastructure or architecture decisions.

## v1 Requirements

### Build and Packaging

- [x] **BUILD-01**: Developer can configure and build Windows VST3 and standalone targets from a clean checkout using documented CMake presets.
- [x] **BUILD-02**: Build resolves pinned JUCE, WebView2 SDK, frontend, Worker, and test dependencies without requiring globally installed project libraries.
- [x] **BUILD-03**: Release targets embed the compiled React frontend and open it without a development server.
- [x] **BUILD-04**: Development mode supports rapid frontend iteration without changing release packaging behavior.
- [x] **BUILD-05**: Project records one Context7 MCP endpoint and the exact JUCE, JUCE WebView tutorial, and Material UI library IDs for implementation guidance.

### Spectrum Analysis

- [x] **DSP-01**: Analyzer accepts mono or stereo floating-point audio at host/device sample rates without modifying source audio.
- [x] **DSP-02**: Analyzer applies a documented window, FFT, single-sided magnitude normalization, and decibel conversion that place deterministic test tones at the expected frequency and level tolerance.
- [x] **DSP-03**: Analyzer publishes a logarithmic-frequency spectrum with sensible fixed FFT size, visible range, smoothing, and decay defaults.
- [x] **DSP-04**: Analyzer handles silence, denormals, changing block sizes, sample-rate changes, and channel-layout changes without invalid output or crashes.
- [x] **DSP-05**: Audio callbacks perform no dynamic allocation, blocking synchronization, file access, network access, JSON work, or WebView calls.
- [x] **DSP-06**: Analyzer configuration and snapshot interfaces allow future FFT, range, smoothing, decay, and display controls without replacing the DSP core.

### VST3 Plug-in

- [x] **VST3-01**: User can load the VST3 as an audio effect in a compatible Windows host and see the spectrum for audio routed through it.
- [x] **VST3-02**: Plug-in output is sample-equivalent to its input within floating-point tolerance for supported layouts.
- [x] **VST3-03**: Plug-in remains stable when its editor is closed, repeatedly opened, resized, or destroyed while audio processing continues.
- [x] **VST3-04**: Plug-in passes automated VST3 validation and documented smoke tests in at least one real Windows DAW.

### Standalone Audio Capture

- [ ] **CAP-01**: User can select and monitor an available Windows audio input device in the standalone application.
- [x] **CAP-02**: User can select and monitor a Windows render endpoint through shared-mode WASAPI loopback without requiring a vendor “Stereo Mix” device.
- [x] **CAP-03**: Standalone converts supported device channel/sample formats into the analyzer ingress contract without changing analyzer behavior.
- [x] **CAP-04**: Standalone safely handles source switching, no active source, device removal, default-device changes, capture invalidation, and restart.
- [ ] **CAP-05**: Standalone persists the last valid source preference and falls back visibly when that source is unavailable.

### WebView User Interface

- [x] **UI-01**: User sees a responsive React/TypeScript Material UI application hosted by JUCE WebView2 in both VST3 and standalone targets.
- [x] **UI-02**: Spectrum renders smoothly at a bounded display frame rate without creating a DOM/MUI element per FFT bin.
- [x] **UI-03**: Native and web layers communicate through a versioned, typed protocol with validated message shapes, request identifiers, and stable error envelopes.
- [ ] **UI-04**: UI exposes source selection and source/error state in standalone while omitting standalone-only controls in VST3.
- [ ] **UI-05**: UI exposes activation, activated, offline-grace, revalidation-required, deactivation, and actionable failure states.
- [ ] **UI-06**: UI remains usable across supported window sizes and Windows DPI scaling, with keyboard-accessible controls and sufficient contrast.
- [ ] **UI-07**: Missing WebView2 runtime, packaged-asset failure, and bridge-version mismatch produce visible native diagnostics rather than a blank editor.

### Cloud Entitlement Service

- [x] **CLOUD-01**: Worker verifies Lemon Squeezy webhook `X-Signature` with HMAC-SHA256 over exact raw request bytes before parsing or storing the event.
- [x] **CLOUD-02**: Worker processes relevant purchase, subscription, expiry, refund, and disablement events idempotently and rejects events for unconfigured stores/products/variants.
- [x] **CLOUD-03**: D1 migrations create versioned license, activation, webhook-idempotency, and audit data with indexes and constraints required by query and policy paths.
- [ ] **CLOUD-04**: Client can activate a valid entitlement for one derived machine identifier and receive a canonical Ed25519-signed entitlement token.
- [ ] **CLOUD-05**: Concurrent activation requests cannot create more than one active machine for a one-seat license.
- [ ] **CLOUD-06**: Client can validate its current activation and receive a refreshed signed entitlement when server entitlement remains valid.
- [ ] **CLOUD-07**: Client can deactivate its matching activation so the license can be activated on another machine.
- [ ] **CLOUD-08**: Activation endpoints use request validation, generic non-leaking errors, rate limiting, bounded payloads, redacted logs, and structured audit events.
- [ ] **CLOUD-09**: Signing supports key identifiers and public-key rotation without invalidating every still-supported token immediately.
- [ ] **CLOUD-10**: Worker tests cover valid and invalid webhooks, retries, activation races, deactivation, expiry/revocation, rate limits, and database failures.

### Native Licensing

- [ ] **LIC-01**: Application derives a versioned, privacy-conscious machine identifier and never transmits raw hardware identifiers to the service.
- [ ] **LIC-02**: Application activates by sending the license key and machine identifier only from a non-real-time thread over authenticated HTTPS.
- [ ] **LIC-03**: Application verifies token signature, key identifier, schema version, product, machine binding, issued time, and validity bounds using embedded public keys.
- [ ] **LIC-04**: Application stores the verified entitlement in the user's application-data directory with appropriate Windows access protection and detects corrupt or tampered files.
- [ ] **LIC-05**: Application launches without a server call for up to seven days after the last successful online validation when the signed entitlement remains otherwise valid.
- [ ] **LIC-06**: Application requires online revalidation after the seven-day window and clearly warns before the offline window expires.
- [ ] **LIC-07**: Application resists simple wall-clock rollback from extending offline use indefinitely and documents the remaining limitations.
- [ ] **LIC-08**: Application clears local entitlement only after successful server deactivation and then permits activation on another machine.
- [ ] **LIC-09**: Licensing state is available to audio/UI code as non-blocking immutable or atomic status and never performs enforcement work in an audio callback.
- [ ] **LIC-10**: Native tests use cross-language token fixtures and cover valid, wrong-machine, expired, corrupt, unknown-key, grace-boundary, clock-change, and server-outage cases.

### Portable Cloudflare Infrastructure

- [ ] **INFRA-01**: Repository contains a declarative YAML manifest describing logical Worker, D1, bindings, environments, required secrets, configured Lemon identifiers, and optional routing without account credentials.
- [x] **INFRA-02**: Developer can run an idempotent bootstrap command that authenticates to a chosen Cloudflare account, creates or reuses required resources, and writes account-specific IDs only to gitignored generated state.
- [x] **INFRA-03**: Developer can apply local and remote D1 migrations through documented package commands.
- [x] **INFRA-04**: Developer can inject required secrets through Wrangler or Cloudflare APIs without placing secret values in committed files or command output.
- [x] **INFRA-05**: Developer can deploy the Worker and bindings through a repeatable command using a least-privilege Cloudflare API token.
- [x] **INFRA-06**: Deployment workflow reports missing prerequisites/configuration before mutation and supports non-destructive verification plus explicitly confirmed teardown.
- [ ] **INFRA-07**: Fresh Cloudflare account can be provisioned, migrated, deployed, and smoke-tested using only repository documentation and declared prerequisites.
- [ ] **INFRA-08**: Documentation explains the unavoidable Lemon Squeezy webhook registration/secret exchange and outputs the deployed callback URL needed to complete it.

### Quality and Handoff

- [ ] **QUAL-01**: CI builds native targets and frontend/Worker packages and runs native, bridge, frontend, and Worker tests on supported Windows tooling.
- [ ] **QUAL-02**: Repository documents architecture, native/web protocol, entitlement token schema, threat model, machine-binding policy, cloud resources, secret rotation, and recovery procedures.
- [ ] **QUAL-03**: Repository includes a manual verification matrix for DAW behavior, devices/loopback, DPI/resize, WebView failures, offline grace, transfer, outage, and clean-account deployment.
- [ ] **QUAL-04**: Example configuration and logs use fictitious/redacted identifiers and repository secret scanning finds no credentials, private keys, license keys, account IDs, or deployed D1 UUIDs.

## v2 Requirements

### Analyzer Controls

- **CTRL-01**: User can choose supported FFT sizes and window functions.
- **CTRL-02**: User can adjust smoothing, decay, visible dB range, and frequency range.
- **CTRL-03**: User can freeze the display, hold peaks, and inspect cursor frequency/level.

### Distribution

- **DIST-01**: User can install signed VST3 and standalone binaries through a Windows installer.
- **DIST-02**: Release pipeline signs binaries and produces checksums/versioned artifacts.

### Licensing Operations

- **OPS-01**: Operator can recover a seat when the licensed machine is permanently lost.
- **OPS-02**: Customer can manage activations through a hosted account portal.

### Platform Expansion

- **PLAT-01**: Project builds macOS VST3, AU, and standalone targets.
- **PLAT-02**: Licensing supports platform-specific machine binding and secure storage on macOS.

## Out of Scope

| Feature | Reason |
|---------|--------|
| Audio effect processing | Product is an analyzer and must preserve audio |
| Linux/mobile/AAX | Windows VST3 and standalone are the approved initial targets |
| More than one concurrent activation | Product policy is one active machine with transfer |
| Bespoke customer/admin portal in v1 | Cloud/provider tooling and focused scripts cover initial operations |
| Full Terraform estate in v1 | Wrangler plus a logical manifest/bootstrap script is smaller and sufficient; reassess with more resources/accounts |
| Strong anti-tamper/DRM claims | Client software cannot make absolute piracy guarantees; prioritize secure design and user recovery |

## Traceability

Roadmap phase mappings are populated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| BUILD-01 | Phase 1 | Complete |
| BUILD-02 | Phase 1 | Complete |
| BUILD-03 | Phase 1 | Complete |
| BUILD-04 | Phase 1 | Complete |
| BUILD-05 | Phase 1 | Complete |
| DSP-01 | Phase 2 | Complete |
| DSP-02 | Phase 2 | Complete |
| DSP-03 | Phase 2 | Complete |
| DSP-04 | Phase 2 | Complete |
| DSP-05 | Phase 2 | Complete |
| DSP-06 | Phase 2 | Complete |
| VST3-01 | Phase 2 | Complete |
| VST3-02 | Phase 2 | Complete |
| VST3-03 | Phase 2 | Complete |
| VST3-04 | Phase 2 | Complete |
| CAP-01 | Phase 3 | Pending |
| CAP-02 | Phase 3 | Complete |
| CAP-03 | Phase 3 | Complete |
| CAP-04 | Phase 3 | Complete |
| CAP-05 | Phase 3 | Pending |
| UI-01 | Phase 1 | Complete |
| UI-02 | Phase 2 | Complete |
| UI-03 | Phase 1 | Complete |
| UI-04 | Phase 3 | Pending |
| UI-05 | Phase 6 | Pending |
| UI-06 | Phase 7 | Pending |
| UI-07 | Phase 7 | Pending |
| CLOUD-01 | Phase 4 | Complete (04-03: Webhook verification) |
| CLOUD-02 | Phase 4 | Complete (04-03: Event processing and idempotency) |
| CLOUD-03 | Phase 4 | Complete (04-03: Repository and schema implementation) |
| CLOUD-04 | Phase 5 | Pending |
| CLOUD-05 | Phase 5 | Pending |
| CLOUD-06 | Phase 5 | Pending |
| CLOUD-07 | Phase 5 | Pending |
| CLOUD-08 | Phase 5 | Pending |
| CLOUD-09 | Phase 5 | Pending |
| CLOUD-10 | Phase 5 | Pending |
| LIC-01 | Phase 6 | Pending |
| LIC-02 | Phase 6 | Pending |
| LIC-03 | Phase 6 | Pending |
| LIC-04 | Phase 6 | Pending |
| LIC-05 | Phase 6 | Pending |
| LIC-06 | Phase 6 | Pending |
| LIC-07 | Phase 6 | Pending |
| LIC-08 | Phase 6 | Pending |
| LIC-09 | Phase 6 | Pending |
| LIC-10 | Phase 6 | Pending |
| INFRA-01 | Phase 4 | In Progress (04-01: Manifest, wrangler config, generated-state) |
| INFRA-02 | Phase 4 | Complete |
| INFRA-03 | Phase 4 | Complete |
| INFRA-04 | Phase 4 | Complete |
| INFRA-05 | Phase 4 | Complete |
| INFRA-06 | Phase 4 | Complete |
| INFRA-07 | Phase 4 | Pending |
| INFRA-08 | Phase 4 | Pending |
| QUAL-01 | Phase 7 | Pending |
| QUAL-02 | Phase 7 | Pending |
| QUAL-03 | Phase 7 | Pending |
| QUAL-04 | Phase 7 | Pending |

**Coverage:**
- v1 requirements: 59 total
- Mapped to phases: 59
- Unmapped: 0 ✓

---
*Requirements defined: 2026-06-22*
*Last updated: 2026-06-22 after roadmap creation*
