param(
    [string]$RemoteUrl = "",
    [switch]$CommitGenesis
)

. (Join-Path $PSScriptRoot "ss2-common.ps1")

$git = Get-Command git -ErrorAction SilentlyContinue
if (-not $git) {
    throw "Git was not found in PATH."
}

$root = Get-SS2Root
Push-Location $root
try {
    if (-not (Test-Path ".git")) {
        & git init -b main
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }

    $branchOutput = & git branch --show-current
    $branch = "$branchOutput".Trim()

    if (-not $branch) {
        & git checkout -b main
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        $branch = "main"
    } elseif ($branch -ne "main") {
        Write-Warning "Current branch is '$branch'; expected 'main'."
    }

    if ($RemoteUrl) {
        $remotes = @(& git remote)

        if ($remotes -contains "origin") {
            $originOutput = & git remote get-url origin
            $origin = "$originOutput".Trim()

            if ($origin -ne $RemoteUrl.Trim()) {
                throw "Remote 'origin' already exists and points to '$origin'. Refusing to overwrite it automatically."
            }
        } else {
            & git remote add origin $RemoteUrl
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }
    }

    & git add .
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host ""
    Write-Host "Staged Genesis files:"
    & git status --short

    if ($CommitGenesis) {
        # show-ref --quiet returns 1 for an unborn branch without writing an error message.
        & git show-ref --verify --quiet refs/heads/main
        $hasHead = ($LASTEXITCODE -eq 0)

        if (-not $hasHead) {
            $nameOutput = & git config --get user.name
            $emailOutput = & git config --get user.email
            $name = "$nameOutput".Trim()
            $email = "$emailOutput".Trim()

            if (-not $name -or -not $email) {
                throw "Git author identity is not configured. Run: git config --global user.name `"Your Name`" and git config --global user.email `"you@example.com`", then rerun."
            }

            & git commit -m "Genesis: freeze SS2 Foundation v1.0"
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        } else {
            Write-Host "Repository already has history. Genesis commit was not created automatically."
        }
    }

    Write-Host ""
    Write-Host "Git ready."
    & git status --short --branch
    & git remote -v
} finally {
    Pop-Location
}
