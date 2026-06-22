# Pitfalls Research

**Domain:** JUCE/WebView real-time analyzer with Cloudflare licensing
**Researched:** 2026-06-22
**Confidence:** HIGH

## Critical Pitfalls

### 1. Real-Time Thread Contamination

**What goes wrong:** Hosts glitch, hang, or fail validation because processing allocates, locks, logs, posts UI events, reads files, or performs network work.

**How to avoid:** Preallocate; use bounded SPSC handoff; keep FFT off the callback when workload warrants; make license/UI state atomic snapshots; test variable block sizes and sample rates.

**Warning signs:** Mutexes, `new`, filesystem, HTTP, JSON, WebView callbacks, or unbounded copies reachable from `processBlock`/device callbacks.

**Phase to address:** Native foundation and analyzer core.

### 2. Incorrect or Misleading Spectrum Math

**What goes wrong:** Peaks appear at wrong amplitudes/frequencies, DC/Nyquist bins are mishandled, stereo cancellation hides content, or display changes with FFT size.

**How to avoid:** Define window, coherent-gain normalization, single-sided scaling, channel policy, dB floor, smoothing, and bin interpolation; verify with deterministic tones and silence.

**Warning signs:** “Looks plausible” is the only test; no assertions across sample rates/FFT orders.

**Phase to address:** Analyzer core.

### 3. Treating WASAPI Loopback as an Ordinary Input

**What goes wrong:** System audio is absent, device changes crash capture, or exclusive-mode assumptions fail.

**How to avoid:** Isolate a native shared-mode loopback adapter; handle format conversion, default-device changes, invalidation, silence, and restart. Keep normal JUCE input separately selectable.

**Warning signs:** Depending on “Stereo Mix,” or assuming every output endpoint appears as a capture device.

**Phase to address:** Standalone capture.

### 4. WebView Bridge Flooding or Drift

**What goes wrong:** UI consumes excessive CPU, lags, or silently breaks after one side changes message shape.

**How to avoid:** Latest-wins snapshots at bounded FPS, compact typed payloads, versioned schemas, contract fixtures, and mock bridge tests. Render graph via Canvas/WebGL rather than MUI/DOM nodes per bin.

**Warning signs:** Every audio block emits JSON; bridge types are duplicated manually with no fixtures.

**Phase to address:** WebView UI integration.

### 5. Activation Race Violates One-Machine Policy

**What goes wrong:** Two concurrent requests both observe zero activations and create two seats.

**How to avoid:** Enforce uniqueness/invariants in schema and transactional/batched write logic; design idempotent activation keys; test concurrency.

**Warning signs:** “SELECT count then INSERT” with no database constraint or conflict handling.

**Phase to address:** Cloud activation backend.

### 6. Invalid Webhook Trust Boundary

**What goes wrong:** Forged or replayed Lemon events grant/revoke licenses.

**How to avoid:** Verify `X-Signature` using HMAC-SHA256 over exact raw bytes before parsing; use constant-time comparison; deduplicate stable event identifiers; validate product/variant/store IDs.

**Warning signs:** JSON is reserialized before verification, signature failures are logged but accepted, or any product event creates entitlement.

**Phase to address:** Cloud activation backend.

### 7. Offline Grace Becomes Permanent Bypass or Outage Trap

**What goes wrong:** Local clock rollback extends licenses forever, copied files activate another machine, or a transient outage blocks valid users.

**How to avoid:** Signed canonical payload with machine/product/issued/expiry/key ID; record monotonic-ish evidence such as max-seen wall time; define grace from last successful validation; distinguish provider outage from explicit revocation; rotate keys with `kid` support.

**Warning signs:** Unsigned JSON, only local timestamps, no key rotation story, or no tests around boundary dates.

**Phase to address:** Client licensing integration.

### 8. “Portable” Cloud Setup Still Contains Hidden Click-Ops

**What goes wrong:** A fork cannot deploy because D1 IDs, routes, secrets, webhook URLs, or dashboard toggles were never captured.

**How to avoid:** Logical YAML manifest, checked-in Wrangler config template, SQL migrations, bootstrap/deploy/verify/teardown scripts, explicit secret contract, generated gitignored account overlay, and fresh-account rehearsal.

**Warning signs:** README says “configure the rest in dashboard,” or committed config contains the author's account/D1 UUID.

**Phase to address:** Cloud foundation before backend features.

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| FFT in editor timer | Easy demo | Couples correctness/lifetime to open UI | Throwaway spike only |
| Raw structs over bridge | Minimal mapping | ABI/schema drift and unsafe parsing | Never |
| One Worker route file | Fast start | Security, policy, and storage become untestable | Initial skeleton only |
| Hard-coded machine hash inputs | Quick binding | Hardware upgrades lock out customers | Never without versioning/recovery |
| Manually created D1 tables | Fast local test | No reproducibility or upgrades | Never after first spike |

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| JUCE standalone target | Assuming generated standalone wrapper covers custom loopback UX | Share core but own a focused standalone shell/source abstraction |
| WebView2 | Assuming runtime exists and blank page is self-explanatory | Detect backend/runtime failure and show native diagnostics |
| Lemon Squeezy | Treating webhook delivery as exactly once | Idempotent event table and safe retries |
| Lemon License API | Calling per UI frame/launch burst | Cache server-side results and honor 60/min documented limit |
| D1 | Relying only on application-level count checks | Unique constraints and conflict-safe writes |
| Cloudflare secrets | Putting values in YAML/Wrangler | Commit names only; inject with `wrangler secret`/secret store |

## Security Mistakes

| Mistake | Risk | Prevention |
|---------|------|------------|
| Private signing key in client or Git | Universal license forgery | Worker secret only; embed public keys |
| Plain license key in logs/D1 indexes | Credential leakage | Redact logs; store keyed/hash lookup where practical |
| Unbounded activation endpoint | Brute force and cost abuse | Per-IP/key limits, generic errors, structured audit events |
| Deactivation authenticated only by machine ID | Seat theft | Require license proof/token and matching activation claims |
| Overpowered Cloudflare token | Account compromise | Document exact minimal scopes and support token rotation |
| No canonical token serialization | Signature ambiguity | Versioned canonical bytes and cross-language fixtures |

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| Blank UI when unlicensed/WebView fails | User thinks plug-in crashed | Native fallback and explicit activation/diagnostic state |
| Device disappears mid-session | Analyzer freezes mysteriously | Detect invalidation, stop safely, offer reselection/retry |
| “Activation failed” only | Support burden | Stable error codes with actionable, non-sensitive messages |
| Seven-day cutoff without warnings | Surprise interruption | Show remaining offline window before expiry |
| Transfer deletes local state before server success | User loses working license | Commit server deactivation first, then clear local token |

## “Looks Done But Isn’t” Checklist

- [ ] **DSP:** Verify tones, silence, stereo policy, sample rates, block sizes, and UI-closed operation.
- [ ] **VST3:** Validate in pluginval and at least one real DAW; confirm unchanged audio.
- [ ] **Standalone:** Test input/loopback, hot unplug, default-device changes, and no-active-audio periods.
- [ ] **WebView:** Test packaged assets, DPI/resize, runtime absence, bridge mismatch, and CSP.
- [ ] **Licensing:** Test concurrent activation, transfer, expiry, revocation, clock rollback, corrupt file, key rotation, and provider outage.
- [ ] **Cloud handoff:** Provision from a clean Cloudflare account with only documented prerequisites.

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Real-time contamination | Native foundation/analyzer | Static review plus stress tests/pluginval |
| Spectrum math | Analyzer core | Deterministic signal tests |
| Loopback lifecycle | Standalone capture | Device matrix and invalidation tests |
| Bridge flooding/drift | WebView UI | Contract tests and frame/CPU budgets |
| Activation races/webhook trust | Cloud backend | Concurrency and signed-payload tests |
| Offline bypass | Client licensing | Cross-language token and time-boundary tests |
| Hidden click-ops | Infrastructure | Fresh-account deployment rehearsal |

## Sources

- Context7 JUCE/WebView/MUI documentation
- [Microsoft WASAPI loopback recording](https://learn.microsoft.com/windows/win32/coreaudio/loopback-recording)
- [Lemon webhook signatures](https://docs.lemonsqueezy.com/help/webhooks/signing-requests)
- [Lemon License API](https://docs.lemonsqueezy.com/api/license-api)
- Cloudflare Workers, D1 migrations, Wrangler, and infrastructure-as-code documentation

---
*Pitfalls research for: JUCE Spectrum Analyzer Example*
*Researched: 2026-06-22*
