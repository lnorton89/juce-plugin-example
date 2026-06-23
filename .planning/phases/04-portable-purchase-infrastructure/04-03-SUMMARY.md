---
phase: 04-portable-purchase-infrastructure
plan: 03
subsystem: worker
tags:
  - webhook
  - d1
  - lemon-squeezy
  - hmac
  - repository
  - tests
requires:
  - 04-01 (Worker skeleton, D1 schema, migration)
  - 04-02 (Infrastructure bootstrap scripts)
provides:
  - Webhook verification and event processing
  - D1 repository layer for licenses, idempotency, audit
  - Comprehensive test suite with mock infrastructure
affects:
  - worker/src/routes.ts (wired webhook handler)
  - worker/src/index.ts (error response hardening)
tech-stack:
  added:
    - Web Crypto API (crypto.subtle.verify for HMAC-SHA256)
    - D1 prepared statements with ON CONFLICT UPSERT
    - Vitest 4 with standard runner (migrated from pool)
  patterns:
    - In-memory MockD1Database for Worker tests
    - Structured webhook event parsing with fallback chains
    - Parameterized SQL queries throughout repository layer
key-files:
  created:
    - worker/src/db/repository.ts
    - worker/src/webhook.ts
    - tests/worker/setup.ts
    - tests/worker/webhook.test.ts
    - tests/worker/repository.test.ts
    - tests/worker/fixtures/lemon-webhook-valid.json
    - tests/worker/fixtures/lemon-webhook-invalid-signature.json
    - tests/worker/fixtures/lemon-webhook-unsupported-event.json
  modified:
    - worker/src/db/schema.ts (added D1ResultRow type, RepositoryConfig)
    - worker/src/routes.ts (wired handleLemonWebhook)
    - worker/src/index.ts (removed error message for T-04-15)
    - worker/vitest.config.ts (migrated from pool to standard runner)
    - .gitignore (added tests/worker/node_modules)
decisions:
  - Use INSERT ... ON CONFLICT DO UPDATE SET instead of INSERT OR REPLACE to preserve activated_count and created_at on existing records.
  - Prefer meta.custom_data identifiers for store/product/variant validation over data.attributes numeric IDs.
  - Use MockD1Database with keyword-based SQL matching rather than the Cloudflare vitest pool (incompatible with Vitest 4).
metrics:
  duration: 0h 25m
  completed_date: 2026-06-23
---

# Phase 4 Plan 3: Webhook and Repository Implementation Summary

**One-liner:** Implemented HMAC-SHA256 Lemon Squeezy webhook verification, D1 repository CRUD operations, and a 33-test comprehensive test suite with in-memory mock infrastructure.

## What Was Built

### D1 Repository Layer (worker/src/db/repository.ts)
- **Repository class** accepting D1Database in constructor with seven methods
- **License operations**: `findByOrderId`, `findByLicenseKey`, `upsertFromWebhook` using `INSERT ... ON CONFLICT DO UPDATE SET` to preserve activated_count
- **Idempotency operations**: `findIdempotency`, `markIdempotency` with `ON CONFLICT DO NOTHING` for deduplication
- **Audit operations**: `writeAudit` with optional resource, IP, and request ID fields
- **Activation stubs**: `findActiveActivations` for Phase 5 use
- All queries use parameterized D1 prepared statements — no SQL injection possible

### Webhook Verification and Event Handler (worker/src/webhook.ts)
- **verifyLemonSignature**: Reads exact raw request bytes, computes HMAC-SHA256 via `crypto.subtle.verify` (constant-time), returns `{valid, body}`
- **parseLemonEvent**: Validates Lemon Squeezy webhook structure, extracts typed event with store/product/variant from `meta.custom_data`, supports order, subscription, and license key events
- **handleLemonWebhook**: Full processing pipeline:
  - Content-Type validation (400)
  - Body size enforcement (413 if >1MB)
  - Signature verification (401)
  - Event parsing (400)
  - Replay protection via 5-minute expiry check (400)
  - Idempotency deduplication (200 on repeat)
  - Product/store/variant allowlist validation (200 "skipped" on mismatch)
  - Supported event routing (order_created, subscription_*, license_key_*)
  - Audit logging and idempotency marking
  - Generic error responses — no internal details leaked
- Routes wired to POST /api/webhook/lemon-squeezy
- index.ts hardened to omit error detail from 500 responses (T-04-15)

### Test Infrastructure (tests/worker/)
- **MockD1Database**: In-memory Map-based D1 implementation handling SELECT, INSERT with ON CONFLICT patterns, and parameterized queries
- **generateSignature**: HMAC-SHA256 hex computation for test signature generation
- **createMockEnv**: Factory for mock Env objects per test scenario
- **33 tests** across two files:
  - **webhook.test.ts** (20 tests): signature verification (4), event parsing (5), integration tests (11) covering valid, duplicate, unsupported, unconfigured, expired, missing Content-Type, oversized, invalid signature, subscription, license key, and partial env config
  - **repository.test.ts** (13 tests): upsert create (1), upsert update (1), find by order (2), find by key (2), idempotency find null (1), mark & find (1), double mark (1), audit write (1), multiple audits (1), active activations (2)

## Deviations from Plan

### Rule 2 — Technical improvements
1. **INSERT ON CONFLICT instead of INSERT OR REPLACE** — The plan specified `INSERT OR REPLACE INTO licenses` which resets `activated_count` and `created_at` on every upsert. Changed to `INSERT ... ON CONFLICT(lemon_order_id) DO UPDATE SET` to preserve existing counters and creation timestamps. This is correct behavior for webhook updates that should not reset activation state.

2. **Parser prefers meta.custom_data for identifiers** — The plan's fixture specification shows `meta.custom_data` with string identifiers and `data.attributes` with numeric IDs. The parser was updated to prefer `meta.custom_data` values (merchant-configured) over resource attribute values (Lemon Squeezy internal) for store/product/variant validation.

3. **Vitest config migrated from pool to standard runner** — The existing `@cloudflare/vitest-pool-workers` pool is incompatible with Vitest 4 (pool architecture was removed). Changed to standard runner with configured root. Node.js v22.18 provides all needed globals (`crypto.subtle`, `Request`, `Response`).

## Threat Model Compliance

| Threat | Disposition | Implementation |
|--------|-------------|----------------|
| T-04-11 (Spoofing) | mitigate | HMAC-SHA256 via crypto.subtle.verify (constant-time) |
| T-04-12 (Tampering) | mitigate | Replay protection: reject events >5 min old |
| T-04-13 (Tampering) | mitigate | Event ID deduplication via webhook_idempotency table |
| T-04-14 (DoS) | mitigate | Content-Type check, 1MB body limit (413) |
| T-04-15 (Info Disclosure) | mitigate | Generic 400/401/413/500 responses, no details |
| T-04-16 (Tampering) | mitigate | Parameterized D1 prepared statements everywhere |
| T-04-17 (Spoofing) | mitigate | Product/store/variant allowlist validation |
| T-04-18 (Tampering) | mitigate | crypto.subtle.verify is constant-time |
| T-04-SC (Tampering) | mitigate | No new npm packages — all Web Crypto built-in |

## Verification Results

- [x] `npm.cmd --prefix worker run typecheck` — exits 0
- [x] `npm.cmd --prefix worker run test` — 33/33 tests passing
- [x] Coverage: valid/invalid signatures, malformed/expired/unsupported events, idempotency, unconfigured store/product, repository CRUD
- [x] webhook.ts uses `crypto.subtle.verify` for HMAC (constant-time)
- [x] repository.ts uses parameterized prepared statements (`.bind()`) everywhere
- [x] Error responses are generic (no internal details leaked)
- [x] routes.ts wires both webhook and health endpoints
- [x] index.ts error handler returns generic `{ error: "Internal Error" }` without details

## Self-Check: PASSED

All files verified:
- `worker/src/db/repository.ts` — exists ✓
- `worker/src/webhook.ts` — exists ✓
- `worker/src/db/schema.ts` — modified ✓
- `worker/src/routes.ts` — modified ✓
- `worker/src/index.ts` — modified ✓
- `tests/worker/webhook.test.ts` — exists ✓
- `tests/worker/repository.test.ts` — exists ✓
- `tests/worker/setup.ts` — exists ✓
- `tests/worker/fixtures/lemon-webhook-valid.json` — exists ✓
- `tests/worker/fixtures/lemon-webhook-invalid-signature.json` — exists ✓
- `tests/worker/fixtures/lemon-webhook-unsupported-event.json` — exists ✓

All commits verified:
- `c7b0a6b` — feat(04-03): implement D1 repository layer ✓
- `4fd2f96` — feat(04-03): implement Lemon webhook signature verification ✓
- `89287cf` — test(04-03): implement Worker tests with fixtures ✓
- `8e2dc4f` — chore(04-03): clean up unused test imports ✓
