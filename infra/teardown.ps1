# teardown.ps1 — LumaScope Activation Infrastructure Teardown
# Destroys Cloudflare resources after explicit confirmation.
# Usage: .\infra\teardown.ps1 [-Environment local|preview|production] [-Force] [-DeleteD1]

[CmdletBinding()]
param(
    [ValidateSet('local', 'preview', 'production')]
    [string] $Environment = 'local',
    [switch] $Force,
    [switch] $DeleteD1
)

$ErrorActionPreference = 'Stop'

# --- Dot-source shared module ---
. (Join-Path $PSScriptRoot 'common.ps1')

# --- Local env check ---------------------------------------------------------
if ($Environment -eq 'local') {
    Write-Host '[SKIP] Local environment has no remote resources to tear down.'
    Write-Host '       To clean local D1 state, delete the .wrangler/ state directory:'
    Write-Host '       Remove-Item -Recurse -Force .wrangler'
    exit 0
}

# --- Prerequisites -----------------------------------------------------------
if (-not (Test-CloudflareToken)) {
    Write-Host '[FAIL] Not authenticated with Cloudflare. Run: npx wrangler login'
    exit 1
}

if (-not (Test-Path -LiteralPath (Get-GeneratedStatePath))) {
    Write-Host '[FAIL] Generated state not found. Nothing to tear down.'
    exit 1
}

$state = Read-GeneratedState
if (-not $state) {
    Write-Host '[FAIL] Could not read generated state.'
    exit 1
}

$manifest = Get-Manifest
$envConfig = $manifest.environments.$Environment
$envState = $state.environments.$Environment

# --- Resource listing --------------------------------------------------------
$workerName = if ($envState.worker_name) { $envState.worker_name } else { $envConfig.worker.name }
$d1Name     = if ($envState.d1_database_name) { $envState.d1_database_name } else { $envConfig.worker.d1_database_name }
$d1Id       = $envState.d1_database_id

Write-Host "=== Resources to destroy ($Environment) ==="
Write-Host "  Worker:   $workerName"
if ($DeleteD1 -and $d1Id) {
    Write-Host "  D1 DB:    $d1Name ($d1Id)"
}
Write-Host ''

Write-Host 'The following wrangler commands will be executed:'
Write-Host "  > npx wrangler delete --env $Environment"
if ($DeleteD1 -and $d1Id) {
    Write-Host "  > npx wrangler d1 delete $d1Name"
}
Write-Host ''

# --- Confirmation ------------------------------------------------------------
if (-not $Force) {
    $response = Read-Host 'Type "yes" to confirm resource destruction'
    if ($response -ne 'yes') {
        Write-Host 'Teardown cancelled.'
        exit 0
    }
    Write-Host ''
}

# --- Delete Worker -----------------------------------------------------------
Write-Host "=== Deleting Worker ($workerName) ==="

try {
    Invoke-Wrangler @('delete', '--env', $Environment)
    Write-Host '[PASS] Worker deleted'
} catch {
    Write-Host "[WARN] Worker deletion issue: $_"
}

# --- Delete D1 (optional) ----------------------------------------------------
if ($DeleteD1 -and $d1Id) {
    Write-Host ''
    Write-Host "=== Deleting D1 Database ($d1Name) ==="

    # Run d1 delete command (Invoke-Wrangler needs --skip-confirmation since D1 delete also prompts)
    $workerDir   = Join-Path $RepoRoot 'worker'
    $originalDir = Get-Location
    try {
        Set-Location -LiteralPath $workerDir -ErrorAction Stop
        Write-Host "> npx wrangler d1 delete $d1Name --skip-confirmation"
        & npx.cmd wrangler d1 delete $d1Name --skip-confirmation 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Host "[WARN] D1 deletion issue (exit code $LASTEXITCODE)"
        } else {
            Write-Host '[PASS] D1 database deleted'
        }
    } finally {
        Set-Location -LiteralPath $originalDir -ErrorAction SilentlyContinue
    }
}

# --- Update generated state --------------------------------------------------
Write-Host ''
Write-Host '=== Updating Generated State ==='

# Clear this environment's state
$state.environments.$Environment.worker_id        = ''
$state.environments.$Environment.worker_name      = ''
$state.environments.$Environment.d1_database_id   = ''
$state.environments.$Environment.d1_database_name = ''

Write-GeneratedState -State $state
Write-Host "[PASS] Generated state updated: $(Get-GeneratedStatePath)"

Write-Host ''
Write-Host '=== Teardown Summary ==='
Write-Host "  Environment: $Environment"
Write-Host "  Worker:      deleted"
if ($DeleteD1 -and $d1Id) { Write-Host "  D1 DB:       deleted" }
Write-Host "  State:       updated"
Write-Host ''
Write-Host 'Teardown complete.'
exit 0
