# Configuration Reference

`project-config.json` is the source of truth for product identity, version, plugin identifiers, runtime file names, UI branding, Worker resource names, and environment variable contracts.

After editing it, regenerate derived source modules and validate the schema:

```powershell
node scripts/generate-all-config.mjs
```

Generated or generated-style consumers include:

| Consumer | File |
|----------|------|
| CMake configure | `cmake/Config.cmake` |
| Native C++ constants | `cmake/templates/ProjectConfig.h.in` -> build `generated/include/LumaScope/ProjectConfig.h` |
| UI TypeScript constants | `ui/src/config/projectConfig.ts` |
| Worker TypeScript constants | `worker/src/config/projectConfig.ts` |
| PowerShell scripts | `scripts/config.ps1` |

PowerShell build and smoke scripts dot-source `scripts/config.ps1` for target names, artifact directories, product names, AppData paths, and generated filenames. Avoid reintroducing hardcoded `LumaScope_*` target or artifact path assumptions in scripts.

Documentation may use template variables when describing configurable values:

| Variable | Resolves To |
|----------|-------------|
| `{{product_name}}` | `product.name` |
| `{{company_name}}` | `product.companyName` |
| `{{version}}` | `version.string` |
| `{{activation_worker_name}}` | `worker.names.production` |
| `{{entitlement_token_type}}` | `tokens.entitlementV1` |

Keep checked-in example values non-secret and portable. Account-specific IDs, D1 UUIDs, routes owned by a real account, private keys, and API tokens must stay in generated local state or environment secrets.
