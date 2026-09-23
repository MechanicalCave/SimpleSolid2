@echo off
setlocal

set "SS2_ROOT=%~dp0"
set "SS2_PROMPT=%SS2_ROOT%work\RESUME_PROMPT.md"
set "POWERSHELL=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
set "CLIP=%SystemRoot%\System32\clip.exe"

cd /d "%SS2_ROOT%"
title SimpleSolid 2.0

echo.
echo === SimpleSolid 2.0 ===
echo Workspace: %SS2_ROOT%
echo.

if exist "%POWERSHELL%" (
    "%POWERSHELL%" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%SS2_ROOT%ss2.ps1" status
) else (
    echo [WARN] Windows PowerShell not found at:
    echo        %POWERSHELL%
)

echo.
if not exist "%SS2_PROMPT%" (
    echo [ERROR] Resume prompt not found:
    echo         %SS2_PROMPT%
    echo.
    pause
    exit /b 1
)

if exist "%CLIP%" (
    type "%SS2_PROMPT%" | "%CLIP%"
    if errorlevel 1 (
        echo [WARN] Could not copy resume prompt to clipboard.
    ) else (
        echo [OK] New-context prompt copied to clipboard.
    )
) else (
    echo [WARN] clip.exe not found. Open manually:
    echo        %SS2_PROMPT%
)

echo.
echo Paste the prompt into a new ChatGPT conversation.
echo A new SS2 PowerShell will open in:
echo   %SS2_ROOT%
echo.

if exist "%POWERSHELL%" (
    start "SS2 Shell" "%POWERSHELL%" -NoLogo -NoExit -NoProfile -ExecutionPolicy Bypass -Command "Set-Location -LiteralPath '%SS2_ROOT%'"
)

endlocal
exit /b 0
