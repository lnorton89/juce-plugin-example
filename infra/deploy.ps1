# deploy.ps1 — LumaScope Activation Worker Deployment
# Applies D1 migrations, deploys the Worker, and injects secrets.
# Usage: .\infra\deploy.ps1 [-Environment local|preview|production] [-Force]

[CmdletBinding()]
param(
    [ValidateSet('local', 'preview', 'production')]
    [string] $Environment = 'local',
    [switch] $Force
)

$ErrorActionPreference = 'Stop'

# --- Dot-source shared module ---
. (Join-Path $PSScriptRoot 'common.ps1')

# --- Validation --------------------------------------------------------------
Write-Host '=== Validation ==='

$statePath = Get-GeneratedStatePath
if (-not (Test-Path -LiteralPath $statePath)) {
    Write-Host "[FAIL] Generated state not found at $statePath"
    Write-Host '       Run infra/bootstrap.ps1 first.'
    exit 1
}
if (-not (Test-GeneratedState)) {
    Write-Host "[FAIL] Generated state at $statePath is invalid or incomplete."
    Write-Host '       Run infra/bootstrap.ps1 -Force to regenerate.'
    exit 1
}
Write-Host '[PASS] Generated state exists and is valid'

if (-not (Test-CloudflareToken)) {
    Write-Host '[FAIL] Not authenticated with Cloudflare. Run: npx wrangler login'
    exit 1
}
Write-Host '[PASS] Cloudflare authentication OK'

$workerPkg = Join-Path $RepoRoot 'worker/package.json'
$wranglerCfg = Join-Path $RepoRoot 'worker/wrangler.toml'
if (-not (Test-Path -LiteralPath $workerPkg)) {
    Write-Host "[FAIL] Worker package.json not found at $workerPkg"
    exit 1
}
if (-not (Test-Path -LiteralPath $wranglerCfg)) {
    Write-Host "[FAIL] wrangler.toml not found at $wranglerCfg"
    exit 1
}
Write-Host '[PASS] Worker sources exist'

# For remote environments, validate secrets
$manifest = Get-Manifest
$envConfig = $manifest.environments.$Environment

if ($Environment -ne 'local') {
    $envVarsMissing = $false
    foreach ($secretName in ($envConfig.secrets | Get-Member -MemberType NoteProperty | Select-Object -ExpandProperty Name)) {
        $secretDef = $envConfig.secrets.$secretName
        if ($secretDef.required -eq $true) {
            $val = [Environment]::GetEnvironmentVariable($secretName)
            if ([string]::IsNullOrEmpty($val)) {
                Write-Host "[MISSING] $secretName"
                $envVarsMissing = $true
            } else {
                Write-Host "[SET]     $secretName"
            }
        }
    }
    if ($envVarsMissing) {
        Write-Host "[FAIL] Required environment variables missing for '$Environment'."
        exit 1
    }
}

# --- D1 Migrations -----------------------------------------------------------
Write-Host ''
Write-Host '=== D1 Migrations ==='

if ($Environment -eq 'local') {
    Write-Host 'Applying local D1 migrations...'
    Invoke-Wrangler @('d1', 'migrations', 'apply', '--env', $Environment, '--local')
} else {
    Write-Host "Applying D1 migrations for '$Environment'..."
    Invoke-Wrangler @('d1', 'migrations', 'apply', '--env', $Environment)
}
Write-Host '[PASS] D1 migrations applied'

# --- Worker Deploy -----------------------------------------------------------
if ($Environment -eq 'local') {
    Write-Host ''
    Write-Host '[SKIP] Local environment: no remote deployment needed.'
    Write-Host '[INFO] Run "npm run dev" in worker/ for local development server.'
    exit 0
}

Write-Host ''
Write-Host "=== Deploy Worker ($Environment) ==="

Invoke-Wrangler @('deploy', '--env', $Environment)
Write-Host '[PASS] Worker deployed'
# Deployment URL is printed by wrangler in its output above

# --- Secret Injection --------------------------------------------------------
Write-Host ''
Write-Host '=== Secret Injection ==='

$secretsInjected = 0
foreach ($secretName in ($envConfig.secrets | Get-Member -MemberType NoteProperty | Select-Object -ExpandProperty Name)) {
    $secretDef = $envConfig.secrets.$secretName
    if ($secretDef.required -eq $false) {
        $val = [Environment]::GetEnvironmentVariable($secretName)
        if ([string]::IsNullOrEmpty($val)) {
            Write-Host "[SKIP]  $secretName (optional, not set)"
            continue
        }
    } else {
        $val = [Environment]::GetEnvironmentVariable($secretName)
    }

    Write-Host "Injecting $secretName ..."
    $val | & npx.cmd wrangler secret put $secretName --env $Environment 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[FAIL] Failed to inject secret: $secretName"
        exit 1
    }
    $secretsInjected++
}
Write-Host "[PASS] $secretsInjected secrets injected"

# --- Summary -----------------------------------------------------------------
Write-Host ''
Write-Host '=== Deploy Summary ==='
Write-Host "  Environment: $Environment"
Write-Host "  Worker:      $($envConfig.worker.name)"
Write-Host "  D1 binding:  $($envConfig.worker.d1_binding)"
Write-Host "  Rate limit:  $($envConfig.worker.rate_limit_binding)"
Write-Host "  Secrets:     $secretsInjected injected"
Write-Host ''
Write-Host 'Deploy complete.'
exit 0
