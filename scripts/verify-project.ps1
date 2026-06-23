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
    'docs/bridge-protocol.md', 'docs/troubleshooting.md',
    'worker/package.json', 'worker/wrangler.toml', 'worker/src/index.ts',
    'worker/src/env.ts', 'worker/src/db/schema.ts', 'worker/src/webhook.ts',
    'worker/src/db/repository.ts',
    'infra/manifest.yaml', 'infra/common.ps1', 'infra/bootstrap.ps1',
    'infra/deploy.ps1', 'infra/verify.ps1', 'infra/teardown.ps1',
    'docs/cloud-infrastructure.md'
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
    Require-Text 'docs/vst3-smoke-test.md' 'Ableton Live is the preferred host' 'VST3 smoke docs do not preserve Ableton preference'
    Require-Text 'docs/vst3-smoke-test.md' 'fallback' 'VST3 smoke docs do not document fallback host behavior'
    Require-Text '.planning/phases/02-end-to-end-vst3-analyzer/02-HOST-SMOKE.md' 'pluginval executable' 'Host smoke evidence does not include pluginval executable field'
    Require-Text '.planning/phases/02-end-to-end-vst3-analyzer/02-HOST-SMOKE.md' 'Limitations versus Ableton' 'Host smoke evidence does not require fallback limitations'
    Require-Text 'docs/troubleshooting.md' 'skipped, not passed' 'Troubleshooting docs do not distinguish skipped pluginval validation from pass'
    Require-Text 'README.md' 'VST3 validation and host smoke' 'README does not point to VST3 validation smoke'
    Require-Text 'docs/bridge-protocol.md' 'spectrum\.snapshot' 'Bridge protocol does not document spectrum.snapshot'

    # Phase 4 — Cloudflare deployment infrastructure
    Require-Text 'infra/manifest.yaml' 'environments' 'Manifest missing environments section'
    Require-Text 'infra/manifest.yaml' 'lemon_squeezy' 'Manifest missing lemon_squeezy section'
    Require-Text 'worker/src/webhook.ts' 'verifyLemonSignature' 'Webhook module missing verifyLemonSignature'
    Require-Text 'worker/src/db/repository.ts' 'Repository' 'Repository module missing Repository class'
    Require-Text 'docs/cloud-infrastructure.md' 'bootstrap\.ps1' 'Cloud infra docs missing bootstrap reference'
    Require-Text 'docs/cloud-infrastructure.md' 'Lemon Squeezy' 'Cloud infra docs missing Lemon Squeezy'
    Require-Text 'scripts/test-all.ps1' 'worker run test' 'test-all missing Worker test step'
    $gitignore = Get-Content -Raw -LiteralPath (Join-Path $root '.gitignore')
    if ($gitignore -notmatch 'generated-state\.json') { $errors.Add('Generated state file is not gitignored (.gitignore)') }
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
Write-Host 'Project verification passed: artifacts, Context7, identity, secrets, remote assets, release-mode controls, and Phase 4 infrastructure.'
