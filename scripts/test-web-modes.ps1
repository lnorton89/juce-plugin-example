[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$results = Join-Path $root 'build/web-mode-results'
$viteProcess = $null

function Invoke-Checked([string] $File, [string[]] $Arguments) {
    & $File @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$File failed with exit code $LASTEXITCODE" }
}

function Assert-ConfigureRejected([string] $Value) {
    $build = Join-Path $results 'hostile-url'
    $oldPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & cmake -S $root -B $build -G 'Visual Studio 16 2019' -A x64 "-DLUMASCOPE_WEBVIEW_DEV_SERVER=$Value" *> $null
    $exitCode = $LASTEXITCODE
    $ErrorActionPreference = $oldPreference
    if ($exitCode -eq 0) { throw "Unsafe development URL was accepted: $Value" }
}

function Stop-OwnedProcessTree($Process) {
    if ($Process -and -not $Process.HasExited) {
        & taskkill.exe /PID $Process.Id /T /F *> $null
    }
}

function Invoke-Smoke([string] $Preset, [string] $ExpectedSource, [string] $Case, [string] $ExpectedStatus, [string] $ExpectedError = '', [string] $Simulation = '') {
    Invoke-Checked 'cmake' @('--preset', $Preset)
    Invoke-Checked 'cmake' @('--build', '--preset', $Preset, '--target', 'LumaScope_Standalone', '--parallel', '4')
    $resultPath = Join-Path $results "$Case.json"
    Remove-Item -LiteralPath $resultPath -Force -ErrorAction SilentlyContinue
    $env:LUMASCOPE_SMOKE_RESULT_FILE = $resultPath
    if ($Simulation) { $env:LUMASCOPE_SIMULATE_WEB_FAILURE = $Simulation }
    else { Remove-Item Env:LUMASCOPE_SIMULATE_WEB_FAILURE -ErrorAction SilentlyContinue }
    $exe = Join-Path $root "build/$Preset/plugin/LumaScope_artefacts/Debug/Standalone/LumaScope.exe"
    $process = Start-Process -FilePath $exe -PassThru
    $deadline = (Get-Date).AddSeconds(20)
    while (-not (Test-Path -LiteralPath $resultPath) -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 100 }
    if (-not (Test-Path -LiteralPath $resultPath)) {
        if (-not $process.HasExited) { $process.Kill() }
        throw "$Case did not produce smoke JSON"
    }
    $result = Get-Content -Raw -LiteralPath $resultPath | ConvertFrom-Json
    if ($result.status -ne $ExpectedStatus -or $result.protocolVersion -ne 1 -or $result.uiSource -ne $ExpectedSource) {
        throw "$Case returned unexpected JSON: $(Get-Content -Raw -LiteralPath $resultPath)"
    }
    if ($ExpectedError -and $result.errorCode -ne $ExpectedError) { throw "$Case returned error code $($result.errorCode)" }
    Remove-Item Env:LUMASCOPE_SIMULATE_WEB_FAILURE -ErrorAction SilentlyContinue
    Write-Host "PASS ${Case}: $(Get-Content -Raw -LiteralPath $resultPath)"
}

try {
    New-Item -ItemType Directory -Force -Path $results | Out-Null
    foreach ($url in @(
        'http://localhost:5174', 'http://192.0.2.1:5174', 'https://127.0.0.1:5174',
        'http://127.0.0.1:5173', 'http://user:pass@127.0.0.1:5174',
        'http://127.0.0.1:5174/path', 'http://127.0.0.1:5174/?query=1',
        'http://127.0.0.1:5174/#fragment', 'not-a-url')) { Assert-ConfigureRejected $url }

    $releaseBuild = Join-Path $results 'release-dev-url'
    $oldPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & cmake -S $root -B $releaseBuild -G Ninja -DCMAKE_BUILD_TYPE=Release '-DLUMASCOPE_WEBVIEW_DEV_SERVER=http://127.0.0.1:5174' *> $null
    $exitCode = $LASTEXITCODE
    $ErrorActionPreference = $oldPreference
    if ($exitCode -eq 0) { throw 'Release configure accepted a development-server URL' }

    Invoke-Checked 'npm.cmd' @('--prefix', 'ui', 'run', 'build')
    $listener = Get-NetTCPConnection -LocalAddress '127.0.0.1' -LocalPort 5174 -State Listen -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($listener) { throw "Port 127.0.0.1:5174 is already in use by PID $($listener.OwningProcess). Stop that process before running this smoke test." }
    $viteProcess = Start-Process -FilePath 'cmd.exe' -ArgumentList @('/d', '/c', 'npm.cmd --prefix ui run dev -- --host 127.0.0.1 --port 5174 --strictPort') -PassThru -WindowStyle Hidden
    $deadline = (Get-Date).AddSeconds(30)
    do {
        try { $ready = (Invoke-WebRequest -UseBasicParsing -Uri 'http://127.0.0.1:5174/' -TimeoutSec 1).StatusCode -eq 200 } catch { $ready = $false }
        if (-not $ready) { Start-Sleep -Milliseconds 250 }
    } until ($ready -or (Get-Date) -ge $deadline -or $viteProcess.HasExited)
    if (-not $ready) { throw 'Vite did not become ready on http://127.0.0.1:5174' }
    Invoke-Smoke 'vs2019-vite' 'vite' 'vite-ready' 'ready'

    Stop-OwnedProcessTree $viteProcess
    $viteProcess = $null
    Invoke-Smoke 'vs2019-vite' 'vite' 'vite-unavailable' 'error' 'development_server_unavailable'
    Invoke-Smoke 'vs2019-debug' 'embedded' 'embedded-ready' 'ready'
    Invoke-Smoke 'vs2019-debug' 'embedded' 'webview2-fallback' 'error' 'webview2_unavailable' 'webview2'
    Invoke-Smoke 'vs2019-debug' 'embedded' 'resource-fallback' 'error' 'embedded_resource_unavailable' 'resource'
    Invoke-Smoke 'vs2019-debug' 'embedded' 'handshake-fallback' 'error' 'handshake_timeout' 'handshake'
} finally {
    Remove-Item Env:LUMASCOPE_SMOKE_RESULT_FILE -ErrorAction SilentlyContinue
    Remove-Item Env:LUMASCOPE_SIMULATE_WEB_FAILURE -ErrorAction SilentlyContinue
    Stop-OwnedProcessTree $viteProcess
}
