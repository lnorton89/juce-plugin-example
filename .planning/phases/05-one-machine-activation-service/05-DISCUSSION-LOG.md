# Phase 5: One-Machine Activation Service - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-06-23T16:04:31-07:00
**Phase:** 05-One-Machine Activation Service
**Areas discussed:** Activation API contract, One-machine transfer policy, Signing and key rotation, Abuse controls and audit detail

---

## Activation API Contract

| Question | Selected | Alternatives Considered |
|----------|----------|-------------------------|
| Endpoint shape | Versioned REST endpoints | Separate REST endpoints; single action endpoint |
| Intended client proof | License-key + machine proof only | Public client ID; embedded shared secret |
| Activation success response | Full signed entitlement envelope | Token only; activation record plus extra license details |
| Error style | Stable machine-readable codes with generic messages | Detailed server messages; HTTP status only |
| Request fields | Minimal required fields | Machine metadata; challenge/nonce handshake |

**User's choices:** 2, 1, 1, 1, 1.
**Notes:** User explicitly emphasized protecting the API from callers other than the plug-in. The captured decision avoids an embedded secret and uses layered practical protections instead.

---

## One-Machine Transfer Policy

| Question | Selected | Alternatives Considered |
|----------|----------|-------------------------|
| Different-machine activation when one is already active | Reject and require deactivation first | Auto-transfer; emergency transfer window |
| Same-machine duplicate activation | Idempotent success with fresh entitlement | Already-active without token; reject duplicate |
| Deactivation proof | License key + same machine ID | Signed entitlement token + machine ID; license key only |
| Validation when activation is missing/deactivated | Reject with `activation_not_found` | Recreate automatically; valid with no refreshed token |

**User's choices:** 1, 1, 1, 1.
**Notes:** Phase 5 policy is strict one-active-machine with explicit self-service transfer. Lost-machine recovery remains outside Phase 5 unless a future phase adds it.

---

## Signing and Key Rotation

| Question | Selected | Alternatives Considered |
|----------|----------|-------------------------|
| Token format | Canonical JSON envelope + detached Ed25519 signature | Compact JWT-style token; opaque signed blob |
| Token validity | `issuedAt`, `refreshAfter`, and `expiresAt` | Long-lived token only; server-session style token |
| Signed payload contents | Minimal licensing claims only | Customer/license display fields; server audit metadata |
| Key representation | Active private key secret + non-secret public key ring | Single pair only; multiple private keys in one secret |

**User's choices:** 1, 1, 1, 1.
**Notes:** The token is intended for Phase 6 offline verification with cross-language fixtures and no customer PII.

---

## Abuse Controls and Audit Detail

| Question | Selected | Alternatives Considered |
|----------|----------|-------------------------|
| Rate-limit keys | Layered route + IP, license hash, machine hash | IP only; license key only |
| Audit contents | Redacted structured events | Minimal events; verbose request diagnostics |
| Malformed request behavior | Fail early with generic errors | Lenient parsing; ad hoc route checks |
| Deployed smoke coverage | Happy path plus policy failures | Happy path only; no deployed smoke |

**User's choices:** 1, 1, 1, 1.
**Notes:** Audit and abuse decisions prioritize support/debuggability without storing raw secrets, raw license keys, raw machine IDs, or request bodies.

---

## the agent's Discretion

- Exact canonical JSON implementation details.
- Exact timestamp skew and body-size bounds.
- Exact rate-limit thresholds and binding names.
- Exact D1 concurrency strategy, provided one-active-machine policy is enforced.

## Deferred Ideas

None.
