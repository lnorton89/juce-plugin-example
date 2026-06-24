# Cloud Infrastructure: LumaScope Activation Service

This document describes the cloud foundation for Lemon Squeezy purchase activation. A Cloudflare Worker ingests Lemon Squeezy webhook events, verifies their authenticity with HMAC-SHA256, processes entitlement events idempotently, and stores license data in D1.

This infrastructure supports Phase 5 (One-Machine Activation Service) and Phase 6 (Native Offline Licensing).

## Architecture

```
Lemon Squeezy --> Webhook (signed HMAC-SHA256) --> Cloudflare Worker
                                                     |
                                                 D1 Database
                                                 (licenses, activations,
                                                  idempotency, audit)
```

The Worker exposes these HTTP endpoints:
- `POST /api/webhook/lemon-squeezy` — Receives and verifies Lemon Squeezy webhook events
- `GET /api/health` — Health check returning `{ status: "ok", version: 1 }`
- `POST /api/v1/activate` — Activates or refreshes the one active machine for a license
- `POST /api/v1/validate` — Refreshes an existing active activation only
- `POST /api/v1/deactivate` — Deactivates the matching active machine for transfer

## Prerequisites

- A [Cloudflare account](https://dash.cloudflare.com/sign-up) (free tier supports Workers + D1)
- Node.js 22+ and npm
- Git checkout of this repository
- A [Lemon Squeezy](https://lemonsqueezy.com) store with a product configured

## Environment Variables

The bootstrap and deploy scripts read these environment variables. Never commit them to the repository.

| Variable | Source | Required |
|----------|--------|----------|
| `CLOUDFLARE_API_TOKEN` | Cloudflare Dashboard > My Profile > API Tokens > Create Token (Edit Cloudflare Workers template) | Yes |
| `CLOUDFLARE_ACCOUNT_ID` | Cloudflare Dashboard > Workers & Pages > Account ID (32-hex-char identifier) | Yes |
| `LEMON_WEBHOOK_SECRET` | Lemon Squeezy Store Settings > Webhooks > signing secret | Yes (for production) |
| `SIGNING_PRIVATE_KEY` | Operator-generated Ed25519 PKCS#8 private key, unpadded base64url | Yes for activation endpoints |
| `SIGNING_KEY_ID` | Operator-selected active key ID matching the public key ring | Yes for activation endpoints |
| `SIGNING_PUBLIC_KEYS` | JSON public key ring for native/offline verification | Yes for activation endpoints |

Create a Cloudflare API token with `Workers:Edit` and `D1:Edit` permissions.

## Infrastructure Manifest

`infra/manifest.yaml` is a declarative YAML file describing all logical resources. It contains:

- **Three environments**: `local`, `preview`, `production`
- **Per-environment**: Worker name, route pattern, D1 database binding and name, required secrets
- **Lemon Squeezy configuration**: Supported event types, webhook secret reference
- **Signing configuration**: Active key ID, public key ring, and private key secret names
- **Rate limiting configuration**: `ACTIVATION_RATE_LIMIT` binding, namespace ID, limit, and period

The manifest contains **zero account IDs, resource UUIDs, or secret values**. It is safe to commit.

## Generated State

`infra/generated-state.json` holds account-specific resource identifiers created during bootstrap. This file is **gitignored** — never committed. Its schema is documented in `infra/generated-state.schema.json`.

After bootstrap, it contains:
- `account_id` — Your Cloudflare account ID
- `account_email` — Account email
- `environments.*.worker_id` / `d1_database_id` — Created resource UUIDs

## Step 1: Bootstrap Resources

Create the required Cloudflare D1 databases for your target environment:

```powershell
$env:CLOUDFLARE_API_TOKEN = "YOUR_CLOUDFLARE_TOKEN"
$env:CLOUDFLARE_ACCOUNT_ID = "YOUR_ACCOUNT_ID"
powershell.exe -NoProfile -File infra/bootstrap.ps1 -Environment production
```

What this does:
1. Authenticates to Cloudflare via Wrangler
2. Creates the D1 database specified in the manifest
3. Writes the returned database ID and Worker name to `infra/generated-state.json`

Use `-Force` to re-run and overwrite existing generated state. Use `-Environment local` for local development (no Cloudflare credentials needed).

## Step 2: Set Required Secrets

Required secrets are read from environment variables — never from files:

```powershell
$env:LEMON_WEBHOOK_SECRET = "your-lemon-webhook-secret"
$env:LEMON_STORE_ID = "your-store-id"
$env:LEMON_PRODUCT_ID = "your-product-id"
$env:LEMON_VARIANT_ID = "your-variant-id"
powershell.exe -NoProfile -File infra/deploy.ps1 -Environment production
```

Required secrets per environment are documented in `infra/manifest.yaml`. For the `local` environment, secrets are injected via `.dev.vars` or inline Wrangler configuration.

## Step 3: Deploy and Migrate

`infra/deploy.ps1` performs three actions in sequence:

1. Applies D1 migrations (creates tables in the target database)
2. Deploys the Worker to Cloudflare
3. Injects secrets from environment variables via `wrangler secret put`

```powershell
powershell.exe -NoProfile -File infra/deploy.ps1 -Environment production
```

Expected output includes the deployment URL where the Worker is accessible.

The Worker declares an `ACTIVATION_RATE_LIMIT` binding in `worker/wrangler.toml` using Wrangler's `[[ratelimits]]` syntax. The namespace ID is a portable placeholder and can be changed per account if a distinct counter namespace is needed.

## Step 4: Verify Deployment

```powershell
powershell.exe -NoProfile -File infra/verify.ps1 -Environment production -Verbose
```

The verification script performs these checks:

| Check | Description |
|-------|-------------|
| Manifest exists | `infra/manifest.yaml` is present |
| Generated state valid | `infra/generated-state.json` has all required fields |
| Wrangler config | `worker/wrangler.toml` exists |
| Worker dependencies | `worker/package.json` present |
| Cloudflare authentication | `npx wrangler whoami` succeeds |
| D1 database exists | Database listed in `npx wrangler d1 list` |
| Migration status | Migrations are applied or pending |
| Rate limit config | `ACTIVATION_RATE_LIMIT` is present in `worker/wrangler.toml` |

All checks must return `[PASS]` for a successful verification.

## Step 5: Register Lemon Squeezy Webhook

After deployment, register the webhook in Lemon Squeezy:

1. Get the Worker URL from the deploy output or Cloudflare Dashboard > Workers & Pages > lumascope-activation
2. Go to Lemon Squeezy > Your Store > Settings > Webhooks
3. Click "Add webhook"
4. Enter the callback URL:
   ```
   https://lumascope-activation.YOUR-WORKER-SUBDOMAIN.workers.dev/api/webhook/lemon-squeezy
   ```
   Replace `YOUR-WORKER-SUBDOMAIN` with your Cloudflare Worker subdomain (visible in Cloudflare Dashboard > Workers & Pages).
5. Select these events:
   - `order_created`
   - `subscription_created`, `subscription_updated`, `subscription_cancelled`, `subscription_expired`
   - `license_key_created`, `license_key_updated`
6. Copy the webhook signing secret and set it as `LEMON_WEBHOOK_SECRET` in your environment
7. Re-run `infra/deploy.ps1` to inject the secret if it was not set before

The Worker verifies every webhook using constant-time HMAC-SHA256 over the exact raw request bytes. Events with invalid signatures return 401 and are logged to the audit table.

## Teardown

To remove all deployed resources:

```powershell
powershell.exe -NoProfile -File infra/teardown.ps1 -Environment production -Force -DeleteD1
```

The script prints the exact `npx wrangler` commands it will run before executing them. It requires either `-Force` or interactive `yes` confirmation. For the `local` environment, teardown is a no-op (no remote resources).

## Development Workflow (Local)

Test the Worker locally without a Cloudflare account:

```powershell
# Install dependencies (one-time)
npm.cmd --prefix worker ci

# Start local Worker
npx wrangler dev --env local
```

The Worker runs at `http://localhost:8787`. Send a test webhook:

```powershell
# Health check
curl http://localhost:8787/api/health

# Get a test webhook signature (using Node)
$signature = node -e "const crypto = require('crypto'); const body = JSON.stringify({meta:{event_name:'order_created',custom_data:{store_id:'1',product_id:'1',variant_id:'1'}},data:{id:'test-001',type:'orders',attributes:{store_id:'1',first_order_item:{product_id:'1',variant_id:'1'},user_name:'Test',user_email:'test@example.com'}}}); const hmac = crypto.createHmac('sha256','test-secret').update(body).digest('hex'); console.log(body); console.log(hmac);"

# Replace body and signature from output above
Invoke-WebRequest -Uri http://localhost:8787/api/webhook/lemon-squeezy -Method POST -ContentType "application/json" -Headers @{"X-Signature"="$signature"} -Body $body
```

## Testing

```powershell
npm.cmd --prefix worker run test
```

Worker tests use an in-memory MockD1Database and require no external infrastructure. The test suite covers:

- Valid and invalid HMAC-SHA256 signature verification
- Lemon webhook event parsing (valid, malformed, missing fields)
- Full webhook processing pipeline (signature → parse → idempotency → store → audit)
- Duplicate event idempotency
- Unsupported event types and unconfigured products
- Expired event replay rejection
- D1 repository CRUD operations (licenses, idempotency, audit, activations)
- Activation lifecycle endpoints: activate, same-machine retry, validate, deactivate, second-machine rejection, inactive license rejection
- Abuse controls: malformed request rejection, replay detection, rate-limit behavior, and redacted audit records

For deployed activation smoke instructions, see `docs/activation-api.md`. Deployed activation smoke requires real Cloudflare credentials, signing secrets, D1 migrations, and an active test license. Local automated tests are the honest substitute when those account-specific prerequisites are unavailable.

## Security Notes

- **No secrets in Git**: Account IDs, credentials, and signing keys exist only in environment variables or gitignored generated state
- **Constant-time signature verification**: Webhook X-Signature is verified using `crypto.subtle.verify` (constant-time HMAC-SHA256)
- **Parameterized queries**: All D1 queries use `prepare().bind()` — no SQL injection possible
- **Replay protection**: Webhook events with `event_created_at` older than 5 minutes are rejected
- **Activation replay protection**: Client request IDs are recorded and repeated IDs return `replay_detected`
- **Rate limiting**: Activation endpoints use route/IP, route/license-hash, and route/machine-hash keys through `ACTIVATION_RATE_LIMIT` when configured
- **Redacted audit records**: Activation audit entries store hashes and coarse client details, never raw license keys, machine IDs, request bodies, or signing secrets
- **Idempotency**: Each `event_id` is processed at most once; duplicates return 200 without mutating state
- **Unconfigured products**: Events for unconfigured store/product/variant IDs are silently skipped
- **Generic errors**: Error responses do not leak internal state, database contents, or configuration values

## Phase 5 Activation Service

Phase 5 adds the one-machine activation service on top of this infrastructure:

- Activation endpoint: validate license key + machine identifier, issue Ed25519-signed entitlement token
- Validation endpoint: refresh and re-sign existing entitlements
- Deactivation endpoint: free the activation slot for another machine
- Concurrency-safe activation limits (max one active machine per license)
- Rate limiting, request validation, and structured audit logging
