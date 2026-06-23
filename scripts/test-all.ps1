[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
& npm.cmd --prefix ui ci
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& npm.cmd --prefix ui run test:run
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& npm.cmd --prefix ui run build
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& npm.cmd --prefix ui run check:bundle
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& cmake --preset vs2019-debug
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& cmake --build --preset vs2019-debug --target LumaScope_Standalone LumaScope_VST3 LumaScopeNativeTests --parallel 4
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& ctest --preset vs2019-debug --output-on-failure
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'validate-plugin.ps1') -AllowMissing
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'test-web-modes.ps1')
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'verify-project.ps1') -SelfTest
exit $LASTEXITCODE
