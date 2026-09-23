param(
    [Parameter(Mandatory = $true)]
    [string]$Root
)

$global:SS2Root = [IO.Path]::GetFullPath($Root)
$utf8 = New-Object System.Text.UTF8Encoding($false)
[Console]::InputEncoding = $utf8
[Console]::OutputEncoding = $utf8
$OutputEncoding = $utf8
Set-Location -LiteralPath $global:SS2Root

function global:ss2-status {
    & (Join-Path $global:SS2Root 'ss2.ps1') status
}

function global:ss2-resume {
    $promptPath = Join-Path $global:SS2Root 'work\RESUME_PROMPT.md'
    if (-not (Test-Path -LiteralPath $promptPath)) {
        Write-Host "[ERROR] Resume prompt not found: $promptPath"
        return
    }

    try {
        $text = [IO.File]::ReadAllText($promptPath, [Text.Encoding]::UTF8)
        Set-Clipboard -Value $text
        Write-Host "[OK] New-context prompt copied to clipboard."
        Write-Host "     Paste it into a new ChatGPT conversation."
    } catch {
        Write-Host "[WARN] Could not copy resume prompt to clipboard."
        Write-Host "       $($_.Exception.Message)"
    }
}

Write-Host ""
Write-Host "=== SS2 PowerShell ==="
Write-Host "Workspace : $global:SS2Root"
Write-Host "Commands  : ss2-status, ss2-resume"
Write-Host "Prompt    : already copied to clipboard by START_SS2.cmd"
Write-Host ""
