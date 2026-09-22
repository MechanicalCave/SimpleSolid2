. (Join-Path $PSScriptRoot "ss2-common.ps1")
$root = Get-SS2Root

Write-Host "=== SimpleSolid2 status ==="
Write-Host "Workspace: $root"
Write-Host ""

$git = Get-Command git -ErrorAction SilentlyContinue
if ($git) { Write-Host "[OK] git -> $($git.Source)" }
else { Write-Warning "git not found" }

$envFile = Join-Path $root ".ss2-local\env.ps1"
if (Test-Path $envFile) {
    . $envFile
    Write-Host "[OK] local env loaded"
    Write-Host " CMake      : $env:SS2_CMAKE_EXE"
    Write-Host " Qt prefix  : $env:SS2_QT_PREFIX"
    Write-Host " OCCT prefix: $env:SS2_OCCT_PREFIX"
    Write-Host " Ninja      : $env:SS2_NINJA_EXE"
} else {
    Write-Warning "Local env not configured. Run .\ss2.ps1 setup"
}

Write-Host ""
if (Test-Path (Join-Path $root ".git")) {
    Push-Location $root
    try {
        git status --short --branch
        git remote -v
    } finally {
        Pop-Location
    }
} else {
    Write-Warning "Workspace is not a Git repository yet."
}
