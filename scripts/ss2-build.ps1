param(
    [string]$BuildDir = "build\dev",
    [string]$Config = "Debug",
    [string[]]$Target = @(),
    [switch]$All
)

$ErrorActionPreference = "Stop"

$targets = @(
    $Target |
        ForEach-Object { $_.Split(",") } |
        ForEach-Object { $_.Trim() } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        Select-Object -Unique
)

foreach ($item in $targets) {
    if ($item -notmatch '^[A-Za-z0-9_.-]+$') {
        throw "Invalid SS2 build target '$item'."
    }
}

if ($All -and $targets.Count -gt 0) {
    throw "SS2 build cannot combine -All with explicit -Target values."
}

if (-not $All -and $targets.Count -eq 0) {
    $targets = @("simplesolid2")
}

$configure = Join-Path $PSScriptRoot "ss2-configure.ps1"
& $configure -BuildDir $BuildDir
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

. (Join-Path $PSScriptRoot "ss2-common.ps1")
Import-SS2LocalEnvironment
$cmake = Get-SS2CMakeExe
$root = Get-SS2Root
$build = Join-Path $root $BuildDir

$buildArgs = @("--build", $build, "--config", $Config)
if (-not $All) {
    $buildArgs += @("--target") + $targets
}

$mode = if ($All) { "ALL" } else { $targets -join "," }
Write-Host "[build] build=$build config=$Config targets=$mode"
& $cmake @buildArgs
exit $LASTEXITCODE
