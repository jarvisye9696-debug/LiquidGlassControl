param([string]$Compiler, [ValidatePattern('^[A-Za-z0-9.-]+\.exe$')][string]$OutputName = 'LiquidGlassControl.exe')
$ErrorActionPreference = 'Stop'
if (-not $Compiler) {
    $found = Get-Command clang++.exe -ErrorAction SilentlyContinue
    if ($found) { $Compiler = $found.Source }
    elseif (Test-Path 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe') {
        $Compiler = 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe'
    } else { throw 'Install a MinGW clang++ toolchain, or build CMakeLists.txt using Visual Studio.' }
}
$outputDir = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
& $Compiler --target=x86_64-w64-mingw32 -std=c++17 -O2 -Wall -Wextra -municode -mwindows -static (Join-Path $PSScriptRoot 'main.cpp') (Join-Path $PSScriptRoot 'assets\icon.res') -o (Join-Path $outputDir $OutputName) -ldwmapi -lshell32 -luser32 -lgdi32 -ladvapi32 -lole32 -lcomctl32 -luxtheme
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed.' }
Write-Host "Built: $outputDir\$OutputName"
