[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
& npm.cmd --prefix ui run test:run
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& cmake --build --preset vs2019-debug --target LumaScopeNativeTests --parallel 4
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& ctest --preset vs2019-debug --output-on-failure
exit $LASTEXITCODE
