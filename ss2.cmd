@echo off
where pwsh >nul 2>nul
if %errorlevel%==0 (
  pwsh -NoProfile -ExecutionPolicy Bypass -File "%~dp0ss2.ps1" %*
  exit /b %errorlevel%
)

where powershell.exe >nul 2>nul
if %errorlevel%==0 (
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0ss2.ps1" %*
  exit /b %errorlevel%
)

echo Neither pwsh nor powershell.exe was found in PATH.
exit /b 1
