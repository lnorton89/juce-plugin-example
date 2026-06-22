# Roadmap: JUCE Spectrum Analyzer Example

## Overview

The roadmap grows one working product slice at a time: first a reproducible JUCE/WebView shell, then a host-safe VST3 analyzer, then standalone Windows capture, followed by a reproducible cloud entitlement service and native offline activation. The final phase proves the whole example can survive real hosts, devices, outages, and a handoff to a fresh Cloudflare account.

## Phases

- [ ] **Phase 1: Reproducible Product Shell** - Build both Windows targets and display a packaged, typed React/MUI WebView shell.
- [ ] **Phase 2: End-to-End VST3 Analyzer** - Analyze host audio correctly and render it smoothly without changing the signal.
- [ ] **Phase 3: Standalone Windows Monitoring** - Analyze selectable input devices and system output through WASAPI loopback.
- [ ] **Phase 4: Portable Purchase Infrastructure** - Provision Cloudflare resources reproducibly and ingest trusted Lemon Squeezy entitlement events.
- [ ] **Phase 5: One-Machine Activation Service** - Activate, validate, deactivate, and sign one-seat entitlements safely under concurrency.
- [ ] **Phase 6: Native Offline Licensing** - Integrate activation, self-service transfer, local verification, and seven-day offline grace into both targets.
- [ ] **Phase 7: Release and Handoff Proof** - Validate hosts, devices, security, CI, diagnostics, and clean-account deployment end to end.

## Phase Details

### Phase 1: Reproducible Product Shell

**Goal**: A developer can build the VST3 and standalone applications from a clean checkout and see the same packaged React/MUI interface in both.
**Mode:** mvp
**Depends on**: Nothing (first phase)
**Requirements**: BUILD-01, BUILD-02, BUILD-03, BUILD-04, BUILD-05, UI-01, UI-03
**UI hint**: yes
**Success Criteria** (what must be TRUE):

  1. Developer can run documented CMake presets on a clean Windows machine and produce VST3 and standalone binaries with pinned dependencies.
  2. Both binaries open the packaged React/TypeScript Material UI shell without a development server.
  3. Frontend development mode reloads rapidly while release mode uses embedded assets through a versioned typed bridge.
  4. Project guidance configures one Context7 MCP endpoint and identifies all three requested documentation libraries.

**Plans**: 3 plans
Plans:
**Wave 1**

- [ ] 01-01: Establish pinned CMake/JUCE targets, presets, dependency policy, and native test skeleton.

**Wave 2** *(blocked on Wave 1 completion)*

- [ ] 01-02: Establish React/TypeScript/Vite/MUI workspace and deterministic embedded-asset pipeline.

**Wave 3** *(blocked on Wave 2 completion)*

- [ ] 01-03: Integrate WebView2 in both targets with versioned bridge handshake and Context7 project guidance.

### Phase 2: End-to-End VST3 Analyzer

**Goal**: A user can insert the VST3 on audio, hear unchanged output, and see a correct, smooth logarithmic spectrum.
**Mode:** mvp
**Depends on**: Phase 1
**Requirements**: DSP-01, DSP-02, DSP-03, DSP-04, DSP-05, DSP-06, VST3-01, VST3-02, VST3-03, VST3-04, UI-02
**UI hint**: yes
**Success Criteria** (what must be TRUE):

  1. Deterministic tone and silence tests verify frequency placement, level normalization, smoothing, and behavior across supported sample rates/block sizes.
  2. A Windows DAW can load the VST3 and display a bounded-frame-rate logarithmic spectrum while audio remains sample-equivalent to input.
  3. Processing remains real-time safe and continues correctly while the editor is closed, reopened, resized, or destroyed.
  4. The VST3 passes automated validation and the analyzer interfaces expose future control extension points.

**Plans**: 4 plans

Plans:

- [ ] 02-01: Implement and test windowed FFT, normalization, log mapping, smoothing, and snapshot model.
- [ ] 02-02: Implement preallocated real-time ingress and worker-to-UI snapshot handoff.
- [ ] 02-03: Integrate transparent VST3 buses/processing and efficient web spectrum renderer.
- [ ] 02-04: Verify lifecycle, host behavior, pluginval, and DAW smoke criteria.

### Phase 3: Standalone Windows Monitoring

**Goal**: A standalone user can select either an input device or render endpoint and reliably inspect its spectrum.
**Mode:** mvp
**Depends on**: Phase 2
**Requirements**: CAP-01, CAP-02, CAP-03, CAP-04, CAP-05, UI-04
**UI hint**: yes
**Success Criteria** (what must be TRUE):

  1. User can choose a JUCE-managed input device and see its spectrum through the same analyzer path used by VST3.
  2. User can choose a Windows output endpoint and capture the shared-mode system mix without a vendor loopback device.
  3. Source switching, device invalidation/removal, silence, and restart are safe and visibly explained.
  4. The last valid source preference is restored or replaced by a clear fallback when unavailable.

**Plans**: 3 plans

Plans:

- [ ] 03-01: Define shared source lifecycle and integrate selectable JUCE input capture.
- [ ] 03-02: Implement native Windows WASAPI loopback adapter, conversion, notifications, and recovery.
- [ ] 03-03: Integrate standalone source controls, persistence, diagnostics, and device test matrix.

### Phase 4: Portable Purchase Infrastructure

**Goal**: A developer can provision the cloud foundation in their own account and safely mirror Lemon Squeezy entitlement events into D1.
**Mode:** mvp
**Depends on**: Phase 1
**Requirements**: CLOUD-01, CLOUD-02, CLOUD-03, INFRA-01, INFRA-02, INFRA-03, INFRA-04, INFRA-05, INFRA-06, INFRA-07, INFRA-08
**Success Criteria** (what must be TRUE):

  1. Developer can bootstrap or reuse D1/Worker resources from a logical YAML manifest without committing account IDs, resource UUIDs, or secrets.
  2. Local and remote migrations create the documented license, activation, idempotency, and audit schema.
  3. Worker accepts only correctly signed, configured Lemon Squeezy events and processes retries idempotently.
  4. A fresh Cloudflare account can be provisioned, configured with secrets, deployed, and smoke-tested using repository instructions.
  5. Scripts fail safely before mutation when prerequisites are missing and make teardown explicit and confirmable.

**Plans**: 4 plans

Plans:

- [ ] 04-01: Define logical infrastructure manifest, portable Wrangler config, generated-state boundary, and token scopes.
- [ ] 04-02: Build idempotent bootstrap, secret, migration, deploy, verify, and teardown command surface.
- [ ] 04-03: Implement D1 schema/repositories and exact-raw-body Lemon webhook verification/idempotency.
- [ ] 04-04: Rehearse fresh-account provisioning and document Lemon webhook registration handoff.

### Phase 5: One-Machine Activation Service

**Goal**: A valid purchase can hold exactly one transferable machine activation and receive verifiable signed entitlements.
**Mode:** mvp
**Depends on**: Phase 4
**Requirements**: CLOUD-04, CLOUD-05, CLOUD-06, CLOUD-07, CLOUD-08, CLOUD-09, CLOUD-10
**Success Criteria** (what must be TRUE):

  1. Valid license can activate one machine and receives a canonical Ed25519-signed token containing a key identifier.
  2. Concurrent requests cannot exceed one active machine, and retries return idempotent outcomes.
  3. Activated machine can validate/refresh or deactivate itself so another machine may activate.
  4. Invalid, expired, refunded, disabled, abusive, oversized, or rate-limited requests receive stable non-leaking errors and structured audit records.
  5. Automated tests cover signing, rotation, webhook changes, concurrency, failures, validation, and deactivation.

**Plans**: 3 plans

Plans:

- [ ] 05-01: Define canonical token/API contracts, key rotation, request validation, and error model.
- [ ] 05-02: Implement concurrency-safe activate/validate/deactivate policy and D1 repository operations.
- [ ] 05-03: Add rate limits, audit/redaction, signing fixtures, integration tests, and deployed smoke tests.

### Phase 6: Native Offline Licensing

**Goal**: Users can activate both products, work offline for seven days, understand status, and transfer the license themselves.
**Mode:** mvp
**Depends on**: Phase 5
**Requirements**: LIC-01, LIC-02, LIC-03, LIC-04, LIC-05, LIC-06, LIC-07, LIC-08, LIC-09, LIC-10, UI-05
**UI hint**: yes
**Success Criteria** (what must be TRUE):

  1. User can enter a valid key on Windows and activate the current privacy-conscious machine identity without affecting audio-thread safety.
  2. Both targets verify and store signed entitlements locally and launch offline for up to seven days after successful validation.
  3. UI warns before grace expiry and distinguishes activated, offline, revalidation-required, revoked/expired, corrupt, and service-unavailable states.
  4. User can deactivate successfully, clear local entitlement afterward, and activate another machine.
  5. Cross-language fixtures and native tests cover signature, machine, time, corruption, rotation, clock rollback, and outage boundaries.

**Plans**: 4 plans

Plans:

- [ ] 06-01: Implement versioned Windows machine identity, token parser/verifier, canonical fixtures, and public-key rotation.
- [ ] 06-02: Implement protected local storage, grace/clock model, entitlement state machine, and native tests.
- [ ] 06-03: Implement non-real-time activation client and React/MUI activation/deactivation/status flows.
- [ ] 06-04: Integrate both targets and verify offline, transfer, expiry, revocation, corruption, and outage scenarios.

### Phase 7: Release and Handoff Proof

**Goal**: Another developer can validate, diagnose, and reproduce the complete product and cloud stack without hidden knowledge.
**Mode:** mvp
**Depends on**: Phases 3 and 6
**Requirements**: UI-06, UI-07, QUAL-01, QUAL-02, QUAL-03, QUAL-04
**UI hint**: yes
**Success Criteria** (what must be TRUE):

  1. CI builds both native targets and runs native, frontend, bridge, Worker, migration, and secret-scan checks.
  2. UI remains accessible and usable across supported size/DPI cases and shows native diagnostics for WebView/runtime/assets/protocol failures.
  3. A documented manual matrix passes in a real DAW, across input/loopback device failures, and through activation/offline/outage/transfer scenarios.
  4. A second developer can follow architecture, threat-model, protocol, recovery, and provisioning documentation to build and deploy into a clean account.
  5. Repository and generated examples contain no credentials, private keys, real license keys, account identifiers, or deployed D1 UUIDs.

**Plans**: 3 plans

Plans:

- [ ] 07-01: Complete CI, validation, secret scanning, WebView fallback diagnostics, accessibility, and DPI hardening.
- [ ] 07-02: Complete architecture, threat model, protocols, operations, recovery, and verification documentation.
- [ ] 07-03: Execute full host/device/licensing/fresh-account handoff matrix and close release gaps.

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Reproducible Product Shell | 0/3 | Not started | - |
| 2. End-to-End VST3 Analyzer | 0/4 | Not started | - |
| 3. Standalone Windows Monitoring | 0/3 | Not started | - |
| 4. Portable Purchase Infrastructure | 0/4 | Not started | - |
| 5. One-Machine Activation Service | 0/3 | Not started | - |
| 6. Native Offline Licensing | 0/4 | Not started | - |
| 7. Release and Handoff Proof | 0/3 | Not started | - |
