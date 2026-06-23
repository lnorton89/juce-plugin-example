# common.ps1 — LumaScope Activation Infrastructure Shared Module
# Dot-source from infra scripts: . .\infra\common.ps1

[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$ScriptRoot = Split-Path -Parent $PSCommandPath
$RepoRoot   = Split-Path -Parent $ScriptRoot

function Get-ManifestPath {
    $ErrorActionPreference = 'Stop'
    return Join-Path $RepoRoot 'infra/manifest.yaml'
}

function Get-GeneratedStatePath {
    $ErrorActionPreference = 'Stop'
    return Join-Path $RepoRoot 'infra/generated-state.json'
}

function Get-Manifest {
    $ErrorActionPreference = 'Stop'

    $manifestPath = Get-ManifestPath
    if (-not (Test-Path -LiteralPath $manifestPath)) {
        throw "Manifest not found at: $manifestPath"
    }
    Write-Host "Parsing $manifestPath ..."
    $json = & npx.cmd js-yaml $manifestPath 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "npx js-yaml failed. Install it with: npm install -g js-yaml"
    }
    return ($json | ConvertFrom-Json)
}

function Test-GeneratedState {
    $ErrorActionPreference = 'Stop'

    $path = Get-GeneratedStatePath
    if (-not (Test-Path -LiteralPath $path)) { return $false }
    try {
        $state = Get-Content -Raw -LiteralPath $path -ErrorAction Stop | ConvertFrom-Json -ErrorAction Stop
        if (-not $state.account_id) { return $false }
        foreach ($env in @('local', 'preview', 'production')) {
            $e = $state.environments.$env
            if (-not $e) { return $false }
            if (-not $e.worker_name) { return $false }
            if (-not $e.d1_database_name) { return $false }
            if (-not $e.d1_database_id) { return $false }
        }
        return $true
    } catch {
        return $false
    }
}

function Read-GeneratedState {
    $ErrorActionPreference = 'Stop'

    $path = Get-GeneratedStatePath
    if (-not (Test-Path -LiteralPath $path)) { return $null }
    try {
        return Get-Content -Raw -LiteralPath $path -ErrorAction Stop | ConvertFrom-Json -ErrorAction Stop
    } catch {
        return $null
    }
}

function Write-GeneratedState {
    param($State)
    $ErrorActionPreference = 'Stop'

    $path      = Get-GeneratedStatePath
    $parent    = Split-Path -Parent $path
    $tempName  = [System.IO.Path]::GetRandomFileName()
    $tempPath  = Join-Path $parent $tempName

    $State | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $tempPath -Encoding UTF8 -ErrorAction Stop

    if (Test-Path -LiteralPath $path) {
        Remove-Item -LiteralPath $path -Force -ErrorAction Stop
    }
    Move-Item -LiteralPath $tempPath -Destination $path -Force -ErrorAction Stop
}

function Test-CloudflareToken {
    $ErrorActionPreference = 'Stop'

    $workerDir   = Join-Path $RepoRoot 'worker'
    $originalDir = Get-Location
    try {
        Set-Location -LiteralPath $workerDir -ErrorAction Stop
        $null = & npx.cmd wrangler whoami 2>&1
        return $LASTEXITCODE -eq 0
    } finally {
        Set-Location -LiteralPath $originalDir -ErrorAction SilentlyContinue
    }
}

function Invoke-Wrangler {
    param([string[]] $Arguments)
    $ErrorActionPreference = 'Stop'

    $workerDir   = Join-Path $RepoRoot 'worker'
    $originalDir = Get-Location
    try {
        Set-Location -LiteralPath $workerDir -ErrorAction Stop
        Write-Host "> npx wrangler $($Arguments -join ' ')"
        & npx.cmd wrangler @Arguments
        if ($LASTEXITCODE -ne 0) {
            throw "wrangler failed with exit code $LASTEXITCODE"
        }
    } finally {
        Set-Location -LiteralPath $originalDir -ErrorAction SilentlyContinue
    }
}
