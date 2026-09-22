@echo off
setlocal
set "SS2_ROOT=%~dp0"
set "WINPS=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"

if exist "%WINPS%" goto :winps

where pwsh.exe >nul 2>nul
if not errorlevel 1 goto :pwsh

where powershell.exe >nul 2>nul
if not errorlevel 1 goto :powershell_path

echo [SS2] PowerShell was not found.
echo Expected Windows PowerShell at:
echo   %WINPS%
pause
exit /b 1

:winps
"%WINPS%" -NoLogo -NoExit -ExecutionPolicy Bypass -File "%SS2_ROOT%scripts\ss2-shell.ps1"
exit /b %ERRORLEVEL%

:pwsh
pwsh.exe -NoLogo -NoExit -ExecutionPolicy Bypass -File "%SS2_ROOT%scripts\ss2-shell.ps1"
exit /b %ERRORLEVEL%

:powershell_path
powershell.exe -NoLogo -NoExit -ExecutionPolicy Bypass -File "%SS2_ROOT%scripts\ss2-shell.ps1"
exit /b %ERRORLEVEL%
