[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string] $SourceDirectory,
    [Parameter(Mandatory = $true)][string] $OutputPath
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$source = (Resolve-Path -LiteralPath $SourceDirectory).Path
$output = [System.IO.Path]::GetFullPath($OutputPath)
$parent = Split-Path -Parent $output
[System.IO.Directory]::CreateDirectory($parent) | Out-Null
if (Test-Path -LiteralPath $output) { Remove-Item -LiteralPath $output -Force }

$stream = [System.IO.File]::Open($output, [System.IO.FileMode]::CreateNew)
try {
    $archive = [System.IO.Compression.ZipArchive]::new($stream, [System.IO.Compression.ZipArchiveMode]::Create, $false)
    try {
        $files = Get-ChildItem -LiteralPath $source -File -Recurse | Sort-Object { $_.FullName.Substring($source.Length) }
        foreach ($file in $files) {
            $relative = $file.FullName.Substring($source.Length).TrimStart('\', '/').Replace('\', '/')
            $entry = $archive.CreateEntry($relative, [System.IO.Compression.CompressionLevel]::Optimal)
            $entry.LastWriteTime = [DateTimeOffset]::new(2020, 1, 1, 0, 0, 0, [TimeSpan]::Zero)
            $input = $file.OpenRead()
            $entryStream = $entry.Open()
            try { $input.CopyTo($entryStream) }
            finally { $entryStream.Dispose(); $input.Dispose() }
        }
    } finally { $archive.Dispose() }
} finally { $stream.Dispose() }

