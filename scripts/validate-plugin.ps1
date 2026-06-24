[CmdletBinding()]
param(
    [string] $PluginvalPath = '',
    [string] $PluginPath = '',
    [int] $StrictnessLevel = 10,
    [int] $TimeoutMs = 60000,
    [switch] $SkipGuiTests,
    [switch] $AllowMissing
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'config.ps1')

function Resolve-Pluginval {
    param([string] $ExplicitPath)

    if (-not [string]::IsNullOrWhiteSpace($ExplicitPath)) {
        $resolved = Resolve-Path -LiteralPath $ExplicitPath -ErrorAction SilentlyContinue
        if ($null -eq $resolved) { return $null }
        return $resolved.ProviderPath
    }

    if (-not [string]::IsNullOrWhiteSpace($env:PLUGINVAL_EXE)) {
        $resolved = Resolve-Path -LiteralPath $env:PLUGINVAL_EXE -ErrorAction SilentlyContinue
        if ($null -ne $resolved) { return $resolved.ProviderPath }
        return $null
    }

    $command = Get-Command pluginval.exe -ErrorAction SilentlyContinue
    if ($null -eq $command) { $command = Get-Command pluginval -ErrorAction SilentlyContinue }
    if ($null -eq $command) { return $null }
    return $command.Source
}

function Resolve-DefaultPluginPath {
    $candidates = @(
        (Join-Path $root "build/vs2019-debug/plugin/$($script:ARTEFACTS_DIR_NAME)/Debug/VST3/$($script:PRODUCT_NAME).vst3"),
        (Join-Path $root "build/vs2019-debug/plugin/$($script:ARTEFACTS_DIR_NAME)/Debug/VST3/$($script:PRODUCT_NAME).vst3/Contents/x86_64-win/$($script:PRODUCT_NAME).vst3"),
        (Join-Path $root "build/vs2019-vite/plugin/$($script:ARTEFACTS_DIR_NAME)/Debug/VST3/$($script:PRODUCT_NAME).vst3")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) { return (Resolve-Path -LiteralPath $candidate).ProviderPath }
    }

    return $candidates[0]
}

$pluginval = Resolve-Pluginval -ExplicitPath $PluginvalPath
if ($null -eq $pluginval) {
    $message = 'pluginval executable not found. Provide -PluginvalPath, set PLUGINVAL_EXE, or add pluginval to PATH.'
    if ($AllowMissing) {
        Write-Warning "$message Automated VST3 validation was SKIPPED, not passed."
        exit 0
    }

    Write-Error $message
    exit 2
}

if (-not (Test-Path -LiteralPath $pluginval -PathType Leaf)) {
    Write-Error "Resolved pluginval path is not an executable file: $pluginval"
    exit 2
}

if ([string]::IsNullOrWhiteSpace($PluginPath)) {
    $PluginPath = Resolve-DefaultPluginPath
}

if (-not (Test-Path -LiteralPath $PluginPath)) {
    Write-Error "VST3 artifact not found: $PluginPath. Build it with: cmake --build --preset vs2019-debug --target $($script:VST3_TARGET_NAME) --parallel 4"
    exit 3
}

$resolvedPlugin = (Resolve-Path -LiteralPath $PluginPath).ProviderPath
Write-Host "pluginval: $pluginval"
try {
    & $pluginval --version
} catch {
    Write-Host 'pluginval version probe was not supported by this executable.'
}

Write-Host "Validating VST3: $resolvedPlugin"
Write-Host "Strictness level: $StrictnessLevel"

$arguments = @('--validate', $resolvedPlugin, '--strictness-level', $StrictnessLevel, '--timeout-ms', $TimeoutMs)
if ($SkipGuiTests) {
    Write-Host 'Skipping pluginval GUI tests.'
    $arguments += '--skip-gui-tests'
}

& $pluginval @arguments
$exitCode = $LASTEXITCODE
if ($null -eq $exitCode) {
    $exitCode = if ($?) { 0 } else { 1 }
}
if ($exitCode -ne 0) {
    Write-Error "pluginval validation failed with exit code $exitCode."
    exit $exitCode
}

Write-Host 'pluginval validation passed.'
