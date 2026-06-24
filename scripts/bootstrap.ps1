[CmdletBinding()]
param(
    [ValidateSet('vs2019-debug', 'vs2019-vite', 'vs2019-release', 'ninja-debug', 'ninja-release')]
    [string] $Preset = 'vs2019-debug',
    [switch] $SkipBuild
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'config.ps1')

function Invoke-Checked([string] $File, [string[]] $Arguments) {
    Write-Host "> $File $($Arguments -join ' ')"
    & $File @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$File failed with exit code $LASTEXITCODE" }
}

Write-Host 'Checking the Windows build environment. This script never installs system software.'
& (Join-Path $PSScriptRoot 'check-environment.ps1')
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Invoke-Checked 'npm.cmd' @('--prefix', 'ui', 'ci')
Invoke-Checked 'cmake' @('--preset', $Preset)
if (-not $SkipBuild) {
    Invoke-Checked 'cmake' @('--build', '--preset', $Preset, '--target', $script:STANDALONE_TARGET_NAME, $script:VST3_TARGET_NAME, $script:TEST_TARGET_NAME, '--parallel', '4')
}

Write-Host "Bootstrap complete for $Preset. Dependencies remain in the ignored .deps and ui/node_modules caches."
