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
& $Compiler --target=x86_64-w64-mingw32 -std=c++17 -O2 -Wall -Wextra -shared -static (Join-Path $PSScriptRoot 'glass_engine.cpp') (Join-Path $PSScriptRoot 'glass_engine.def') -o (Join-Path $outputDir 'GlassEngine.dll') -lole32 -loleaut32 -luuid -lruntimeobject -lcomctl32 -ldwmapi -ladvapi32
if ($LASTEXITCODE -ne 0) { throw 'Engine compilation failed.' }
$engineBytes = [IO.File]::ReadAllBytes((Join-Path $outputDir 'GlassEngine.dll'))
$writer = [IO.BinaryWriter]::new([IO.File]::Create((Join-Path $outputDir 'engine.res')))
try {
    # Standard Win32 .res null record followed by RCDATA resource 201.
    foreach ($record in @(0,1)) {
        $writer.Write([uint32]$(if ($record) { $engineBytes.Length } else { 0 }))
        $writer.Write([uint32]32); $writer.Write([uint16]65535); $writer.Write([uint16]$(if ($record) { 10 } else { 0 }))
        $writer.Write([uint16]65535); $writer.Write([uint16]$(if ($record) { 201 } else { 0 }))
        $writer.Write([uint32]0); $writer.Write([uint16]0); $writer.Write([uint16]0); $writer.Write([uint32]0); $writer.Write([uint32]0)
        if ($record) { $writer.Write($engineBytes); while ($writer.BaseStream.Position % 4) { $writer.Write([byte]0) } }
    }
} finally { $writer.Dispose() }
& $Compiler --target=x86_64-w64-mingw32 -std=c++17 -O2 -Wall -Wextra -municode -mwindows -static (Join-Path $PSScriptRoot 'main.cpp') (Join-Path $PSScriptRoot 'assets\icon.res') (Join-Path $outputDir 'engine.res') -o (Join-Path $outputDir $OutputName) -ldwmapi -lshell32 -luser32 -lgdi32 -ladvapi32 -lole32 -lcomctl32 -luxtheme -luuid
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed.' }
Write-Host "Built: $outputDir\$OutputName"
