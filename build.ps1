param([string]$Compiler, [ValidatePattern('^[A-Za-z0-9.-]+\.exe$')][string]$OutputName = 'LiquidGlassControl.exe', [string]$WinrtInclude, [string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
if (-not $Compiler) {
    $found = Get-Command clang++.exe -ErrorAction SilentlyContinue
    if ($found) { $Compiler = $found.Source }
    elseif (Test-Path 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe') {
        $Compiler = 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe'
    } else { throw 'Install a MinGW clang++ toolchain, or build CMakeLists.txt using Visual Studio.' }
}
# Candidates pass -OutputDirectory so a new build never overwrites the desktop target.
$buildDir = Join-Path $PSScriptRoot 'build'
$outputDir = if ($OutputDirectory) { [IO.Path]::GetFullPath($OutputDirectory) } else { $buildDir }
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
$engineIncludes = @()
if (-not $WinrtInclude -and (Test-Path (Join-Path $buildDir 'winrt\include\winrt\Microsoft.UI.Xaml.h'))) { $WinrtInclude = Join-Path $buildDir 'winrt\include' }
if ($WinrtInclude) { $engineIncludes = @('-I' + [IO.Path]::GetFullPath($WinrtInclude)) }
$hookObjects = @()
foreach ($unit in @('buffer','hook','trampoline','hde/hde64')) {
    $object = Join-Path $outputDir (($unit.Replace('/','-')) + '.o')
    & $Compiler --target=x86_64-w64-mingw32 -x c -O2 -c (Join-Path $PSScriptRoot "third_party/minhook/src/$unit.c") -o $object
    if ($LASTEXITCODE -ne 0) { throw 'MinHook compilation failed.' }
    $hookObjects += $object
}
& $Compiler --target=x86_64-w64-mingw32 -std=c++20 @engineIncludes -O2 -Wall -Wextra -shared -static (Join-Path $PSScriptRoot 'glass_engine.cpp') (Join-Path $PSScriptRoot 'glass_engine.def') @hookObjects -o (Join-Path $outputDir 'GlassEngine.dll') -lole32 -loleaut32 -luuid -lruntimeobject -lcomctl32 -ldwmapi -ladvapi32 -ld2d1
if ($LASTEXITCODE -ne 0) { throw 'Engine compilation failed.' }
$engineBytes = [IO.File]::ReadAllBytes((Join-Path $outputDir 'GlassEngine.dll'))
$noticeBytes = [IO.File]::ReadAllBytes((Join-Path $PSScriptRoot 'release\Third Party Notices.txt'))
$writer = [IO.BinaryWriter]::new([IO.File]::Create((Join-Path $outputDir 'engine.res')))
try {
    # Standard Win32 .res null record followed by RCDATA resource 201.
    foreach ($record in @(0,1,2)) {
        $payload = [byte[]]@()
        if ($record -eq 1) { $payload = $engineBytes } elseif ($record -eq 2) { $payload = $noticeBytes }
        $writer.Write([uint32]$payload.Length)
        $writer.Write([uint32]32); $writer.Write([uint16]65535); $writer.Write([uint16]$(if ($record) { 10 } else { 0 }))
        $writer.Write([uint16]65535); $writer.Write([uint16]$(if ($record -eq 1) { 201 } elseif ($record -eq 2) { 202 } else { 0 }))
        $writer.Write([uint32]0); $writer.Write([uint16]0); $writer.Write([uint16]0); $writer.Write([uint32]0); $writer.Write([uint32]0)
        if ($record) { $writer.Write([byte[]]$payload); while ($writer.BaseStream.Position % 4) { $writer.Write([byte]0) } }
    }
} finally { $writer.Dispose() }
& $Compiler --target=x86_64-w64-mingw32 -std=c++17 -O2 -Wall -Wextra -municode -mwindows -static (Join-Path $PSScriptRoot 'main.cpp') (Join-Path $PSScriptRoot 'assets\icon.res') (Join-Path $outputDir 'engine.res') -o (Join-Path $outputDir $OutputName) -ldwmapi -lshell32 -luser32 -lgdi32 -ladvapi32 -lole32 -lcomctl32 -luxtheme -luuid
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed.' }
Write-Host "Built: $outputDir\$OutputName"
