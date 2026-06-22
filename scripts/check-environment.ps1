[CmdletBinding()]
param(
    [switch] $Json
)

$ErrorActionPreference = 'Stop'

function Get-CommandVersion([string] $Name, [string[]] $Arguments) {
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($null -eq $command) { return $null }
    $output = & $command.Source @Arguments 2>&1
    if ($LASTEXITCODE -ne 0) { return $null }
    return (($output | Select-Object -First 1) -replace '^[^0-9]*', '').Trim()
}

$windows = Get-CimInstance Win32_OperatingSystem
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = $null
if (Test-Path $vswhere) {
    $vsJson = & $vswhere -version '[16.0,17.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -latest -format json -utf8
    if ($LASTEXITCODE -eq 0 -and $vsJson) { $vs = ($vsJson | ConvertFrom-Json | Select-Object -First 1) }
}

$webView = $null
$webViewKeys = @(
    'HKLM:\SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}',
    'HKCU:\Software\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}'
)
foreach ($key in $webViewKeys) {
    $value = Get-ItemProperty -LiteralPath $key -ErrorAction SilentlyContinue
    if ($value -and $value.pv) { $webView = [string] $value.pv; break }
}

$result = [ordered]@{
    ok = $true
    windows = [ordered]@{ product = $windows.Caption; build = $windows.BuildNumber; architecture = $windows.OSArchitecture }
    visualStudio = if ($vs) { [ordered]@{ version = $vs.installationVersion; path = $vs.installationPath; cppWorkload = $true } } else { $null }
    cmake = Get-CommandVersion 'cmake' @('--version')
    node = Get-CommandVersion 'node' @('--version')
    npm = Get-CommandVersion 'npm.cmd' @('--version')
    ninja = Get-CommandVersion 'ninja' @('--version')
    vsDeveloperEnvironment = [bool] (Get-Command 'cl.exe' -ErrorAction SilentlyContinue)
    webView2Sdk = if (Test-Path (Join-Path $PSScriptRoot '../.deps/webview2/packages/Microsoft.Web.WebView2.1.0.4022.49/build/native/Microsoft.Web.WebView2.targets')) { '1.0.4022.49 (cached)' } else { 'not cached (CMake downloads the pinned SDK)' }
    webView2 = $webView
    errors = [System.Collections.Generic.List[string]]::new()
}

if (-not $IsWindows -and $PSVersionTable.PSEdition -eq 'Core') { $result.errors.Add('Windows is required.') }
if ($windows.OSArchitecture -ne '64-bit') { $result.errors.Add('64-bit Windows is required.') }
if (-not $vs) { $result.errors.Add('Visual Studio 2019 with the Desktop development with C++ workload is required. Install that workload; LumaScope will not substitute another compiler.') }
foreach ($tool in @('cmake', 'node', 'npm')) {
    if (-not $result[$tool]) { $result.errors.Add("$tool is required and was not found on PATH.") }
}
if (-not $result.ninja) { $result.errors.Add('Ninja was not found on PATH. Install Ninja or use the default Visual Studio 2019 presets.') }
if (-not $webView) { $result.errors.Add('Microsoft Edge WebView2 Evergreen Runtime is missing. Install or repair the Evergreen Runtime before launching LumaScope.') }
$result.ok = $result.errors.Count -eq 0

if ($Json) {
    $result | ConvertTo-Json -Depth 4
} else {
    Write-Host "Windows: $($result.windows.product) build $($result.windows.build) $($result.windows.architecture)"
    Write-Host "Visual Studio 2019: $($result.visualStudio.version)"
    Write-Host "CMake: $($result.cmake) | Node: $($result.node) | npm: $($result.npm) | Ninja: $($result.ninja)"
    Write-Host "VS developer environment active: $($result.vsDeveloperEnvironment) (required only for Ninja/MSVC command-line builds)"
    Write-Host "WebView2 SDK: $($result.webView2Sdk)"
    Write-Host "WebView2 Evergreen Runtime: $($result.webView2)"
    foreach ($message in $result.errors) { Write-Error $message }
}

if (-not $result.ok) { exit 1 }
