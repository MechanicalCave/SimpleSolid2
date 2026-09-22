. (Join-Path $PSScriptRoot "ss2-common.ps1")
$root = Get-SS2Root

$fail = $false
Write-Host "=== SS2 bootstrap verification ==="

foreach ($f in @(
    "README.md",
    "AGENTS.md",
    "governance\FOUNDATION.md",
    "governance\CONSTITUTION.md",
    "governance\ARCHITECTURE.md",
    "work\ACTIVE.yaml",
    "CMakeLists.txt"
)) {
    $p = Join-Path $root $f
    if (Test-Path $p) { Write-Host "[OK] $f" }
    else { Write-Error "[MISSING] $f"; $fail = $true }
}

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Write-Error "Git not found."
    $fail = $true
} else {
    Write-Host "[OK] Git available"
}

$envFile = Join-Path $root ".ss2-local\env.ps1"
if (Test-Path $envFile) {
    . $envFile
    Write-Host "[OK] Local environment configured"
    if ($env:SS2_CMAKE_EXE -and (Test-Path $env:SS2_CMAKE_EXE)) {
        Write-Host "[OK] CMake: $env:SS2_CMAKE_EXE"
    } else {
        Write-Error "Configured CMake executable is missing."
        $fail = $true
    }
    if ($env:SS2_QT_PREFIX) { Write-Host "[OK] Qt prefix detected: $env:SS2_QT_PREFIX" }
    else { Write-Warning "Qt prefix not detected" }
    if ($env:SS2_OCCT_PREFIX) { Write-Host "[OK] OCCT prefix detected: $env:SS2_OCCT_PREFIX" }
    else { Write-Host "[INFO] OCCT prefix not detected (not a Genesis blocker)" }
} else {
    Write-Error "Local environment not configured. Run .\ss2.ps1 setup"
    $fail = $true
}

if (-not $fail) {
    Write-Host ""
    Write-Host "Running minimal CMake configure..."
    & (Join-Path $PSScriptRoot "ss2-configure.ps1")
    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake configure failed."
        $fail = $true
    } else {
        Write-Host "[OK] CMake configure passed"
    }
}

if ($fail) {
    Write-Host ""
    Write-Error "SS2 bootstrap verification FAILED."
    exit 1
}

Write-Host ""
Write-Host "SS2 bootstrap verification PASSED."
exit 0
