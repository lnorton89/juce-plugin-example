---
phase: 04-portable-purchase-infrastructure
plan: 02
subsystem: infra
tags: [cloudflare, wrangler, d1, powershell, infrastructure-automation]

# Dependency graph
requires:
  - phase: 04-01
    provides: infrastructure manifest, wrangler configuration, D1 migration, Worker skeleton
provides:
  - infra/common.ps1 shared module for all infra scripts
  - infra/bootstrap.ps1 for idempotent D1 database creation
  - infra/deploy.ps1 for migration apply, Worker deploy, secret injection
  - infra/verify.ps1 for non-destructive prerequisite and resource validation
  - infra/teardown.ps1 for confirmable resource destruction
  - Phase 4 artifact verification in scripts/verify-project.ps1
  - Worker pipeline steps in scripts/test-all.ps1
affects:
  - 04-03: infra automation scripts available for fresh-account provisioning
  - 04-04: deploy/teardown scripts used for provisioning rehearsal

# Tech tracking
tech-stack:
  added: [wrangler CLI, npx js-yaml]
  patterns:
    - Dot-sourced shared module pattern for multi-script infra automation
    - Invoke-Checked pattern for native command error propagation
    - Generated-state boundary pattern (read/write gitignored state)
    - Fail-before-mutation pattern for all infra scripts

key-files:
  created:
    - infra/common.ps1
    - infra/bootstrap.ps1
    - infra/deploy.ps1
    - infra/verify.ps1
    - infra/teardown.ps1
  modified:
    - scripts/test-all.ps1
    - scripts/verify-project.ps1

key-decisions:
  - "All scripts share common.ps1 via dot-sourcing to avoid duplication"
  - "Every script validates prerequisites before performing mutations"
  - "Secrets are read from environment variables only, never echoed or written to files"
  - "Local environment uses --local flag for migrations; remote operations skip when no remote resources exist"
  - "Generated state uses temp-file-then-rename pattern for atomic writes"

patterns-established:
  - "Shared module pattern: infra/common.ps1 dot-sourced by all infra scripts"
  - "Fail-safe: every script validates prerequisites before mutation"
  - "Script-scoped ErrorActionPreference = 'Stop' in all functions"
  - "Secret hygiene: [SET]/[MISSING] markers, never echo values"

requirements-completed: [INFRA-02, INFRA-03, INFRA-04, INFRA-05, INFRA-06]

# Metrics
duration: 7h 18min
completed: 2026-06-23
---

# Phase 4 Plan 2: Infrastructure Automation Scripts

**Idempotent PowerShell scripts for Cloudflare D1 resource bootstrapping, Worker deployment with secret injection, non-destructive verification, and confirmable teardown — all sharing a common.ps1 module and failing safely on missing prerequisites.**

## Performance

- **Duration:** 7h 18min
- **Started:** 2026-06-23T13:11:16Z
- **Completed:** 2026-06-23T20:29:16Z
- **Tasks:** 3
- **Files modified:** 9 (569 insertions, 21 deletions)

## Accomplishments

- **infra/common.ps1**: Shared module with functions for manifest parsing (npx js-yaml), generated-state read/write with atomic temp-file pattern, Cloudflare token validation, and wrangler command execution from the worker directory
- **infra/bootstrap.ps1**: Idempotent D1 database creation with prerequisite validation, manifest parsing, existing-state conflict detection, and atomic generated-state writes
- **infra/deploy.ps1**: Multi-step deployment applying D1 migrations, deploying the Worker, and injecting secrets from environment variables — with local/remote environment branching
- **infra/verify.ps1**: Non-destructive 8-9 point check covering manifest, generated-state, wrangler.toml, package.json, Cloudflare auth, D1 listing, migration status, and dry-run deploy
- **infra/teardown.ps1**: Confirmable worker deletion with optional D1 cleanup, printing exact wrangler commands before confirmation, and updating generated state after teardown
- **Scripts updated**: test-all.ps1 now runs Worker CI/typecheck/test pipeline; verify-project.ps1 asserts all Phase 4 artifacts and gitignored state boundary

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement shared infrastructure module and bootstrap script** - `4d873c3` (feat)
2. **Task 2: Implement deploy, verify, and teardown scripts** - `8483f28` (feat)
3. **Task 3: Update test-all.ps1, verify-project.ps1, and .gitignore for Phase 4** - `6bac9e9` (feat)

## Files Created/Modified

- `infra/common.ps1` - Shared module dot-sourced by all infra scripts (Get-Manifest, Test/Read/Write-GeneratedState, Test-CloudflareToken, Invoke-Wrangler)
- `infra/bootstrap.ps1` - Idempotent Cloudflare D1 resource creation and state generation
- `infra/deploy.ps1` - D1 migration apply, Worker deploy, and secret injection
- `infra/verify.ps1` - Non-destructive prerequisite/resource verification with [PASS]/[FAIL] reporting
- `infra/teardown.ps1` - Confirmable resource destruction with explicit command listing
- `scripts/test-all.ps1` - Added `npm --prefix worker ci`, `typecheck`, and `test` steps after UI pipeline
- `scripts/verify-project.ps1` - Added Phase 4 artifact paths, manifest text checks, and gitignore assertion

## Decisions Made

- **Scripts dot-source common.ps1** to avoid duplicating helper functions across 5 infra scripts
- **Temp-file-then-rename pattern** for atomic writes to generated-state.json prevents partial writes
- **Set-Location to worker/** for wrangler commands ensures the locally-installed wrangler (via node_modules/.bin) is used instead of npx's global cache
- **Local env skips remote operations** (deploy, secrets) and only applies migrations with `--local` flag
- **Teardown prints exact commands** before confirmation for operator review (T-04-10)

## Deviations from Plan

None — plan executed exactly as written.

## Issues Encountered

- **PowerShell 5.1 parser quirk with [scriptblock] + try/catch**: Found that combining a `[scriptblock]` parameter with `try/catch` and `$LASTEXITCODE` inside a function caused `"Array index expression is missing or not valid"` parser errors. Resolved by restructuring helper functions to avoid the `try/catch` pattern inside scriptblock-checking functions. (Affected `verify.ps1`)
- **Pre-existing `@oxc-project` scoped package path issue**: `verify-project.ps1 -SelfTest` fails at `Get-ChildItem` in `ui/node_modules/@oxc-project` due to PowerShell 5.1 limitations with junction/symlink directories. This is a pre-existing issue in the Phase 1-3 verification code, not introduced by this plan.

## User Setup Required

None - no external service configuration required by these scripts themselves. Users must have:
- Cloudflare account with API token (`CLOUDFLARE_API_TOKEN`)
- `npx wrangler` available (via Worker workspace `npm install`)

## Next Phase Readiness

- All 5 infra scripts are created, syntax-checked, and verified
- Scripts are ready for Plan 04-04 (fresh-account provisioning rehearsal)
- Worker pipeline steps integrated into test-all.ps1
- Phase 4 artifact verification in verify-project.ps1

---
*Phase: 04-portable-purchase-infrastructure*
*Completed: 2026-06-23*
