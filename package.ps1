param([switch]$SkipBuild, [string]$ExecutablePath, [string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$version = '0.9.1'
$name = "LiquidGlassControl-$version-win11-x64"
$dist = if ($OutputDirectory) { [IO.Path]::GetFullPath($OutputDirectory) } else { Join-Path $PSScriptRoot 'dist' }
$stage = Join-Path $PSScriptRoot ('build\package-' + [guid]::NewGuid().ToString('N'))
if ($ExecutablePath) {
    $source = (Resolve-Path -LiteralPath $ExecutablePath).Path
} elseif (-not $SkipBuild) {
    & (Join-Path $PSScriptRoot 'build.ps1') -OutputName LiquidGlassControl.release.exe
    $source = Join-Path $PSScriptRoot 'build\LiquidGlassControl.release.exe'
} else {
    $source = Join-Path $PSScriptRoot 'build\LiquidGlassControl.exe'
}
if (-not (Test-Path -LiteralPath $source)) { throw 'Build the application first.' }
New-Item -ItemType Directory -Path $stage -Force | Out-Null
New-Item -ItemType Directory -Path $dist -Force | Out-Null
$exe = Join-Path $stage 'LiquidGlassControl.exe'
Copy-Item -LiteralPath $source -Destination $exe
# Use a clean folder: the test must not consume the developer's settings or backups.
$test = Start-Process -FilePath $exe -ArgumentList '--self-test' -WindowStyle Hidden -PassThru
if (-not $test.WaitForExit(20000)) { throw 'Release self-test timed out.' }
$test.Refresh()
if ($test.ExitCode -ne 0) { throw "Release self-test failed: $($test.ExitCode)" }
Get-Content -LiteralPath (Join-Path $stage 'self-test.txt')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'release\Quick Start.txt') -Destination $stage
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'release\Create Desktop Shortcut.cmd') -Destination $stage
# Explicit file list prevents shipping settings, test reports, personal data or recovery journals.
$files = @($exe, (Join-Path $stage 'Quick Start.txt'), (Join-Path $stage 'Create Desktop Shortcut.cmd'))
$zip = Join-Path $dist ($name + '.zip')
Compress-Archive -LiteralPath $files -DestinationPath $zip -CompressionLevel Optimal -Force
$standalone = Join-Path $dist ($name + '.exe')
Copy-Item -LiteralPath $exe -Destination $standalone -Force
$hashes = foreach ($file in @($zip, $standalone)) {
    $hash = Get-FileHash -LiteralPath $file -Algorithm SHA256
    '{0}  {1}' -f $hash.Hash.ToLowerInvariant(), [IO.Path]::GetFileName($file)
}
$hashes | Set-Content -LiteralPath (Join-Path $dist 'SHA256SUMS.txt') -Encoding ascii
Write-Host "Portable package: $zip"
Write-Host "Standalone EXE: $standalone"
