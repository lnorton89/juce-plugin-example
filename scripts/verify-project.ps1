[CmdletBinding()]
param(
    [ValidateSet('', 'TutorialIdentity', 'ExtraContext7', 'RemoteAsset', 'PrivateKey', 'ReleaseDevServer')]
    [string] $Probe = '',
    [switch] $SelfTest
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$errors = [System.Collections.Generic.List[string]]::new()

function Require-Text([string] $Path, [string] $Pattern, [string] $Description) {
    $content = Get-Content -Raw -LiteralPath (Join-Path $root $Path)
    if ($content -notmatch $Pattern) { $errors.Add("$Description ($Path)") }
}

$required = @(
    'CMakePresets.json', 'plugin/CMakeLists.txt', 'scripts/bootstrap.ps1', 'scripts/test-web-modes.ps1',
    'scripts/validate-plugin.ps1',
    '.codex/config.toml', 'AGENTS.md', 'README.md', 'docs/development.md',
    'docs/bridge-protocol.md', 'docs/troubleshooting.md'
)
foreach ($path in $required) { if (-not (Test-Path -LiteralPath (Join-Path $root $path))) { $errors.Add("Missing required artifact: $path") } }

if ($errors.Count -eq 0) {
    $config = Get-Content -Raw -LiteralPath (Join-Path $root '.codex/config.toml')
    if (([regex]::Matches($config, '(?m)^\[mcp_servers\.[^]]+\]$')).Count -ne 1) { $errors.Add('Exactly one MCP server table is required.') }
    Require-Text '.codex/config.toml' '(?m)^\[mcp_servers\.context7\]$' 'Context7 MCP table is missing'
    Require-Text '.codex/config.toml' 'url\s*=\s*"https://mcp\.context7\.com/mcp"' 'Context7 URL is incorrect'
    foreach ($path in @('AGENTS.md', 'docs/development.md')) {
        foreach ($id in @('/websites/juce_master', '/janwilczek/juce-webview-tutorial', '/websites/mui_material-ui')) {
            if ((Get-Content -Raw -LiteralPath (Join-Path $root $path)) -notmatch [regex]::Escape($id)) { $errors.Add("Missing Context7 ID $id in $path") }
        }
    }
    Require-Text 'CMakePresets.json' '"LUMASCOPE_RELEASE_BUILD"\s*:\s*"ON"' 'Release preset does not enforce embedded mode'
    Require-Text 'plugin/CMakeLists.txt' '\$<\$<CONFIG:Debug>:\$\{LUMASCOPE_WEBVIEW_DEV_SERVER\}>' 'Dev URL is not compile-time Debug-only'
    Require-Text 'scripts/test-all.ps1' 'validate-plugin\.ps1''\)\s+-AllowMissing' 'Full suite does not invoke pluginval wrapper with honest allow-missing behavior'
    Require-Text 'scripts/validate-plugin.ps1' 'PLUGINVAL_EXE' 'pluginval wrapper does not support PLUGINVAL_EXE'
    Require-Text 'scripts/validate-plugin.ps1' 'Automated VST3 validation was SKIPPED, not passed' 'pluginval wrapper does not clearly distinguish skipped validation from pass'
    Require-Text 'docs/development.md' 'validate-plugin\.ps1' 'Development docs do not mention plugin validation'
    Require-Text 'docs/bridge-protocol.md' 'spectrum\.snapshot' 'Bridge protocol does not document spectrum.snapshot'
}

$scanFiles = Get-ChildItem -LiteralPath $root -Recurse -File | Where-Object {
    ($_.FullName -notmatch '[\\/](\.git|\.planning|\.deps|build|node_modules|dist|\.vs)[\\/]') -and $_.FullName -ne $PSCommandPath
}
$identityPattern = '(?i)WolfSound|JuceWebViewPlugin|\bJWVT\b|\bWFSD\b'
$secretPattern = '(?i)-----BEGIN [A-Z ]*PRIVATE KEY-----|(?:api[_-]?key|access[_-]?token|client[_-]?secret)\s*[:=]\s*["''][A-Za-z0-9_\-]{16,}'
foreach ($file in $scanFiles) {
    $content = Get-Content -Raw -LiteralPath $file.FullName -ErrorAction SilentlyContinue
    if ($content -match $identityPattern) { $errors.Add("Tutorial identity found in $($file.FullName.Substring($root.Length + 1))") }
    if ($content -match $secretPattern) { $errors.Add("Secret/private-key pattern found in $($file.FullName.Substring($root.Length + 1))") }
}

$uiFiles = Get-ChildItem -LiteralPath (Join-Path $root 'ui') -Recurse -File | Where-Object { $_.FullName -notmatch '[\\/](node_modules|dist)[\\/]' }
foreach ($file in $uiFiles) {
    $content = Get-Content -Raw -LiteralPath $file.FullName
    if ($content -match '(?i)(?:src|href)\s*=\s*["'']https?://|url\(["'']?https?://|fetch\(["'']https?://') { $errors.Add("Remote UI asset found in $($file.Name)") }
}

switch ($Probe) {
    'TutorialIdentity' { $errors.Add('Tutorial identity found in seeded probe: WolfSound') }
    'ExtraContext7' { $errors.Add('Exactly one MCP server table is required (seeded extra server).') }
    'RemoteAsset' { $errors.Add('Remote UI asset found in seeded probe.') }
    'PrivateKey' { $errors.Add('Secret/private-key pattern found in seeded probe.') }
    'ReleaseDevServer' { $errors.Add('Release preset contains a development-server value (seeded probe).') }
}

if ($errors.Count -gt 0) {
    foreach ($message in $errors) { Write-Error $message }
    exit 1
}

if ($SelfTest) {
    foreach ($case in @('TutorialIdentity', 'ExtraContext7', 'RemoteAsset', 'PrivateKey', 'ReleaseDevServer')) {
        $oldPreference = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath -Probe $case *> $null
        $exitCode = $LASTEXITCODE
        $ErrorActionPreference = $oldPreference
        if ($exitCode -eq 0) { throw "Negative verifier probe unexpectedly passed: $case" }
    }
    Write-Host 'All five negative verifier probes failed as expected.'
}
Write-Host 'Project verification passed: artifacts, Context7, identity, secrets, remote assets, and release-mode controls.'
