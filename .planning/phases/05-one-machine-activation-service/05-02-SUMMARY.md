---
phase: 05-one-machine-activation-service
plan: 02
subsystem: api
tags: [cloudflare-workers, d1, activation-api, licensing, ed25519]

requires:
  - phase: 05-one-machine-activation-service
    provides: v1 activation contracts, request validation, and entitlement signing helpers
provides:
  - Versioned activate, validate, and deactivate Worker endpoints
  - D1-backed one-active-machine activation policy
  - Unique active activation index migration for concurrency safety
  - Endpoint and repository tests for activation lifecycle and race behavior
affects: [05-one-machine-activation-service, 06-native-offline-licensing, cloud-infrastructure]

tech-stack:
  added: []
  patterns:
    - D1 unique partial index backs one-machine activation policy
    - Route handlers delegate policy state to Repository and return only stable error envelopes
    - Activation, validation, and deactivation all share bounded request validation

key-files:
  created:
    - tests/worker/activation-routes.test.ts
    - worker/src/api/activationRoutes.ts
    - worker/src/db/migrations/0002_activation_policy.sql
  modified:
    - docs/activation-api.md
    - tests/worker/repository.test.ts
    - tests/worker/setup.ts
    - worker/src/db/repository.ts
    - worker/src/db/schema.ts
    - worker/src/routes.ts

key-decisions:
  - "D1 enforces one active activation per license with a unique partial index, and repository code treats race collisions as activation-limit failures."
  - "Validation refreshes existing active activations only and never recreates missing or deactivated activations."
  - "Deactivation requires the same license key and matching machine ID; a license-key-only request cannot deactivate a different active machine."

patterns-established:
  - "Activation lifecycle handlers live in worker/src/api/activationRoutes.ts and are dispatched from worker/src/routes.ts."
  - "Repository activation policy methods return explicit outcomes instead of leaking database errors to route handlers."
  - "MockD1Database simulates activation inserts, updates, and unique-index race winners for policy tests."

requirements-completed:
  - CLOUD-04
  - CLOUD-05
  - CLOUD-06
  - CLOUD-07

duration: 6 min
completed: 2026-06-23
---

# Phase 05 Plan 02: Activation Lifecycle Summary

**One-machine activation, validation, and deactivation endpoints backed by D1 policy constraints**

## Performance

- **Duration:** 6 min
- **Started:** 2026-06-23T23:30:50Z
- **Completed:** 2026-06-23T23:36:24Z
- **Tasks:** 3
- **Files modified:** 9

## Accomplishments

- Added `/api/v1/activate`, `/api/v1/validate`, and `/api/v1/deactivate` route dispatch and handlers.
- Implemented repository methods for same-machine idempotent activation, different-machine rejection, validation refresh, matching-machine deactivation, and reactivation after deactivation.
- Added `0002_activation_policy.sql` with a unique partial index that prevents multiple active activations for the same license in D1.
- Added endpoint and repository tests covering success, same-machine retry, second-machine rejection, inactive/missing licenses, validation miss, machine mismatch, deactivation, transfer, malformed requests, and simulated activation races.

## Task Commits

1. **Task 1-3: Repository policy, endpoints, lifecycle docs, and tests** - `972aa8b` (feat)

**Plan metadata:** pending in this summary commit

## Files Created/Modified

- `worker/src/api/activationRoutes.ts` - Activation lifecycle route handlers, entitlement issuance, license checks, and stable policy errors.
- `worker/src/routes.ts` - Dispatches v1 activation endpoints.
- `worker/src/db/repository.ts` - Activation policy operations for activate, validate, deactivate, and machine lookup.
- `worker/src/db/schema.ts` - Activation policy outcome/result types.
- `worker/src/db/migrations/0002_activation_policy.sql` - Unique active activation index.
- `tests/worker/activation-routes.test.ts` - End-to-end route tests through `handleRequest`.
- `tests/worker/repository.test.ts` - Repository policy and race simulation tests.
- `tests/worker/setup.ts` - Mock D1 support for activation writes, updates, and uniqueness races.
- `docs/activation-api.md` - Lifecycle documentation.

## Decisions Made

- Used a D1 unique partial index as the hard backstop for one-machine policy rather than relying only on application checks.
- Kept activation ID presentation as `act_{activation.id}` for Phase 5 simplicity; future migration can introduce opaque IDs if needed before public release.
- Left broader abuse, rate-limit, redacted audit, and database-failure coverage for 05-03, where those controls are planned.

## Deviations from Plan

None - plan executed exactly as written.

**Total deviations:** 0 auto-fixed.
**Impact on plan:** No scope change.

## Issues Encountered

- The first race test mostly covered the pre-insert guard path. Tightened the mock so a competing active activation appears during the rejected insert, proving the repository collision handling path.

## Verification

- `npm.cmd --prefix worker run test -- repository.test.ts` - passed, 19 tests at the time of repository isolation.
- `npm.cmd --prefix worker run test -- activation-routes.test.ts repository.test.ts` - passed, 26 tests.
- `npm.cmd --prefix worker run typecheck` - passed.
- `npm.cmd --prefix worker run test` - passed, 57 tests.
- Route table scan confirmed `/api/v1/activate`, `/api/v1/validate`, and `/api/v1/deactivate`.

## User Setup Required

None - no external service configuration required for this plan beyond signing values already documented in 05-01.

## Next Phase Readiness

Ready for 05-03. The activation lifecycle now works through versioned endpoints; remaining hardening is rate limiting, replay/idempotency enforcement for client request IDs, redacted audit helpers, smoke docs, and broader abuse/failure tests.

## Self-Check: PASSED

- Key files created and modified as planned.
- Requirements `CLOUD-04`, `CLOUD-05`, `CLOUD-06`, and `CLOUD-07` are satisfied through working endpoints and tests.
- `CLOUD-10` is partially covered by lifecycle/race tests and remains pending for 05-03's abuse and database-failure coverage.

---
*Phase: 05-one-machine-activation-service*
*Completed: 2026-06-23*
