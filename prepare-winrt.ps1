param([string]$OutputDirectory = (Join-Path $PSScriptRoot 'build\winrt'), [string]$SdkMetadataDirectory = 'sdk')
$ErrorActionPreference = 'Stop'
# Build-time dependencies only. The delivered EXE uses Windows' own frameworks.
$packages = @{
    'microsoft.windows.cppwinrt' = '2.0.240405.15'
    'microsoft.windowsappsdk' = '1.6.250205002'
    'microsoft.web.webview2' = '1.0.2651.64'
}
$hashes = @{
    'microsoft.windows.cppwinrt' = 'E889007B5D9235931E7340DDF737D2C346EEBDD23C619F1F4F2426A2AAE47180'
    'microsoft.windowsappsdk' = '2DD7A4B34D85393858B322152022CADF34DC6E587DC950363A878E57D01D6D6B'
    'microsoft.web.webview2' = 'C7909CDAB68A56D1B8EEB1D5B18B0E00122ED812A075A8BE785ED48DE689EECA'
}
foreach ($name in $packages.Keys) {
    $version = $packages[$name]
    $destination = Join-Path $OutputDirectory "$name-$version"
    if (-not (Test-Path -LiteralPath (Join-Path $destination '[Content_Types].xml') -PathType Leaf)) {
        New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
        $archive = Join-Path $OutputDirectory "$name-$version.zip"
        Invoke-WebRequest "https://api.nuget.org/v3-flatcontainer/$name/$version/$name.$version.nupkg" -OutFile $archive
        if ((Get-FileHash -LiteralPath $archive).Hash -ne $hashes[$name]) { throw "Package checksum mismatch: $name" }
        Expand-Archive -LiteralPath $archive -DestinationPath $destination -Force
    }
}
$generator = Join-Path $OutputDirectory 'microsoft.windows.cppwinrt-2.0.240405.15\bin\cppwinrt.exe'
$metadata = Join-Path $OutputDirectory 'microsoft.windowsappsdk-1.6.250205002\lib\uap10.0'
$composition = Join-Path $OutputDirectory 'microsoft.windowsappsdk-1.6.250205002\lib\uap10.0.18362'
$webview = Join-Path $OutputDirectory 'microsoft.web.webview2-1.0.2651.64\lib\Microsoft.Web.WebView2.Core.winmd'
$headers = Join-Path $OutputDirectory 'include'
& $generator -input $SdkMetadataDirectory $metadata $composition $webview -output $headers
if ($LASTEXITCODE -ne 0) { throw 'C++/WinRT header generation failed. Install the Windows 11 SDK.' }
Write-Host "Generated build headers: $headers"
