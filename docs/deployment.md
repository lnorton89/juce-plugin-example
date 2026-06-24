# Deployment and Operations Guide: LumaScope Activation Service

This guide provides comprehensive, step-by-step instructions for deploying, promoting, verifying, and maintaining the **LumaScope Activation Service** across different environments (staging/testing and production). 

The infrastructure is built on Cloudflare Workers and D1, with purchase authorization managed through Lemon Squeezy.

---

## 1. Environments Overview

The service defines three distinct environments in `infra/manifest.yaml` mapped to logical resources:

| Environment | Purpose | Target Domain | Worker Name | D1 Database Name |
|-------------|---------|---------------|-------------|------------------|
| **`local`** | Local development, testing, offline prototyping | `localhost:8787` | `lumascope-activation-local` | `lumascope-activation-db-local` |
| **`preview`** | Staging, integration testing, QA, and sandbox webhooks | `preview-api.lumascope.example.com` | `lumascope-activation-preview` | `lumascope-activation-db-preview` |
| **`production`** | Live end-user traffic, production webhooks, and activation | `api.lumascope.example.com` | `lumascope-activation` | `lumascope-activation-db` |

---

## 2. Deployment Prerequisites

Before deploying to `preview` (staging) or `production`, ensure you have the following:

1. **Cloudflare Account**: Access to a Cloudflare account with a Workers subdomain configured.
2. **Cloudflare Credentials**:
   - `CLOUDFLARE_ACCOUNT_ID`: Visible on your Cloudflare Workers & Pages dashboard.
   - `CLOUDFLARE_API_TOKEN`: Created via Cloudflare Dashboard > My Profile > API Tokens > Create Token (use the **Edit Cloudflare Workers** template).
3. **Wrangler**: Node.js and `npm` installed locally. Wrangler is run via `npx` (already pinned in `worker/package.json`).
4. **Lemon Squeezy Sandbox and Live Stores**:
   - A Lemon Squeezy account in **developer/test mode** (for `preview` staging testing).
   - A Lemon Squeezy account in **production mode** (for `production` live purchases).
5. **Private/Public Key Pairs**: An Ed25519 signing key pair for entitlement generation.

---

## 3. Step-by-Step Deployment Instructions

To deploy to either **`preview`** or **`production`**, follow these steps. Replace `-Environment preview` with `-Environment production` when going to production.

### Step 3.1: Bootstrap Cloudflare Resources
First, provision the logical databases and Worker state defined in the manifest.

```powershell
# Set credentials for authentication
$env:CLOUDFLARE_API_TOKEN = "YOUR_CLOUDFLARE_API_TOKEN"
$env:CLOUDFLARE_ACCOUNT_ID = "YOUR_CLOUDFLARE_ACCOUNT_ID"

# Run bootstrap script (creates D1 database if missing and populates infra/generated-state.json)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File infra/bootstrap.ps1 -Environment preview
```

### Step 3.2: Generate and Set Secrets
LumaScope utilizes environment variables to securely inject secrets during deployment. **Never check in secret values.**

#### A. Generate Ed25519 Keys
You need an Ed25519 private key in unpadded Base64URL format and a JSON-formatted public key ring. 
You can generate a new pair using a Node.js utility or similar:
```powershell
node -e "const crypto = require('crypto'); const { privateKey, publicKey } = crypto.generateKeyPairSync('ed25519'); console.log('Private:', privateKey.export({type:'pkcs8',format:'der'}).toString('base64url')); console.log('Public:', publicKey.export({type:'spki',format:'der'}).toString('base64url'));"
```

#### B. Setup Environment Secrets
Define the secrets in your deployment terminal environment. Always keep staging (`preview`) secrets separate from production keys.

```powershell
# Webhook signing secret from Lemon Squeezy Store > Settings > Webhooks
$env:LEMON_WEBHOOK_SECRET = "your-env-specific-webhook-secret"

# Product IDs from Lemon Squeezy dashboard
$env:LEMON_STORE_ID = "your-store-id"
$env:LEMON_PRODUCT_ID = "your-product-id"
$env:LEMON_VARIANT_ID = "your-variant-id"

# Key ID matching the public key ring
$env:SIGNING_KEY_ID = "key-2026-v1"

# Entitlement private key (unpadded base64url)
$env:SIGNING_PRIVATE_KEY = "YOUR_GENERATED_PRIVATE_KEY"

# JSON string representing the public key ring
$env:SIGNING_PUBLIC_KEYS = '{"key-2026-v1":"M_YOUR_GENERATED_PUBLIC_KEY"}'
```

### Step 3.3: Deploy and Migrate
Now, run the deployment script. It will automatically apply any pending SQL schema migrations to the D1 database, compile the Worker with Vite/Wrangler, upload the assets, and securely inject the secrets using `wrangler secret put`.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File infra/deploy.ps1 -Environment preview
```

*Note: The script output will display the final deployed Worker URL.*

### Step 3.4: Verify Deployment
Verify that everything is set up correctly and the Worker is fully functional:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File infra/verify.ps1 -Environment preview -Verbose
```

---

## 4. Webhook Registration (Lemon Squeezy)

To connect the activation service to your purchase funnel, register the webhook endpoint in the Lemon Squeezy dashboard:

1. **Staging / Test Mode**: Log into your Lemon Squeezy dashboard and toggle **Design / Test Mode**.
2. **Production Mode**: Log in and ensure you are in **Live Mode**.
3. Go to **Settings > Webhooks** and click **Add Webhook**.
4. Set the **Callback URL**:
   - For preview/staging: `https://lumascope-activation-preview.YOUR-SUBDOMAIN.workers.dev/api/webhook/lemon-squeezy`
   - For production: `https://lumascope-activation.YOUR-SUBDOMAIN.workers.dev/api/webhook/lemon-squeezy`
5. Set the **Signing Secret**: Must match the value you set for `$env:LEMON_WEBHOOK_SECRET`.
6. Select the required **Webhook Events**:
   - `order_created`
   - `subscription_created`, `subscription_updated`, `subscription_cancelled`, `subscription_expired`
   - `license_key_created`, `license_key_updated`
7. Click **Save Webhook**.

---

## 5. Staging/Testing and QA Validation Flow

Before promoting any changes to production, follow this integration test procedure in the `preview` environment:

### Step 5.1: Test Webhook Ingestion
1. In your Lemon Squeezy dashboard (Test Mode), go to **Store > Products** and create a test order.
2. In the order completion, Lemon Squeezy generates a mock license key.
3. Check the Worker's real-time tail logs to verify that the webhook was processed successfully:
   ```powershell
   npx wrangler tail --env preview
   ```
4. Verify that the license was inserted into the database by running a remote query:
   ```powershell
   npx wrangler d1 execute lumascope-activation-db-preview --env preview --command "SELECT * FROM licenses ORDER BY created_at DESC LIMIT 1;"
   ```

### Step 5.2: Test Client Activation (Simulated)
Simulate a client activation request to the preview endpoint:

```powershell
$body = @{
    license_key = "MOCK_LICENSE_KEY_FROM_LEMON"
    machine_id = "win-dev-test-01"
} | ConvertTo-Json

$resp = Invoke-WebRequest -Uri "https://lumascope-activation-preview.YOUR-SUBDOMAIN.workers.dev/api/v1/activate" `
                          -Method POST `
                          -ContentType "application/json" `
                          -Body $body

$resp.Content
```

Verify that the response returns a valid signed entitlement token containing your `SIGNING_KEY_ID`.

### Step 5.3: Test App Integration
To test the built C++ application against the staging/testing environment:
1. Open `project-config.json` and update `urls.previewApi` with your staging Worker URL:
   ```json
   "urls": {
     "previewApi": "https://lumascope-activation-preview.YOUR-SUBDOMAIN.workers.dev"
   }
   ```
2. Build the application in Vite/Dev mode.
3. Attempt to activate using your test license key.

---

## 6. Promotion to Production

Once the staging environment (`preview`) passes all verification steps, promote the service to production.

### Step 6.1: Version and Config Audit
1. Audit any schema or config files changed.
2. Ensure the version string in `project-config.json` is bumped (e.g., `0.1.1` or `1.0.0`).
3. Regenerate all derived build targets to match the version:
   ```powershell
   node scripts/generate-all-config.mjs
   ```
4. Rebuild the application binary to ensure it matches the final configuration.

### Step 6.2: Production Deployment
Run the bootstrap and deployment process targeting `production`:

```powershell
# Set production environment secrets (distinct from staging)
$env:LEMON_WEBHOOK_SECRET = "PROD_LEM_WEBHOOK_SECRET"
$env:SIGNING_PRIVATE_KEY = "PROD_PRIVATE_KEY"
$env:SIGNING_KEY_ID = "prod-key-v1"
$env:SIGNING_PUBLIC_KEYS = '{"prod-key-v1":"PROD_PUBLIC_KEY"}'

# Bootstrap production D1 database
powershell.exe -NoProfile -ExecutionPolicy Bypass -File infra/bootstrap.ps1 -Environment production

# Deploy Worker and apply migrations to production D1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File infra/deploy.ps1 -Environment production

# Verify production deployment
powershell.exe -NoProfile -ExecutionPolicy Bypass -File infra/verify.ps1 -Environment production -Verbose
```

### Step 6.3: Swap Application to Production API
1. Set `"urls.productionApi"` in `project-config.json` to point to the production endpoint.
2. Run `node scripts/generate-all-config.mjs`.
3. Compile the production-ready installer/VST3/Standalone bundle.

---

## 7. Backups and Restore Procedures

The licensing database is stored in Cloudflare D1. Since D1 is serverless, backups should be maintained regularly.

### Step 7.1: Automated Backups
Cloudflare D1 automatically performs daily backups of your databases. You can view, download, or restore these directly from the **Cloudflare Dashboard > D1 > [Select Database] > Backups**.

### Step 7.2: Manual Backups (Disaster Recovery / Local Mirroring)
To take a manual snapshot of the database schema and data from the remote environment:

```powershell
# Export remote database to SQL script
npx wrangler d1 export lumascope-activation-db --env production --output ./production_backup_$(Get-Date -Format "yyyyMMdd_HHmmss").sql
```

### Step 7.3: Database Restore / Rollback
To restore a database state (warning: this will overwrite existing data):

```powershell
# Execute the SQL backup script on the remote environment
npx wrangler d1 execute lumascope-activation-db --env production --file ./production_backup_20260624.sql
```

---

## 8. Worker Rollback Procedures

If a newly deployed Worker introduces bugs or performance regressions in production, you can instantly roll back to a previously healthy deployment.

### Step 8.1: List Deployments
View the deployment history of your Worker to identify the target ID:

```powershell
npx wrangler deployments --env production
```

### Step 8.2: Rollback Deployment
Roll back instantly to a specific healthy version without rebuilding or pushing code:

```powershell
# Replace the ID with the desired healthy deployment ID
npx wrangler rollback <deployment-id> --env production
```

---

## 9. Signing Key Rotation Procedures

To maintain high security, Ed25519 signing keys should be rotated periodically (e.g., annually) or immediately if a compromise is suspected.

The service supports seamless, non-breaking key rotation using a **Public Key Ring** configuration. This prevents invalidating existing user activation tokens during transition.

### Step 9.1: Generate a New Key Pair
Create a new Ed25519 key pair (e.g., `prod-key-v2` / `key-2027-v1`).

### Step 9.2: Update the Key Ring
Append the new public key to the `SIGNING_PUBLIC_KEYS` JSON ring. Keep the old public key in the ring so that existing active offline activations remain valid.

```json
{
  "key-2026-v1": "OLD_PUBLIC_KEY",
  "key-2027-v1": "NEW_PUBLIC_KEY"
}
```

### Step 9.3: Swap the Active Private Key
Update your deployment environment variables to use the new Private Key and active Key ID:

```powershell
# Set active key ID to the new key
$env:SIGNING_KEY_ID = "key-2027-v1"

# Set the active private key used to sign new activations
$env:SIGNING_PRIVATE_KEY = "NEW_PRIVATE_KEY"

# Ensure the public key ring contains BOTH public keys
$env:SIGNING_PUBLIC_KEYS = '{"key-2026-v1":"OLD_PUBLIC_KEY","key-2027-v1":"NEW_PUBLIC_KEY"}'
```

### Step 9.4: Deploy Changes
Deploy to the target environment:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File infra/deploy.ps1 -Environment production
```

#### What happens after deployment:
1. **Existing Clients**: Clients with an activation token signed with `key-2026-v1` remain authorized. The application verifies their token using the old public key still present in the key ring.
2. **New/Revalidating Clients**: Any new activation or validation request signs a new token using the private key for `key-2027-v1`. The application verifies these tokens using the new public key.

### Step 9.5: Clean Up Old Keys (Optional)
Once the grace period has passed and all clients have revalidated (at least 7 days after rotation), you can safely remove the old key from the public key ring:

```powershell
$env:SIGNING_PUBLIC_KEYS = '{"key-2027-v1":"NEW_PUBLIC_KEY"}'
powershell.exe -NoProfile -ExecutionPolicy Bypass -File infra/deploy.ps1 -Environment production
```
