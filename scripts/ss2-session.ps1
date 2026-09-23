param(
    [Parameter(Mandatory = $true)]
    [string]$Root
)

$global:SS2Root = [IO.Path]::GetFullPath($Root)
Set-Location -LiteralPath $global:SS2Root

function global:ss2-status {
    & (Join-Path $global:SS2Root 'ss2.ps1') status
}

function global:ss2-resume {
    $promptPath = Join-Path $global:SS2Root 'work\RESUME_PROMPT.md'
    $clipPath = Join-Path $env:SystemRoot 'System32\clip.exe'

    if (-not (Test-Path -LiteralPath $promptPath)) {
        Write-Host "[ERROR] Resume prompt not found: $promptPath"
        return
    }

    if (-not (Test-Path -LiteralPath $clipPath)) {
        Write-Host "[WARN] clip.exe not found. Open manually: $promptPath"
        return
    }

    Get-Content -Raw -LiteralPath $promptPath | & $clipPath
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[OK] New-context prompt copied to clipboard."
        Write-Host "     Paste it into a new ChatGPT conversation."
    } else {
        Write-Host "[WARN] Could not copy resume prompt to clipboard."
    }
}

Write-Host ""
Write-Host "=== SS2 PowerShell ==="
Write-Host "Workspace : $global:SS2Root"
Write-Host "Commands  : ss2-status, ss2-resume"
Write-Host "Prompt    : already copied to clipboard by START_SS2.cmd"
Write-Host ""
