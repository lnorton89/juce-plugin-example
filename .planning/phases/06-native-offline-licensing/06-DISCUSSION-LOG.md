# Phase 6: Native Offline Licensing - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-06-23T17:05:08.2833635-07:00
**Phase:** 6-Native Offline Licensing
**Areas discussed:** Machine identity, local entitlement and offline grace, activation UI placement and tone, transfer and failure behavior

---

## Machine Identity

| Option | Description | Selected |
|--------|-------------|----------|
| Balanced fingerprint | Derive from several Windows/device signals, hash locally, never send raw inputs. More stable across ordinary changes, with documented limitations. | yes |
| Conservative app-local identity | Generate and store a random local machine secret, then derive the machine ID from that. Very privacy-friendly, but reinstall/profile loss can consume a new activation. | |
| Strict hardware-derived identity | Bind more tightly to hardware/OS identifiers. Better abuse resistance, but more false lockouts and more privacy sensitivity. | |
| Other | User-provided preference. | |

**User's choice:** Balanced fingerprint.
**Notes:** This should preserve privacy by transmitting only a derived identifier while being more durable than an app-local random secret.

---

## Local Entitlement and Offline Grace

| Option | Description | Selected |
|--------|-------------|----------|
| Grace is generous but visible | Launch normally while token is valid and within grace, show clear offline/grace status, warn as expiry approaches, require online validation after day 7. | yes |
| Grace is quiet until near expiry | Mostly invisible while valid, then warn only in the final stretch. Less intrusive, but users may be surprised when revalidation becomes required. | |
| Grace is strict and prominent | Always show a licensing warning whenever offline. Clear enforcement posture, but can feel noisy for normal use. | |
| Other | User-provided preference. | |

**User's choice:** Grace is generous but visible.
**Notes:** Offline operation should remain functional and calm, while the app stays honest about grace state and upcoming revalidation.

---

## Activation UI Placement and Tone

| Option | Description | Selected |
|--------|-------------|----------|
| Compact status/control surface | Keep licensing in a compact top/header or footer-adjacent control, with a modal/dialog for entering keys and deactivation confirmation. Preserves the analyzer-first feel. | yes |
| Dedicated activation panel | Add a larger settings-style panel or view for license management. Clearer for setup, but heavier than the current focused instrument UI. | |
| Blocking activation screen | Show activation as the main surface until licensed. Strong enforcement, but risks making the example feel less usable and less developer-friendly. | |
| Other | User-provided preference. | |

**User's choice:** Compact status/control surface.
**Notes:** Licensing should be visible and actionable without displacing the analyzer stage.

---

## Transfer and Failure Behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Server-authoritative transfer, local-safe failure | Clear local entitlement only after successful server deactivation; if refresh fails because the service is unavailable, continue within remaining grace; if server says revoked/expired/deactivated, show reactivation-required and stop relying on the token. | yes |
| Optimistic local transfer | Clear local entitlement immediately when user requests deactivation, then try the server. Feels responsive, but can strand users if the network/server fails. | |
| Always preserve local entitlement until replacement activation | Safest for local access, but conflicts with clear one-machine transfer semantics. | |
| Other | User-provided preference. | |

**User's choice:** Server-authoritative transfer, local-safe failure.
**Notes:** Transfer should not delete local entitlement state until the server confirms deactivation; outages should consume remaining grace, while authoritative negative server responses override local activated status.

---

## the agent's Discretion

- Exact Windows identity inputs and local derivation format.
- Exact protected storage mechanism and on-disk layout.
- Exact native class boundaries, retry/backoff policy, and bridge event naming.
- Exact UI copy and visual severity, within the compact analyzer-first surface.

## Deferred Ideas

None.
