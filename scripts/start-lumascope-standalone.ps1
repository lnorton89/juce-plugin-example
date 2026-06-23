<#
.SYNOPSIS
  Launch LumaScope standalone executable with validation checks.

.DESCRIPTION
  Checks for build artifacts, WebView2 runtime, and preferred source
  preference before launching the standalone executable.

.PARAMETER Config
  Build configuration: Debug or Release. Default: Debug.

.PARAMETER BuildDir
  Path to the CMake build directory. Default: .\build\vs2019-vite

.PARAMETER Restart
  If specified, waits for the process to exit and restarts it (for
  testing source preference persistence across launches).

.EXAMPLE
  # Launch Debug build
  .\scripts\start-lumascope-standalone.ps1

  # Launch Release build
  .\scripts\start-lumascope-standalone.ps1 -Config Release

  # Launch and restart after exit
  .\scripts\start-lumascope-standalone.ps1 -Restart
#>

param(
  [ValidateSet('Debug', 'Release')]
  [string]$Config = 'Debug',

  [string]$BuildDir = ".\build\vs2019-vite",

  [switch]$Restart
)

$ErrorActionPreference = 'Stop'

# Resolve build directory
$resolvedBuild = Resolve-Path $BuildDir -ErrorAction SilentlyContinue
if (-not $resolvedBuild) {
  Write-Error "Build directory not found: $BuildDir"
  Write-Error "Run: cmake --preset vs2019-vite"
  exit 1
}

$exePath = Join-Path $resolvedBuild "plugin\$Config\LumaScope_Standalone.exe"
if (-not (Test-Path $exePath)) {
  Write-Error "Standalone executable not found: $exePath"
  Write-Error "Build with: cmake --build `"$resolvedBuild`" --target LumaScope_Standalone --config $Config"
  exit 1
}

# Check WebView2 runtime
$webviewCheck = Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}" -Name name -ErrorAction SilentlyContinue
if (-not $webviewCheck) {
  Write-Warning "WebView2 Runtime may not be installed."
  Write-Warning "Download from: https://developer.microsoft.com/en-us/microsoft-edge/webview2/"
}

# Check source preference file (informational)
$prefsFile = Join-Path $env:APPDATA "LumaScope\source-preference.json"
if (Test-Path $prefsFile) {
  try {
    $prefs = Get-Content $prefsFile -Raw | ConvertFrom-Json
    Write-Host "Saved source preference: $($prefs.mode) / $($prefs.displayName)"
  } catch {
    Write-Warning "Source preference file exists but is invalid: $prefsFile"
  }
} else {
  Write-Host "No saved source preference (first launch or reset)"
}

Write-Host "Launching LumaScope Standalone ($Config)..."
Write-Host "  Executable: $exePath"

do {
  $process = Start-Process -FilePath $exePath -NoNewWindow -PassThru -Wait
  Write-Host "LumaScope exited with code $($process.ExitCode)"

  if ($Restart) {
    Write-Host "Restarting in 2 seconds..."
    Start-Sleep -Seconds 2
  }
} while ($Restart)
