@echo off
setlocal
set "LG_PACKAGE_DIR=%~dp0"
powershell.exe -NoProfile -Command "$ErrorActionPreference='Stop'; try { $dir=$env:LG_PACKAGE_DIR; $exe=Join-Path $dir 'LiquidGlassControl.exe'; if(-not (Test-Path -LiteralPath $exe)){throw 'Extract the entire ZIP first.'}; $shell=New-Object -ComObject WScript.Shell; $link=$shell.CreateShortcut((Join-Path ([Environment]::GetFolderPath('Desktop')) 'Liquid Glass Control.lnk')); $link.TargetPath=$exe; $link.WorkingDirectory=$dir; $link.IconLocation=$exe+',0'; $link.Save(); Write-Host 'Desktop shortcut created.' } catch { Write-Error $_; exit 1 }"
if errorlevel 1 (
  echo Failed to create shortcut. You can still open LiquidGlassControl.exe directly.
  pause
  exit /b 1
)
pause
