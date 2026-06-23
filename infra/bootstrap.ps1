# bootstrap.ps1 — LumaScope Activation Infrastructure Bootstrap
# Creates Cloudflare D1 databases and writes IDs to generated-state.json.
# Usage: .\infra\bootstrap.ps1 [-Environment local|preview|production] [-Force] [-SkipDbCreate]

[CmdletBinding()]
param(
    [ValidateSet('local', 'preview', 'production')]
    [string] $Environment = 'local',
    [switch] $Force,
    [switch] $SkipDbCreate
)

$ErrorActionPreference = 'Stop'

# --- Dot-source shared module ---
. (Join-Path $PSScriptRoot 'common.ps1')

function Invoke-Checked {
    param([string] $File, [string[]] $Arguments)
    $ErrorActionPreference = 'Stop'
    Write-Host "> $File $($Arguments -join ' ')"
    & $File @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$File failed with exit code $LASTEXITCODE" }
}

# --- Prerequisites -----------------------------------------------------------
Write-Host '=== Prerequisites ==='

$manifestPath = Get-ManifestPath
if (-not (Test-Path -LiteralPath $manifestPath)) {
    Write-Host "[FAIL] Manifest not found at $manifestPath"
    exit 1
}
Write-Host "[PASS] Manifest exists: $manifestPath"

# Check npx/npm availability
$npxCheck = Get-Command 'npx.cmd' -ErrorAction SilentlyContinue
if (-not $npxCheck) {
    Write-Host '[FAIL] npx.cmd not found. Node.js and npm must be installed and on PATH.'
    exit 1
}
Write-Host '[PASS] npx.cmd available'

# Check Cloudflare authentication
if (-not (Test-CloudflareToken)) {
    Write-Host '[FAIL] Not authenticated with Cloudflare. Run: npx wrangler login'
    Write-Host '       Alternatively, set CLOUDFLARE_API_TOKEN environment variable.'
    exit 1
}
Write-Host '[PASS] Cloudflare authentication OK'

# Check required env vars for the target environment
$manifest = Get-Manifest
$envConfig = $manifest.environments.$Environment

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
    Write-Host "[FAIL] Required environment variables are missing for '$Environment'."
    Write-Host "       Set them and re-run. Values are never echoed or written to files."
    exit 1
}
Write-Host '[PASS] Required environment variables present'

# --- Existing state check ----------------------------------------------------
$statePath = Get-GeneratedStatePath
$stateValid = Test-GeneratedState

if ($stateValid -and -not $Force) {
    Write-Host ''
    Write-Host "Generated state already exists and is valid at: $statePath"
    $response = Read-Host 'Continue and overwrite? [y/N]'
    if ($response -notin @('y', 'Y', 'yes', 'Yes')) {
        Write-Host 'Aborted by user.'
        exit 0
    }
}

# --- Gather account information ----------------------------------------------
Write-Host ''
Write-Host '=== Account Information ==='

$whoamiOutput = & npx.cmd wrangler whoami 2>&1
$accountId   = ''
$accountEmail = ''

# Parse "Account ID   xyz" from wrangler whoami output
foreach ($line in $whoamiOutput) {
    if ($line -match 'Account\s*ID\s*\|?\s*([a-f0-9]{32}|[a-f0-9]{16,})') {
        $accountId = $matches[1]
    }
    if ($line -match 'Email\s*\|?\s*(\S+@\S+)') {
        $accountEmail = $matches[1]
    }
}

if (-not $accountId) {
    # Try CLOUDFLARE_ACCOUNT_ID env var as fallback
    $accountId = [Environment]::GetEnvironmentVariable('CLOUDFLARE_ACCOUNT_ID')
    if ([string]::IsNullOrEmpty($accountId)) {
        Write-Host '[FAIL] Could not determine Cloudflare Account ID. Set CLOUDFLARE_ACCOUNT_ID env var.'
        exit 1
    }
}
Write-Host "[INFO] Account ID: $accountId"

# Get wrangler version
$versionOutput = & npx.cmd wrangler --version 2>&1
$wranglerVersion = if ($LASTEXITCODE -eq 0) { ($versionOutput -join ' ').Trim() } else { 'unknown' }

# --- D1 Database Creation ----------------------------------------------------
$d1DatabaseId   = ''
$d1DatabaseName = ''

if (-not $SkipDbCreate) {
    Write-Host ''
    Write-Host '=== D1 Database ==='

    $d1Name = $envConfig.worker.d1_database_name
    Write-Host "Creating D1 database: $d1Name ..."

    $d1Output = & npx.cmd wrangler d1 create $d1Name 2>&1
    if ($LASTEXITCODE -ne 0) {
        $errorText = $d1Output -join "`n"
        if ($errorText -match 'already exists') {
            Write-Host '[INFO] D1 database already exists; fetching ID...'
            $listOutput = & npx.cmd wrangler d1 list 2>&1
            if ($LASTEXITCODE -eq 0) {
                $listText = $listOutput -join "`n"
                if ($listText -match [regex]::Escape($d1Name) -and $listText -match '([a-f0-9\-]{36})') {
                    $d1DatabaseId = $matches[1]
                    $d1DatabaseName = $d1Name
                }
            }
            if (-not $d1DatabaseId) {
                Write-Host '[FAIL] D1 database already exists but could not parse its ID from "wrangler d1 list".'
                Write-Host '       Run "npx wrangler d1 list" manually and update generated-state.json.'
                exit 1
            }
        } else {
            Write-Host "[FAIL] Failed to create D1 database: $errorText"
            exit 1
        }
    } else {
        # Parse successful creation output for database_id and database_name
        $d1Text = $d1Output -join "`n"
        if ($d1Text -match 'database_name\s*=\s*"([^"]+)"') {
            $d1DatabaseName = $matches[1]
        }
        if ($d1Text -match 'database_id\s*=\s*"([^"]+)"') {
            $d1DatabaseId = $matches[1]
        }
        # Fallback: if the pattern doesn't match, try JSON output
        if (-not $d1DatabaseId) {
            $d1Json = $d1Output -join "`n" | ConvertFrom-Json -ErrorAction SilentlyContinue
            if ($d1Json -and $d1Json.uuid) {
                $d1DatabaseId = $d1Json.uuid
            }
            if ($d1Json -and $d1Json.name) {
                $d1DatabaseName = $d1Json.name
            }
        }
        if (-not $d1DatabaseId) {
            Write-Host '[FAIL] Could not parse D1 database ID from wrangler output.'
            Write-Host "Raw output: $d1Text"
            exit 1
        }
    }

    Write-Host "[PASS] D1 database '$d1DatabaseName' ready (ID: $d1DatabaseId)"
} else {
    Write-Host '[SKIP] D1 database creation skipped (-SkipDbCreate)'
}

# --- Update generated-state.json ---------------------------------------------
Write-Host ''
Write-Host '=== Updating Generated State ==='

$state = Read-GeneratedState
if (-not $state) {
    $state = [PSCustomObject]@{
        account_id          = ''
        account_email       = ''
        bootstrap_timestamp = ''
        wrangler_version    = ''
        environments        = [PSCustomObject]@{
            local      = [PSCustomObject]@{ worker_id = ''; worker_name = ''; d1_database_id = ''; d1_database_name = '' }
            preview    = [PSCustomObject]@{ worker_id = ''; worker_name = ''; d1_database_id = ''; d1_database_name = '' }
            production = [PSCustomObject]@{ worker_id = ''; worker_name = ''; d1_database_id = ''; d1_database_name = '' }
        }
    }
}

$state.account_id          = $accountId
$state.account_email       = $accountEmail
$state.bootstrap_timestamp = (Get-Date -Format 'o')
$state.wrangler_version    = $wranglerVersion

# Update the specific environment
$envState = $state.environments.$Environment
$envState.worker_name      = $envConfig.worker.name
$envState.d1_database_name = $d1DatabaseName
if ($d1DatabaseId) {
    $envState.d1_database_id = $d1DatabaseId
}

Write-GeneratedState -State $state
Write-Host "[PASS] Generated state written to: $(Get-GeneratedStatePath)"

# --- Summary -----------------------------------------------------------------
Write-Host ''
Write-Host '=== Bootstrap Summary ==='
Write-Host "  Environment:   $Environment"
Write-Host "  Account ID:    $accountId"
Write-Host "  Worker name:   $($envConfig.worker.name)"
if ($d1DatabaseName) {
    Write-Host "  D1 database:   $d1DatabaseName ($d1DatabaseId)"
}
Write-Host "  State file:    $(Get-GeneratedStatePath)"
Write-Host ''
Write-Host 'Bootstrap complete. Run infra/deploy.ps1 to apply migrations and deploy the Worker.'
exit 0
