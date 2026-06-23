# verify.ps1 — LumaScope Activation Infrastructure Verification
# Non-destructively validates prerequisites, state, and deployed resources.
# Usage: .\infra\verify.ps1 [-Environment local|preview|production] [-ShowDiagnostics]

[CmdletBinding()]
param(
    [ValidateSet('local', 'preview', 'production')]
    [string] $Environment = 'local',
    [switch] $ShowDiagnostics
)

$ErrorActionPreference = 'Stop'
$passed  = 0
$failed  = 0
$total   = 0

# --- Dot-source shared module ---
. (Join-Path $PSScriptRoot 'common.ps1')

function Add-Check {
    param($Name, [scriptblock] $Condition)
    $script:total++
    $result = & $Condition
    if ($result) {
        Write-Host "[PASS] $Name"
        $script:passed++
    } else {
        Write-Host "[FAIL] $Name"
        $script:failed++
    }
}

function Check-Wrangler {
    param($Name, [string[]] $Args)
    $script:total++
    $workerDir = Join-Path $RepoRoot 'worker'
    $originalDir = Get-Location
    Set-Location -LiteralPath $workerDir -ErrorAction Stop
    $null = & npx.cmd wrangler @Args 2>&1
    Set-Location -LiteralPath $originalDir -ErrorAction SilentlyContinue
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[PASS] $Name"
        $script:passed++
    } else {
        Write-Host "[FAIL] $Name"
        $script:failed++
    }
}

# --- Checks ------------------------------------------------------------------
Write-Host "=== Verifying $Environment infrastructure ==="
Write-Host ''

# 1. Manifest exists
Add-Check -Name 'Manifest exists' -Condition {
    Test-Path -LiteralPath (Get-ManifestPath)
}

# 2. Generated state exists
Add-Check -Name 'Generated state exists' -Condition {
    Test-Path -LiteralPath (Get-GeneratedStatePath)
}

# 3. Generated state is valid
Add-Check -Name 'Generated state is valid' -Condition {
    Test-GeneratedState
}

# 4. wrangler.toml exists
Add-Check -Name 'wrangler.toml exists' -Condition {
    Test-Path -LiteralPath (Join-Path $RepoRoot 'worker/wrangler.toml')
}

# 5. worker/package.json exists
Add-Check -Name 'worker/package.json exists' -Condition {
    Test-Path -LiteralPath (Join-Path $RepoRoot 'worker/package.json')
}

# 6. Cloudflare authentication
Add-Check -Name 'Cloudflare authentication' -Condition {
    Test-CloudflareToken
}

# 7. D1 database exists
Check-Wrangler -Name 'D1 database listed' -Args @('d1', 'list')

# 8. Migration status
if ($Environment -eq 'local') {
    Check-Wrangler -Name 'Migration status checkable' -Args @('d1', 'migrations', 'list', '--env', $Environment, '--local')
} else {
    Check-Wrangler -Name 'Migration status checkable' -Args @('d1', 'migrations', 'list', '--env', $Environment)
}

# 9. Worker deployment dry-run (only for remote envs)
if ($Environment -ne 'local') {
    Check-Wrangler -Name "Worker '$Environment' deployment dry-run" -Args @('deploy', '--dry-run', '--env', $Environment)
}

# --- Results ----------------------------------------------------------------
Write-Host ''
Write-Host "=== Results: $passed/$total passed, $failed failed ==="

if ($ShowDiagnostics -and $failed -gt 0) {
    Write-Host ''
    Write-Host 'Verbose diagnostics:'
    Write-Host '  - Ensure CLOUDFLARE_API_TOKEN environment variable is set'
    Write-Host '  - Run "npx wrangler whoami" to verify authentication'
    Write-Host "  - Check $ScriptRoot for manifest and state files"
    Write-Host '  - Verify wrangler.toml has correct D1 binding configuration'
}

if ($failed -gt 0) { exit 1 }
exit 0
