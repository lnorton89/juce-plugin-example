# Phase 5: One-Machine Activation Service - Context

**Gathered:** 2026-06-23T16:04:31-07:00
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 5 delivers the Cloudflare Worker activation service for one-seat licensing: versioned activation, validation, and deactivation endpoints; concurrency-safe one-active-machine policy; Ed25519-signed entitlement issuance; request validation, replay resistance, rate limits, redacted audit records, and Worker tests. Native machine identity derivation and local token storage remain Phase 6.

</domain>

<decisions>
## Implementation Decisions

### Activation API Contract

- **D-01:** Use versioned REST endpoints: `POST /api/v1/activate`, `POST /api/v1/validate`, and `POST /api/v1/deactivate`.
- **D-02:** Do not embed an app secret in the plug-in. API protection is license-key plus machine proof, bounded request fields, request IDs/timestamps/nonces, replay checks, rate limits, generic errors, and audit.
- **D-03:** Activation success returns a full signed entitlement envelope: `activationId`, `entitlementToken`, `tokenType`, `expiresAt`, `serverTime`, and `refreshAfter`.
- **D-04:** Errors use stable machine-readable codes with generic messages. Native/UI maps codes to friendly text later.
- **D-05:** Activation request fields are minimal: `licenseKey`, `machineId`, `requestId`, `timestamp`, and optional `appVersion`. Phase 6 owns how `machineId` is derived.

### One-Machine Transfer Policy

- **D-06:** If a license already has one active different machine, activation rejects with `activation_limit_reached`. No automatic transfer in Phase 5.
- **D-07:** If the same active machine activates again with the same license, return idempotent success with a fresh signed entitlement envelope for the existing activation.
- **D-08:** `POST /api/v1/deactivate` requires `licenseKey`, the same `machineId`, `requestId`, and `timestamp`; it only deactivates the matching active activation.
- **D-09:** `POST /api/v1/validate` refreshes an existing active activation only. If server activation is missing or deactivated, reject with `activation_not_found`; validation must not recreate activations.

### Signing and Key Rotation

- **D-10:** Use a canonical JSON entitlement envelope plus detached Ed25519 signature with `kid`, not JWT or opaque blobs.
- **D-11:** Token validity uses `issuedAt`, `refreshAfter`, and `expiresAt`, with a short refresh cadence inside the longer offline entitlement window.
- **D-12:** Signed payload contains minimal licensing claims only: `schemaVersion`, `licenseKeyHash`, `activationId`, `machineId`, `productId`, `variantId`, `status`, `issuedAt`, `refreshAfter`, `expiresAt`, and `kid`.
- **D-13:** Do not include customer name/email, raw license key, full request bodies, IP details, webhook lineage, or audit metadata in the local signed token.
- **D-14:** `SIGNING_PRIVATE_KEY` holds the active private key. A non-secret public key ring maps `kid -> publicKey` for fixtures, docs, and future rotation.

### Abuse Controls and Audit Detail

- **D-15:** Use layered rate limit keys: route plus IP, license key hash, and machine ID hash where available.
- **D-16:** Audit logs store redacted structured events only: action, outcome code, license key hash, machine ID hash, activation ID when known, request ID, route, coarse client info, and timestamp.
- **D-17:** Never store raw license keys, raw machine IDs, secrets, or full request bodies in audit records.
- **D-18:** Fail malformed or oversized requests before D1 mutation: method/content type, maximum body size, schema validation, string length limits, timestamp skew, and request ID format.
- **D-19:** Deployed smoke tests should cover happy path and policy failures: activate, same-machine retry, validate, deactivate, second-machine rejection, inactive license rejection, malformed request rejection, and rate-limit behavior where locally/deployed feasible.

### the agent's Discretion

The planner may choose exact JSON canonicalization mechanics, timestamp skew bounds, body-size limits, route helper structure, D1 transaction/query strategy, and concrete rate-limit thresholds, provided the decisions above and Worker best practices are preserved.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project and Phase Scope

- `.planning/PROJECT.md` - Product constraints, licensing policy, privacy/security posture, and core value.
- `.planning/REQUIREMENTS.md` - Phase 5 requirements `CLOUD-04` through `CLOUD-10`.
- `.planning/ROADMAP.md` - Phase 5 goal, success criteria, and planned waves.
- `.planning/STATE.md` - Current project state and accumulated prior decisions.
- `.planning/phases/04-portable-purchase-infrastructure/04-04-SUMMARY.md` - Phase 4 handoff: Worker/D1 foundation, webhook ingestion, infrastructure verification.

### Existing Worker and Data Model

- `worker/src/routes.ts` - Current route switch with `/api/health` and Lemon webhook route; Phase 5 endpoints connect here.
- `worker/src/index.ts` - Current Worker entry, CORS headers, and top-level error handling.
- `worker/src/env.ts` - Current Env contract including `ACTIVATION_DB` and `SIGNING_PRIVATE_KEY`.
- `worker/src/db/schema.ts` - License, activation, webhook idempotency, and audit types.
- `worker/src/db/repository.ts` - Existing repository operations and `findActiveActivations` stub surface.
- `worker/src/db/migrations/0001_create_tables.sql` - Current D1 schema, unique activation constraint, active activation index, and audit table.
- `worker/src/webhook.ts` - Existing request validation, exact-body HMAC verification, idempotency, and audit patterns.
- `tests/worker/repository.test.ts` - Existing repository and activation stub tests.
- `tests/worker/setup.ts` - Mock D1 implementation that Phase 5 tests may need to extend.

### Infrastructure and Operations

- `infra/manifest.yaml` - Secret contracts and signing key placeholders.
- `worker/wrangler.toml` - Worker/D1 environment bindings and deployment shape.
- `docs/cloud-infrastructure.md` - Cloudflare/Lemon provisioning and deployment handoff.

### Current Cloudflare Guidance

- `https://developers.cloudflare.com/workers/best-practices/workers-best-practices/` - Use Web Crypto, no request state in globals, await or `ctx.waitUntil` promises, explicit error handling, bindings over REST APIs, structured logs.
- `https://developers.cloudflare.com/workers/runtime-apis/bindings/rate-limit/` - Workers Rate Limiting API, route/customer keyed limits, locality/accuracy caveats, Wrangler 4.36+ requirement.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets

- `Repository` already finds licenses by key/order, writes audit entries, records webhook idempotency, and lists active activations.
- `activations` table already has `UNIQUE(license_id, machine_identifier)` and an index for active activations by license.
- `audit_log` table already has action/resource/request fields but may need stricter structured redaction conventions.
- Worker tests already use Vitest plus an in-memory `MockD1Database` that can be expanded for activation mutations and race simulations.
- `webhook.ts` already demonstrates content-type checks, maximum webhook body size, generic error responses, configured product validation, idempotency checks, and audit writes.

### Established Patterns

- Routes are currently a simple method/path switch in `worker/src/routes.ts`; Phase 5 can extend this without adopting a framework.
- Errors are generic JSON responses rather than leaking internals.
- Secret values live in Worker environment bindings and are declared in `infra/manifest.yaml`, never committed.
- Existing tests use deterministic fixtures and in-memory D1 mocks rather than live Cloudflare resources for ordinary unit coverage.

### Integration Points

- Add `/api/v1/activate`, `/api/v1/validate`, and `/api/v1/deactivate` to `worker/src/routes.ts`.
- Extend `Env` for rate-limit bindings and key-ring/current-key metadata if selected by the planner.
- Extend migrations/repository to support atomic activation/deactivation/validation updates and replay/idempotency for client requests.
- Extend `scripts/test-all.ps1`, Worker tests, and deployment smoke scripts if Phase 5 adds new verification commands.

</code_context>

<specifics>
## Specific Ideas

- The API should be protected against direct third-party callers as much as practical, but planning must remain honest: a distributed plug-in cannot keep an embedded shared secret confidential forever.
- Versioned endpoints are preferred now so Phase 6 native integration has a stable explicit contract.
- Keep local entitlement tokens private and minimal; user-facing licensing copy belongs in Phase 6 UI mapping, not the server token.

</specifics>

<deferred>
## Deferred Ideas

None - discussion stayed within Phase 5 scope.

</deferred>

---

*Phase: 05-One-Machine Activation Service*
*Context gathered: 2026-06-23T16:04:31-07:00*
