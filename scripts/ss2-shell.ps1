param(
    [switch]$NoBanner
)

$ErrorActionPreference = "Continue"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$global:SS2_ROOT = $root
Set-Location $root

$envFile = Join-Path $root ".ss2-local\env.ps1"
if (Test-Path $envFile) {
    . $envFile
    $envReady = $true
} else {
    $envReady = $false
}

function global:ss2 {
    & (Join-Path $global:SS2_ROOT "ss2.ps1") @args
}

function global:ss2-state {
    Set-Location $global:SS2_ROOT

    Write-Host "=== SS2 local state ==="
    Write-Host "Workspace : $global:SS2_ROOT"

    if (Test-Path (Join-Path $global:SS2_ROOT ".git")) {
        $branch = (& git branch --show-current 2>$null | Out-String).Trim()
        $sha = (& git rev-parse --short HEAD 2>$null | Out-String).Trim()
        $subject = (& git log -1 --pretty=format:"%s" 2>$null | Out-String).Trim()

        Write-Host "Branch    : $branch"
        Write-Host "HEAD      : $sha $subject"
        & git status --short --branch
    } else {
        Write-Warning "Not a Git repository."
    }

    if ($env:SS2_CMAKE_EXE) { Write-Host "CMake     : $env:SS2_CMAKE_EXE" }
    if ($env:SS2_QT_PREFIX) { Write-Host "Qt        : $env:SS2_QT_PREFIX" }
    if ($env:SS2_OCCT_PREFIX) {
        Write-Host "OCCT      : $env:SS2_OCCT_PREFIX"
    } else {
        Write-Host "OCCT      : not configured / not required for Genesis"
    }
}

function global:ss2-resume {
    Set-Location $global:SS2_ROOT

    $promptPath = Join-Path $global:SS2_ROOT "work\RESUME_PROMPT.md"
    if (-not (Test-Path $promptPath)) {
        Write-Error "Missing $promptPath"
        return
    }

    $basePrompt = Get-Content -Path $promptPath -Raw -Encoding UTF8

    $branch = ""
    $sha = ""
    $head = ""
    $remote = ""
    $status = ""

    if (Test-Path (Join-Path $global:SS2_ROOT ".git")) {
        $branch = (& git branch --show-current 2>$null | Out-String).Trim()
        $sha = (& git rev-parse --short HEAD 2>$null | Out-String).Trim()
        $head = (& git log -1 --pretty=format:"%h %s" 2>$null | Out-String).Trim()

        $remotes = @(& git remote 2>$null)
        if ($remotes -contains "origin") {
            $remote = (& git remote get-url origin 2>$null | Out-String).Trim()
        }

        $status = (& git status --short --branch 2>&1 | Out-String).TrimEnd()
    }

    $localState = @"

---

## LOCAL STATE APPENDED BY ss2-resume

Workspace: $global:SS2_ROOT
Origin: $remote
Branch: $branch
HEAD: $head
HEAD short SHA: $sha

git status:
```text
$status
```

This local-state block is a handoff hint only. Re-check the authoritative repository/PR state before making persistent changes.
"@

    $resumeText = $basePrompt.TrimEnd() + "`r`n" + $localState

    $copied = $false
    try {
        Set-Clipboard -Value $resumeText -ErrorAction Stop
        $copied = $true
    } catch {
        $clip = Get-Command clip.exe -ErrorAction SilentlyContinue
        if ($clip) {
            $resumeText | clip.exe
            $copied = $true
        }
    }

    if ($copied) {
        Write-Host "[OK] SS2 resume prompt + current local Git state copied to clipboard."
        Write-Host "Paste it as the first message in the new ChatGPT conversation."
    } else {
        Write-Warning "Clipboard helper unavailable. Printing prompt below:"
        Write-Output $resumeText
    }
}

function global:prompt {
    $branch = ""
    if (Test-Path (Join-Path $global:SS2_ROOT ".git")) {
        $branch = (& git -C $global:SS2_ROOT branch --show-current 2>$null | Out-String).Trim()
    }

    $where = $executionContext.SessionState.Path.CurrentLocation
    if ($branch) {
        return "[SS2:$branch] PS $where> "
    }

    return "[SS2] PS $where> "
}

if (-not $NoBanner) {
    Write-Host ""
    Write-Host "SimpleSolid 2 Developer Shell"
    Write-Host "============================="
    Write-Host "Workspace : $root"

    if ($envReady) {
        Write-Host "[OK] machine-local SS2 environment loaded"
        if ($env:SS2_CMAKE_EXE) { Write-Host "CMake     : $env:SS2_CMAKE_EXE" }
        if ($env:SS2_QT_PREFIX) { Write-Host "Qt        : $env:SS2_QT_PREFIX" }
        if ($env:SS2_OCCT_PREFIX) {
            Write-Host "OCCT      : $env:SS2_OCCT_PREFIX"
        } else {
            Write-Host "OCCT      : not configured"
        }
    } else {
        Write-Warning ".ss2-local\env.ps1 not found. Run: ss2 setup"
    }

    Write-Host ""
    if (Test-Path (Join-Path $root ".git")) {
        & git status --short --branch
        $head = (& git log -1 --pretty=format:"%h %s" 2>$null | Out-String).Trim()
        if ($head) { Write-Host "HEAD: $head" }
    }

    Write-Host ""
    Write-Host "Commands:"
    Write-Host "  ss2 status|verify|build|test   project tooling"
    Write-Host "  ss2-state                      concise local state"
    Write-Host "  ss2-resume                     copy continuation prompt + Git state"
    Write-Host ""
}
