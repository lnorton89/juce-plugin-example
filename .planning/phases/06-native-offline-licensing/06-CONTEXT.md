# Phase 6: Native Offline Licensing - Context

**Gathered:** 2026-06-23T17:05:08.2833635-07:00
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 6 connects the native Windows products to the completed Phase 5 activation service. Users can enter a license key, activate the current machine, verify and store a signed entitlement locally, launch offline for up to seven days after successful online validation, understand licensing state in the WebView UI, deactivate the current machine, and transfer the single activation to another machine.

This phase owns native machine identity derivation, local entitlement parsing and Ed25519 verification, public-key rotation support, protected local storage, grace and clock-rollback handling, non-real-time activation/validation/deactivation calls, licensing state propagation to native/UI code, compact activation/status UI, and cross-language fixtures/tests. It must not perform licensing network, disk, JSON, or enforcement work in any audio callback.

</domain>

<decisions>
## Implementation Decisions

### Machine Identity

- **D-01:** Use a balanced Windows machine fingerprint for v1: derive the machine identifier locally from multiple privacy-conscious Windows/device signals, hash or otherwise derive the final stable identifier locally, and never transmit raw hardware identifiers to the service.
- **D-02:** The machine identity must be versioned so future platform or privacy changes can coexist with existing activations and fixtures.
- **D-03:** Document the exact local inputs, stability expectations, and limitations. The design should be honest about normal Windows hardware churn, profile changes, repair/reinstall cases, and the fact that client-side identity cannot be a perfect anti-tamper boundary.

### Local Entitlement and Offline Grace

- **D-04:** Offline grace should be generous but visible. If the signed entitlement is otherwise valid and the last successful online validation is within seven days, both targets should launch and operate normally while showing a clear offline/grace status.
- **D-05:** The UI should warn before grace expiry and distinguish normal activated, offline grace, revalidation required, revoked/expired, corrupt/tampered, and service-unavailable states.
- **D-06:** After the seven-day offline window expires, the application requires successful online validation before treating the product as activated again.
- **D-07:** The local state machine should resist simple wall-clock rollback from extending grace indefinitely, while documenting remaining limitations rather than implying strong DRM guarantees.

### Activation UI and Product Tone

- **D-08:** Licensing belongs in a compact status/control surface, not a full-screen or dashboard-first experience. Preserve the analyzer-first instrument feel.
- **D-09:** Use a compact header/footer-adjacent control or status affordance for activation state, with dialogs for license-key entry, activation progress/errors, and deactivation confirmation.
- **D-10:** The UI should make licensing action states clear and actionable without turning the app into an account-management portal. Recovery copy maps stable native/server error codes to friendly bounded messages.

### Transfer and Failure Behavior

- **D-11:** Transfer is server-authoritative. Clear the local entitlement only after successful server deactivation of the matching activation.
- **D-12:** If refresh/validation fails because the service or network is unavailable, continue to honor the remaining local grace window and present a service-unavailable/offline status.
- **D-13:** If the server reports revoked, expired, deactivated, activation-not-found, or another authoritative entitlement failure, show reactivation-required or a specific failure state and stop relying on the local token for activated status.
- **D-14:** Failed deactivation must leave the current local entitlement intact and explain that transfer has not completed.

### the agent's Discretion

The planner may choose the exact Windows identity inputs, hash/encoding format, protected-storage mechanism, file layout, state-machine class boundaries, retry/backoff policy, request scheduling, UI component breakdown, and copy details, provided the decisions above and existing real-time, bridge, security, and privacy constraints are preserved.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project and Phase Scope

- `.planning/PROJECT.md` - Product constraints, Windows-only target, licensing policy, privacy/security posture, local flow, transfer flow, and core value.
- `.planning/REQUIREMENTS.md` - Phase 6 requirements `LIC-01` through `LIC-10` and `UI-05`.
- `.planning/ROADMAP.md` - Phase 6 goal, success criteria, planned waves, dependency on Phase 5, and Phase 7 handoff boundary.
- `.planning/STATE.md` - Current project state, accumulated decisions, and baseline concerns.
- `.planning/phases/05-one-machine-activation-service/05-CONTEXT.md` - Locked Phase 5 activation API, one-machine policy, token envelope, signing, key rotation, and abuse-control decisions.
- `.planning/phases/05-one-machine-activation-service/05-03-SUMMARY.md` - Phase 5 implementation summary and verification handoff for activation endpoints, signing, rate limits, audit, and smoke tests.

### Existing Activation and Signing Contracts

- `worker/src/api/contracts.ts` - Activation request/response types, endpoint paths, signed entitlement token shape, and local token claims.
- `worker/src/signing/entitlement.ts` - Canonical JSON Ed25519 signing/verification helpers, public-key ring parsing, and base64url helpers that native fixtures must match.
- `worker/src/signing/canonicalJson.ts` - Canonical JSON behavior for cross-language token fixtures.
- `worker/src/routes.ts` - Existing `/api/v1/activate`, `/api/v1/validate`, and `/api/v1/deactivate` route integration.
- `tests/worker/signing.test.ts` - Worker-side signing fixtures and verification expectations to mirror in native tests.
- `tests/worker/activation-routes.test.ts` - Server behavior that native activation client tests should align with.

### Native, Bridge, and UI Integration

- `plugin/include/LumaScope/HostBridge.h` - Current protocol-v1 event identifiers and bounded payload helpers; Phase 6 licensing events should follow this pattern.
- `plugin/source/HostBridge.cpp` - Native bridge payload construction and validation patterns.
- `tests/native/HostBridgeTests.cpp` - Native bridge fixture/test pattern for closed protocol payloads.
- `docs/bridge-protocol.md` - Current protocol-v1 schemas and versioning rules; licensing status/request events must extend this intentionally.
- `ui/src/bridge/protocol.ts` - TypeScript closed schema parsers and bridge state model to extend for licensing state.
- `ui/src/bridge/BridgeProvider.tsx` - Existing native event listeners and request helper pattern.
- `ui/src/app/AppShell.tsx` - Current analyzer-first shell layout and compact standalone strip integration point.
- `ui/src/components/StatusFooter.tsx` - Current compact status/diagnostic footer pattern that may inform licensing status placement.

### Prior Native and UI Decisions

- `.planning/phases/02-end-to-end-vst3-analyzer/02-CONTEXT.md` - Real-time audio/thread boundaries, latest-snapshot handoff, analyzer-first UI, and explicit licensing deferral.
- `.planning/phases/03-standalone-windows-monitoring/03-CONTEXT.md` - Standalone UI compact strip pattern, source-state bridge additions, and no-surprise failure-state decisions.
- `plugin/include/LumaScope/PluginProcessor.h` - Processor ownership and audio callback boundary where licensing status must be non-blocking/immutable.
- `plugin/source/PluginProcessor.cpp` - Current audio processing path that licensing work must not block or modify.
- `plugin/include/LumaScope/PluginEditor.h` - Editor/WebView ownership and message-thread polling boundary.
- `plugin/source/PluginEditor.cpp` - Existing WebView setup, bridge readiness, timer callback, and diagnostic patterns.

### External Documentation Sources

- Context7 library `/websites/juce_master` - Use for JUCE application data locations, threading/timer/message-manager patterns, URL/network helpers if applicable, and Windows target behavior.
- Context7 library `/janwilczek/juce-webview-tutorial` - Use when extending JUCE WebView native/web communication or embedded asset behavior.
- Context7 library `/websites/mui_material-ui` - Use when adding Material UI activation dialogs, status controls, accessible forms, and warning states.
- First-party Microsoft documentation for Windows protected storage / DPAPI or selected credential-storage APIs - Use for local entitlement storage security decisions.
- First-party Windows documentation for any selected machine-identity inputs - Use to document stability, privacy, and limitations.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets

- Phase 5 activation contracts already define `licenseKey`, `machineId`, `requestId`, `timestamp`, optional `appVersion`, and success envelopes containing `activationId`, signed entitlement token, `expiresAt`, `serverTime`, and `refreshAfter`.
- Worker signing helpers already define the token claims native code must verify: `schemaVersion`, `licenseKeyHash`, `activationId`, `machineId`, `productId`, `variantId`, `status`, `issuedAt`, `refreshAfter`, `expiresAt`, and `kid`.
- The React bridge already handles native event subscriptions, parser rejection for malformed payloads, and request helper emission for standalone source actions.
- The native `HostBridge` already centralizes protocol-v1 event IDs, bounded string handling, and `juce::var` payload construction.
- The existing app shell has a compact analyzer-first layout with header, optional strip, analyzer stage, and footer.

### Established Patterns

- Native/web payloads are closed, typed, versioned, and bounded on both sides.
- WebView calls and JSON construction happen on non-audio paths only; audio callbacks publish only through bounded native handoff models.
- UI status surfaces are concise and actionable, with copy diagnostics available for failure cases.
- Provider secrets and signing private keys remain server-side; native code embeds only public keys or non-secret metadata.

### Integration Points

- Add a native licensing core that can be tested independently of VST3 hosts and WebView.
- Add a non-real-time activation client that calls Phase 5 endpoints and never runs from an audio callback.
- Add protected local entitlement storage under the user's application-data area and expose immutable/atomic licensing status to processor/editor/UI layers.
- Extend `HostBridge`, `docs/bridge-protocol.md`, `ui/src/bridge/protocol.ts`, and `BridgeProvider` with licensing status events and activation/deactivation request events.
- Add compact React/MUI activation status controls and dialogs in the existing shell without replacing the analyzer stage.
- Extend native, frontend, Worker fixture, and end-to-end scripts so cross-language token verification, grace boundaries, transfer, corruption, outage, and revocation behavior are proven.

</code_context>

<specifics>
## Specific Ideas

- The identity approach should balance persistence and privacy rather than being purely app-random or strictly hardware-bound.
- Offline use should feel normal while valid, but the app should not hide that it is operating in a grace window.
- Activation should be available and visible without taking over the analyzer UI.
- Deactivation should fail safely: no local entitlement deletion unless the server confirms transfer is complete.

</specifics>

<deferred>
## Deferred Ideas

None - discussion stayed within Phase 6 scope.

</deferred>

---

*Phase: 6-Native Offline Licensing*
*Context gathered: 2026-06-23T17:05:08.2833635-07:00*
