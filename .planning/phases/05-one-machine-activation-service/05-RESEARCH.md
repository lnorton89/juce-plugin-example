# Phase 05: One-Machine Activation Service - Research

**Researched:** 2026-06-23  
**Domain:** Cloudflare Worker activation API, D1 one-seat policy, Ed25519 entitlement signing, abuse controls  
**Confidence:** HIGH for Worker/D1 integration and API boundaries, MEDIUM for exact D1 concurrency mechanics until implemented and tested against local/remote D1.

## Summary

Phase 5 should extend the existing Cloudflare Worker/D1 foundation from Phase 4 rather than introducing a framework or external service. The current Worker already uses `worker/src/routes.ts` for route dispatch, `Repository` for D1 access, `webhook.ts` for validation/audit patterns, and Vitest with an in-memory D1 mock. The activation service should add versioned endpoints under `/api/v1`, define a stable request/error/token contract, enforce one active machine through repository methods and D1 constraints, and issue canonical JSON entitlement envelopes with detached Ed25519 signatures.

The API cannot be restricted solely to “the plug-in” in a cryptographic sense because any secret embedded in distributed client software can be extracted. The practical protection model is layered: valid license key, bounded derived machine identifier, request ID/timestamp replay checks, one-machine state in D1, rate limits, generic errors, redacted audits, and signed server-issued entitlements.

## Current Worker Shape

| Area | Existing Asset | Phase 5 Use |
|------|----------------|-------------|
| Routing | `worker/src/routes.ts` | Add `POST /api/v1/activate`, `/validate`, `/deactivate`. |
| Env | `worker/src/env.ts` | Add signing metadata/rate-limit bindings if required by implementation. |
| D1 repository | `worker/src/db/repository.ts` | Add activation mutation, idempotency/replay, validate, deactivate, audit helpers. |
| Schema | `worker/src/db/migrations/0001_create_tables.sql` | Reuse `licenses`, `activations`, `audit_log`; add migration only if request idempotency/key-ring persistence needs schema. |
| Tests | `tests/worker/*` | Extend mock D1 for activation mutations, race tests, signing fixtures, and route tests. |
| Infra | `infra/manifest.yaml`, `worker/wrangler.toml` | Add required signing/rate-limit config without secrets in Git. |

## Cloudflare Guidance

- Workers best practices require request-scoped state to be passed through function arguments, not stored in module-level mutable globals.
- Promises must be awaited, returned, or passed to `ctx.waitUntil`; audit writes after responses should use `ctx.waitUntil` only when failure is non-critical.
- Security-sensitive IDs and signatures should use Web Crypto, not `Math.random` or ad hoc crypto.
- Explicit `try/catch` with structured generic errors is preferred over pass-through exception behavior.
- Cloudflare's Worker Rate Limiting binding supports route/customer keyed rate limits and can use stable application keys such as route plus license/machine hashes. It is fast but local/eventually consistent, so it should be abuse friction, not the only enforcement mechanism.

Sources:
- Cloudflare Workers best practices: https://developers.cloudflare.com/workers/best-practices/workers-best-practices/
- Cloudflare Workers Rate Limiting binding: https://developers.cloudflare.com/workers/runtime-apis/bindings/rate-limit/

## API Contract Research

Versioned REST endpoints keep native integration explicit and allow future contract evolution:

- `POST /api/v1/activate`
- `POST /api/v1/validate`
- `POST /api/v1/deactivate`

Requests should be JSON only, bounded, and validated before any D1 mutation. Common request controls:

- `requestId`: bounded UUID-like or opaque unique string
- `timestamp`: ISO string or Unix milliseconds with skew bound
- `licenseKey`: present for activate/deactivate/validate
- `machineId`: derived privacy-conscious machine identifier from Phase 6; Phase 5 treats it as opaque bounded text
- `appVersion`: optional bounded diagnostic field

Responses should use stable error codes with generic messages so the native UI can map them later:

- `invalid_request`
- `license_not_found`
- `license_inactive`
- `activation_limit_reached`
- `activation_not_found`
- `machine_mismatch`
- `rate_limited`
- `replay_detected`
- `server_error`

## One-Machine Policy Research

The existing `activations` table has `UNIQUE(license_id, machine_identifier)` and an active index. That supports idempotent same-machine activation and active-machine lookup, but D1 transaction behavior should be tested carefully for race cases. Planning should include repository-level tests for:

- No active activation -> create activation
- Same active machine -> update validation timestamp and return fresh entitlement
- Different active machine -> reject `activation_limit_reached`
- Deactivate matching active machine -> set inactive and timestamp
- Validate matching active machine -> refresh entitlement
- Validate missing/deactivated activation -> reject `activation_not_found`
- Concurrent activation attempts -> at most one active machine

If D1 cannot guarantee the desired race behavior with the current schema alone, the planner may add a migration with a partial unique index or an activation policy helper table, but the simplest path should be attempted first.

## Signing Research

Use Web Crypto Ed25519 where supported by the Worker runtime. The service should keep `SIGNING_PRIVATE_KEY` as a Worker secret and expose only public keys through docs/fixtures/config. The signed entitlement should be a deterministic canonical JSON object plus detached signature:

```json
{
  "payload": { "schemaVersion": 1, "activationId": "...", "kid": "..." },
  "signature": "base64url-signature"
}
```

Canonicalization must be identical across Worker tests and future native verification. The planner should either implement a small deterministic canonical JSON serializer for allowed plain objects or define exact field ordering and string encoding for the token payload.

## Abuse and Audit Research

Audit records should be structured and redacted. Use hashes for license key and machine ID. The raw license key, raw machine ID, secrets, and request body must not be logged. Layered rate limiting should combine:

- route plus coarse client IP
- route plus license key hash when parseable
- route plus machine ID hash when parseable

Rate limiting should happen after cheap shape parsing but before expensive D1/signing work where possible. D1 remains the source of truth for one-machine policy.

## Planning Recommendation

Create three plans matching the roadmap:

1. `05-01`: Define contracts, signing primitives, canonical token fixtures, request validation, and error model.
2. `05-02`: Implement repository methods and versioned activate/validate/deactivate endpoints with one-machine policy.
3. `05-03`: Add rate limits, redacted audit helpers, concurrency/failure tests, deployed/local smoke tests, and docs.

## RESEARCH COMPLETE
