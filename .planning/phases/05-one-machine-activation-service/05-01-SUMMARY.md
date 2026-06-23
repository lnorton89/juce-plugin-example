---
phase: 05-one-machine-activation-service
plan: 01
subsystem: api
tags: [cloudflare-workers, activation-api, ed25519, canonical-json, licensing]

requires:
  - phase: 04-portable-purchase-infrastructure
    provides: Cloudflare Worker, D1 schema, Lemon Squeezy webhook ingestion, and infrastructure manifest
provides:
  - Versioned v1 activation API request, response, and error contracts
  - Reusable bounded JSON request validation for activation endpoints
  - Deterministic canonical JSON and Ed25519 detached entitlement signing helpers
  - Cross-language entitlement fixture for native verifier work in Phase 6
  - Activation API documentation and signing environment contract
affects: [05-one-machine-activation-service, 06-native-offline-licensing, cloud-infrastructure]

tech-stack:
  added: []
  patterns:
    - Stable machine-readable activation errors with generic response messages
    - Canonical JSON plus detached Ed25519 signatures for local entitlement verification
    - Server-side private signing key with non-secret public key ring metadata

key-files:
  created:
    - docs/activation-api.md
    - tests/worker/contracts.test.ts
    - tests/worker/fixtures/entitlement-v1.json
    - tests/worker/signing.test.ts
    - worker/src/api/contracts.ts
    - worker/src/api/errors.ts
    - worker/src/api/requestValidation.ts
    - worker/src/signing/canonicalJson.ts
    - worker/src/signing/entitlement.ts
  modified:
    - infra/manifest.yaml
    - worker/src/env.ts

key-decisions:
  - "Activation clients do not receive or embed an app secret; protection is license key plus machine proof, bounded request validation, replay/rate controls, audit, and signed server entitlements."
  - "Entitlements use deterministic canonical JSON and detached Ed25519 signatures with kid metadata rather than JWT or opaque blobs."
  - "Signed entitlement payloads contain minimal licensing claims only and exclude raw license keys, customer PII, request bodies, IP details, and audit metadata."

patterns-established:
  - "Activation endpoint contracts live in worker/src/api/contracts.ts and are mirrored by docs/activation-api.md."
  - "Activation errors are created only through worker/src/api/errors.ts to keep stable codes and generic messages."
  - "Cross-language signing compatibility is locked by tests/worker/fixtures/entitlement-v1.json."

requirements-completed:
  - CLOUD-04
  - CLOUD-08
  - CLOUD-09

duration: 25 min
completed: 2026-06-23
---

# Phase 05 Plan 01: Activation Contract and Signing Summary

**Versioned activation API contracts with bounded validation and canonical Ed25519 entitlement signing**

## Performance

- **Duration:** 25 min
- **Started:** 2026-06-23T23:06:00Z
- **Completed:** 2026-06-23T23:30:50Z
- **Tasks:** 3
- **Files modified:** 11

## Accomplishments

- Defined `/api/v1/activate`, `/api/v1/validate`, and `/api/v1/deactivate` request/response contracts with stable error envelopes.
- Added reusable request validation for JSON method/content-type/body-size/schema/string-length/requestId/timestamp checks before endpoint policy or D1 mutation work.
- Added deterministic canonical JSON, Ed25519 signing/verification helpers, base64url key encoding helpers, and a v1 entitlement fixture for Phase 6 native verification.
- Documented the activation API, no-app-secret boundary, minimal signed claims, and signing key environment contract.

## Task Commits

1. **Task 1-3: Contracts, validation, signing, docs, and fixtures** - `b174979` (feat)

**Plan metadata:** pending in this summary commit

## Files Created/Modified

- `worker/src/api/contracts.ts` - Versioned activation request, response, and entitlement token types.
- `worker/src/api/errors.ts` - Stable activation error code list, status mapping, and generic error envelope helpers.
- `worker/src/api/requestValidation.ts` - Shared bounded JSON request validation for activation endpoints.
- `worker/src/signing/canonicalJson.ts` - Deterministic canonical JSON serializer and byte encoder.
- `worker/src/signing/entitlement.ts` - Entitlement claim builder, Ed25519 sign/verify helpers, key import, base64url, and public key ring parser.
- `tests/worker/contracts.test.ts` - Contract, error, and request validation tests.
- `tests/worker/signing.test.ts` - Canonical JSON, signing fixture, verification, and public key ring tests.
- `tests/worker/fixtures/entitlement-v1.json` - Fixed v1 entitlement payload, canonical string, signature, and key ring fixture.
- `docs/activation-api.md` - Human-readable v1 activation API and signing contract.
- `infra/manifest.yaml` - Added signing key ID and public key ring configuration entries.
- `worker/src/env.ts` - Added signing key ID and public key ring environment fields.

## Decisions Made

- No embedded app secret was added because distributed client secrets are extractable; Phase 5 uses layered server-side controls instead.
- Canonical JSON plus detached Ed25519 was implemented as the local entitlement format so Phase 6 can verify tokens without a network dependency.
- The signing fixture intentionally excludes raw license keys and customer PII from signed claims while still documenting request examples separately.

## Deviations from Plan

None - plan executed exactly as written.

**Total deviations:** 0 auto-fixed.
**Impact on plan:** No scope change.

## Issues Encountered

- The initial contract test helper tried to send a body with a GET request and accidentally replaced Content-Type while adding Content-Length. Fixed the helper and reran targeted tests successfully.

## Verification

- `npm.cmd --prefix worker run typecheck` - passed.
- `npm.cmd --prefix worker run test -- contracts.test.ts signing.test.ts` - passed, 11 tests.
- `npm.cmd --prefix worker run test` - passed, 44 tests.
- Fixture inspection confirmed no raw license key or customer PII in `tests/worker/fixtures/entitlement-v1.json`.

## User Setup Required

None - no external service configuration required for this plan. Future deployed activation work must provide `SIGNING_PRIVATE_KEY`, `SIGNING_KEY_ID`, and `SIGNING_PUBLIC_KEYS`.

## Next Phase Readiness

Ready for 05-02. Endpoint implementation can consume the request validators, error helpers, and entitlement signer without revisiting the v1 contract.

## Self-Check: PASSED

- Key files created and modified as planned.
- Requirements `CLOUD-04`, `CLOUD-08`, and `CLOUD-09` are represented in code, tests, and docs.
- Success criteria met: no app secret introduced, token fixture is ready for native verification, and stable API/token schemas exist.

---
*Phase: 05-one-machine-activation-service*
*Completed: 2026-06-23*
