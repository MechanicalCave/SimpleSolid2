@echo off
setlocal

set "SS2_ROOT=%~dp0"
set "SS2_PROMPT=%SS2_ROOT%work\RESUME_PROMPT.md"
set "POWERSHELL=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
set "CLIP=%SystemRoot%\System32\clip.exe"
if not defined SS2_RUNNER_ROOT set "SS2_RUNNER_ROOT=D:\runner-ss2"

cd /d "%SS2_ROOT%"
title SimpleSolid 2.0

echo.
echo === SimpleSolid 2.0 ===
echo Workspace: %SS2_ROOT%
echo.

call :ensure_runner

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

if exist "%POWERSHELL%" (
    "%POWERSHELL%" -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command "$text=[IO.File]::ReadAllText($env:SS2_PROMPT,[Text.Encoding]::UTF8); Set-Clipboard -Value $text"
    if errorlevel 1 (
        echo [WARN] Could not copy resume prompt to clipboard.
    ) else (
        echo [OK] New-context prompt copied to clipboard.
    )
) else (
    echo [WARN] Windows PowerShell not found. Open manually:
    echo        %SS2_PROMPT%
)

echo.
echo Paste the prompt into a new ChatGPT conversation.
echo A new SS2 PowerShell will open in:
echo   %SS2_ROOT%
echo.

if exist "%POWERSHELL%" (
    start "SS2 Shell" "%POWERSHELL%" -NoLogo -NoExit -NoProfile -ExecutionPolicy Bypass -Command ". '%SS2_ROOT%scripts\ss2-session.ps1' -Root '%SS2_ROOT%'"
)

endlocal
exit /b 0

:ensure_runner
if not exist "%SS2_RUNNER_ROOT%\run.cmd" (
    echo [WARN] self-hosted runner not found:
    echo        %SS2_RUNNER_ROOT%\run.cmd
    echo        Set SS2_RUNNER_ROOT to override the default runner location.
    exit /b 0
)

if not exist "%POWERSHELL%" (
    echo [WARN] cannot detect runner state because Windows PowerShell is unavailable.
    exit /b 0
)

call :runner_is_running
if not errorlevel 1 (
    echo [OK] self-hosted runner already running: %SS2_RUNNER_ROOT%
    exit /b 0
)

start "SS2 Runner" /min /D "%SS2_RUNNER_ROOT%" "%ComSpec%" /d /c "call run.cmd"

if exist "%SystemRoot%\System32\timeout.exe" (
    "%SystemRoot%\System32\timeout.exe" /t 2 /nobreak >nul
)

call :runner_is_running
if errorlevel 1 (
    echo [WARN] self-hosted runner start requested, but listener is not detected yet.
    echo        If CI jobs remain queued, check: %SS2_RUNNER_ROOT%
) else (
    echo [OK] self-hosted runner started: %SS2_RUNNER_ROOT%
)
exit /b 0

:runner_is_running
"%POWERSHELL%" -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command "$expected=[IO.Path]::GetFullPath((Join-Path $env:SS2_RUNNER_ROOT 'bin\Runner.Listener.exe')); foreach($p in @(Get-Process -Name 'Runner.Listener' -ErrorAction SilentlyContinue)){ if($p.Path -and [string]::Equals([IO.Path]::GetFullPath($p.Path),$expected,[StringComparison]::OrdinalIgnoreCase)){ exit 0 } }; exit 1" >nul 2>&1
exit /b %ERRORLEVEL%
