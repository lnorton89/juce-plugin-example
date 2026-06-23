# Activation API

Phase 5 defines the server-owned contract for one-machine activation. The native products call the Cloudflare Worker over HTTPS; they do not contain an app secret. Protection is layered through a license key, a bounded derived machine identifier, request IDs and timestamps, replay checks, one-machine state, rate limits, generic errors, and server-signed entitlements.

## Endpoints

All v1 activation endpoints use `POST` with `Content-Type: application/json`.

| Endpoint | Purpose |
|----------|---------|
| `/api/v1/activate` | Create or refresh the one active machine activation for a license. |
| `/api/v1/validate` | Refresh an existing active activation only. |
| `/api/v1/deactivate` | Deactivate the matching active machine so another machine can activate. |

## Request Shape

```json
{
  "licenseKey": "LICENSE-AAAA-BBBB-CCCC",
  "machineId": "opaque-derived-machine-id",
  "requestId": "req_20260623_abcdef",
  "timestamp": "2026-06-23T12:00:00.000Z",
  "appVersion": "1.0.0"
}
```

`appVersion` is optional. `machineId` is an opaque bounded string owned by Phase 6; Phase 5 never derives it from raw hardware identifiers.

## Activation Success

```json
{
  "activationId": "act_01JZ9Z4X4Y6Q8A9B0C1D2E3F4G",
  "entitlementToken": {
    "payload": {
      "schemaVersion": 1,
      "licenseKeyHash": "sha256:...",
      "activationId": "act_01JZ9Z4X4Y6Q8A9B0C1D2E3F4G",
      "machineId": "opaque-derived-machine-id",
      "productId": "prod_1",
      "variantId": "var_1",
      "status": "active",
      "issuedAt": "2026-06-23T12:00:00.000Z",
      "refreshAfter": "2026-06-24T12:00:00.000Z",
      "expiresAt": "2026-06-30T12:00:00.000Z",
      "kid": "prod-ed25519-2026-06"
    },
    "signature": "base64url-ed25519-signature",
    "kid": "prod-ed25519-2026-06",
    "algorithm": "Ed25519",
    "canonical": "deterministic-canonical-json"
  },
  "tokenType": "lumascope-entitlement-v1",
  "expiresAt": "2026-06-30T12:00:00.000Z",
  "serverTime": "2026-06-23T12:00:00.000Z",
  "refreshAfter": "2026-06-24T12:00:00.000Z"
}
```

The signed payload contains only minimal licensing claims. It must not contain customer name, customer email, raw license key, full request bodies, IP details, webhook lineage, or audit metadata.

## Lifecycle

1. A valid active license calls `/api/v1/activate` with its derived `machineId` and receives a signed entitlement.
2. Repeating `/api/v1/activate` from the same active machine is idempotent and returns a fresh signed entitlement for the existing activation.
3. A different machine receives `activation_limit_reached` while one active machine exists.
4. The active machine calls `/api/v1/validate` to refresh its entitlement. Validation does not recreate missing or deactivated activations.
5. The active machine calls `/api/v1/deactivate`; after that, validation returns `activation_not_found` and another machine may activate.

## Error Model

Errors use stable machine-readable codes with generic messages.

| Code | HTTP |
|------|------|
| `invalid_request` | 400 |
| `license_not_found` | 404 |
| `license_inactive` | 403 |
| `activation_limit_reached` | 409 |
| `activation_not_found` | 404 |
| `machine_mismatch` | 403 |
| `rate_limited` | 429 |
| `replay_detected` | 409 |
| `server_error` | 500 |

Example:

```json
{
  "error": {
    "code": "activation_limit_reached",
    "message": "The activation limit has been reached."
  }
}
```

## Signing Configuration

`SIGNING_PRIVATE_KEY` is a Worker secret containing the active Ed25519 PKCS#8 private key encoded as unpadded base64url. `SIGNING_KEY_ID` identifies that active key. `SIGNING_PUBLIC_KEYS` is non-secret JSON for docs, fixtures, and native verification:

```json
[
  {
    "kid": "prod-ed25519-2026-06",
    "publicKey": "base64url-spki-public-key",
    "algorithm": "Ed25519"
  }
]
```

`tests/worker/fixtures/entitlement-v1.json` locks the v1 canonical payload, canonical string, key material format, signature, and public key ring for cross-language Phase 6 verification.

## Abuse Controls

Activation endpoints apply controls before activation state changes:

- Request validation rejects wrong method, missing JSON content type, oversized bodies, malformed JSON, overlong fields, stale timestamps, and malformed request IDs.
- Replay protection records accepted request IDs and returns `replay_detected` for repeated IDs.
- Rate limiting uses the `ACTIVATION_RATE_LIMIT` Cloudflare binding when configured. Keys are layered by route plus IP, route plus license-key hash, and route plus machine-ID hash.
- Audit records store structured redacted details only: action, outcome, route, request ID, activation ID when known, app version, coarse client family, license-key hash, and machine-ID hash.

Audit records must never include raw license keys, raw machine IDs, signing secrets, full request bodies, or customer PII.

## Local Smoke

Automated local coverage is the smoke substitute that requires no Cloudflare account or Lemon Squeezy fixture account:

```powershell
npm.cmd --prefix worker run typecheck
npm.cmd --prefix worker run test -- activation-routes.test.ts abuse.test.ts
```

Those tests cover activate, same-machine retry, validate, deactivate, second-machine rejection, inactive license rejection, malformed request rejection, replay detection, rate-limit behavior, and audit redaction.

## Deployed Smoke

Deployed smoke requires a real Cloudflare environment, D1 database, signing secrets, and at least one active test license stored through the Lemon Squeezy webhook path. Do not claim deployed activation smoke passed unless those account-specific prerequisites are available.

After deploy and migrations, use a non-production test license:

```powershell
$base = "https://api.example.com"
$body = @{
  licenseKey = "TEST-LICENSE-FROM-LEMON"
  machineId = "test-machine-derived-id"
  requestId = "req_$(New-Guid)"
  timestamp = (Get-Date).ToUniversalTime().ToString("o")
  appVersion = "smoke"
} | ConvertTo-Json -Compress

Invoke-RestMethod -Uri "$base/api/v1/activate" -Method POST -ContentType "application/json" -Body $body
Invoke-RestMethod -Uri "$base/api/v1/validate" -Method POST -ContentType "application/json" -Body $body
Invoke-RestMethod -Uri "$base/api/v1/deactivate" -Method POST -ContentType "application/json" -Body $body
```

Repeat activation with the same machine and a fresh `requestId` to confirm idempotent success. Use a different `machineId` before deactivation to confirm `activation_limit_reached`. Reuse the same `requestId` to confirm `replay_detected`. Send an empty `machineId` to confirm `invalid_request`. Rate-limit behavior is feasible only when the deployed `ACTIVATION_RATE_LIMIT` binding is configured and the smoke can safely exceed the configured threshold.
