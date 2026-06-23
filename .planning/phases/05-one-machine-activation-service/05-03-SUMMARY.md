---
phase: 05-one-machine-activation-service
plan: 03
subsystem: api
tags: [cloudflare-workers, rate-limiting, audit, replay-protection, smoke-tests]

requires:
  - phase: 05-one-machine-activation-service
    provides: activation lifecycle endpoints, D1 one-machine policy, and entitlement signing
provides:
  - Layered activation rate-limit abstraction and Wrangler binding configuration
  - Activation request replay tracking through D1
  - Redacted structured activation audit helper
  - Abuse-control tests for rate limits, replay, malformed requests, and audit redaction
  - Local and deployed Phase 5 smoke documentation
affects: [05-one-machine-activation-service, 06-native-offline-licensing, cloud-infrastructure]

tech-stack:
  added: []
  patterns:
    - Route plus IP, route plus license hash, and route plus machine hash rate-limit keys
    - Redacted activation audit details serialized through a helper instead of ad hoc logging
    - Account-specific deployed smoke is documented separately from local automated proof

key-files:
  created:
    - tests/worker/abuse.test.ts
    - worker/src/api/audit.ts
    - worker/src/api/rateLimit.ts
    - worker/src/db/migrations/0003_activation_request_idempotency.sql
  modified:
    - docs/activation-api.md
    - docs/cloud-infrastructure.md
    - infra/deploy.ps1
    - infra/manifest.yaml
    - infra/verify.ps1
    - scripts/verify-project.ps1
    - tests/worker/setup.ts
    - worker/src/api/activationRoutes.ts
    - worker/src/db/repository.ts
    - worker/src/db/schema.ts
    - worker/src/env.ts
    - worker/src/routes.ts
    - worker/wrangler.toml

key-decisions:
  - "Activation rate limiting uses Cloudflare's RateLimit binding when configured, with local tests using deterministic mock behavior."
  - "Activation replay protection records request IDs before policy mutation and returns replay_detected for repeats."
  - "Activation audit entries store hashes and coarse client metadata only; raw license keys, machine IDs, request bodies, PII, and secrets are excluded."
  - "Deployed smoke is documented as credential-dependent and is not claimed as passed without real Cloudflare/Lemon configuration."

patterns-established:
  - "Activation hardening modules live under worker/src/api and are called from route handlers before expensive activation work."
  - "Rate-limit and replay outcomes share the stable generic activation error envelope."
  - "Project verification checks Phase 5 activation hardening artifacts."

requirements-completed:
  - CLOUD-08
  - CLOUD-09
  - CLOUD-10

duration: 14 min
completed: 2026-06-23
---

# Phase 05 Plan 03: Activation Hardening Summary

**Layered rate limits, replay protection, redacted audit, and smoke documentation for the activation API**

## Performance

- **Duration:** 14 min
- **Started:** 2026-06-23T23:36:24Z
- **Completed:** 2026-06-23T23:50:01Z
- **Tasks:** 3
- **Files modified:** 17

## Accomplishments

- Added `ACTIVATION_RATE_LIMIT` support using Cloudflare's RateLimit binding and layered keys.
- Added D1-backed activation request replay tracking and stable `replay_detected` responses.
- Added a redacted activation audit helper that hashes license keys and machine IDs and records structured outcomes.
- Added abuse tests for rate limiting, replay detection, malformed request rejection, and audit redaction.
- Updated activation/cloud docs with honest local and deployed smoke coverage.
- Updated project verifier checks for Phase 5 hardening artifacts.

## Task Commits

1. **Task 1-3: Rate limits, replay, audit, smoke docs, and verifier hooks** - `77db002` (feat)

**Plan metadata:** pending in this summary commit

## Files Created/Modified

- `worker/src/api/rateLimit.ts` - Layered key builder and optional RateLimit binding wrapper.
- `worker/src/api/audit.ts` - Redacted structured activation audit helper.
- `worker/src/api/activationRoutes.ts` - Calls rate-limit, replay, and audit controls from activation endpoints.
- `worker/src/db/migrations/0003_activation_request_idempotency.sql` - Request ID replay tracking table.
- `worker/src/db/repository.ts` / `worker/src/db/schema.ts` - Replay repository and schema types.
- `worker/wrangler.toml` / `infra/manifest.yaml` - `ACTIVATION_RATE_LIMIT` binding/config contract.
- `tests/worker/abuse.test.ts` - Abuse-control coverage.
- `docs/activation-api.md` / `docs/cloud-infrastructure.md` - Local/deployed smoke and hardening docs.
- `scripts/verify-project.ps1` / `infra/verify.ps1` / `infra/deploy.ps1` - Phase 5 verification and deployment visibility updates.

## Decisions Made

- Used route plus IP, route plus license-key hash, and route plus machine-ID hash keys to match the project decision while acknowledging Cloudflare's advice that IP-only keys are a poor sole identifier.
- Kept deployed smoke instructions explicit about account-specific prerequisites, so local tests remain the honest default proof when real credentials are unavailable.
- Recorded human doc review as approved by the user on 2026-06-23.

## Deviations from Plan

None - plan executed exactly as written.

**Total deviations:** 0 auto-fixed.
**Impact on plan:** No scope change.

## Issues Encountered

- The Cloudflare docs page recommends the current `[[ratelimits]]` Wrangler shape and notes RateLimit counters are local/eventually consistent. The implementation treats rate limiting as abuse friction and keeps D1 as the policy source of truth.

## Verification

- `npm.cmd --prefix worker run test -- abuse.test.ts activation-routes.test.ts repository.test.ts` - passed, 30 tests.
- `npm.cmd --prefix worker run typecheck` - passed.
- `npm.cmd --prefix worker run test` - passed, 61 tests.
- `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/verify-project.ps1 -SelfTest` - passed, including five negative probes.
- Human checkpoint: docs reviewed and approved by user on 2026-06-23.

## User Setup Required

Cloudflare deployed smoke requires account-specific setup: real Cloudflare environment, D1 migrations, `SIGNING_PRIVATE_KEY`, `SIGNING_KEY_ID`, `SIGNING_PUBLIC_KEYS`, Lemon Squeezy webhook data, and an active test license. Local automated tests require none of that.

## Next Phase Readiness

Phase 6 can consume the signed entitlement fixture, activation lifecycle endpoints, and documented smoke contract. Native offline licensing should verify the canonical JSON fixture before trusting local entitlement storage.

## Self-Check: PASSED

- Key files created and modified as planned.
- Requirements `CLOUD-08`, `CLOUD-09`, and `CLOUD-10` are covered by code, tests, docs, and verifier checks.
- Success criteria met: rate limits, audit redaction, request validation, signing rotation metadata, failure tests, and smoke docs are ready for Phase 6.

---
*Phase: 05-one-machine-activation-service*
*Completed: 2026-06-23*
