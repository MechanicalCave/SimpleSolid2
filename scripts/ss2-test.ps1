param(
    [string]$BuildDir = "build\dev",
    [string]$Config = "Debug",
    [ValidateSet("fast","subsystem","full")]
    [string]$Tier = "full",
    [string]$Subsystem = "",
    [switch]$NoBuild,
    [switch]$ListOnly,
    [switch]$SelfTest
)

$ErrorActionPreference = "Stop"

$knownSubsystems = @(
    "core",
    "application",
    "persistence",
    "part",
    "sketch",
    "viewer",
    "ui",
    "project"
)

function Get-SS2SubsystemLabelRegex {
    param([Parameter(Mandatory=$true)][string]$Value)

    $requested = @(
        $Value.Split(",") |
            ForEach-Object { $_.Trim().ToLowerInvariant() } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
            Select-Object -Unique
    )

    if ($requested.Count -eq 0) {
        throw "SUBSYSTEM tier requires -Subsystem <name>[,<name>...]."
    }

    foreach ($name in $requested) {
        if ($knownSubsystems -notcontains $name) {
            throw "Unknown SS2 test subsystem '$name'. Known values: $($knownSubsystems -join ', ')."
        }
    }

    $escaped = @(
        $requested |
            ForEach-Object { [regex]::Escape($_) }
    )
    return "^subsystem-(" + ($escaped -join "|") + ")$"
}

if ($SelfTest) {
    if ($Tier -ne "full") {
        throw "ss2-test self-test expects the dispatcher default Tier=full."
    }

    $probe = Get-SS2SubsystemLabelRegex "sketch, viewer,ui,sketch"
    if ($probe -ne "^subsystem-(sketch|viewer|ui)$") {
        throw "Subsystem selector self-test produced unexpected regex: $probe"
    }

    $rejected = $false
    try {
        $null = Get-SS2SubsystemLabelRegex "not-a-subsystem"
    } catch {
        $rejected = $true
    }
    if (-not $rejected) {
        throw "Subsystem selector self-test did not reject an unknown subsystem."
    }

    Write-Host "[test] tier dispatcher self-test passed"
    exit 0
}

. (Join-Path $PSScriptRoot "ss2-common.ps1")
Import-SS2LocalEnvironment
$root = Get-SS2Root

$build =
    if ([IO.Path]::IsPathRooted($BuildDir)) {
        [IO.Path]::GetFullPath($BuildDir)
    } else {
        [IO.Path]::GetFullPath((Join-Path $root $BuildDir))
    }

if ($NoBuild) {
    if (-not (Test-Path -LiteralPath $build -PathType Container)) {
        Write-Error "SS2 test -NoBuild requires an existing configured build directory: $build"
        exit 2
    }
} else {
    & (Join-Path $PSScriptRoot "ss2-build.ps1") -BuildDir $BuildDir -Config $Config
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$ctestArgs = @(
    "--test-dir", $build,
    "-C", $Config,
    "--output-on-failure"
)

switch ($Tier) {
    "fast" {
        $ctestArgs += @("-L", "^tier-fast$")
    }
    "subsystem" {
        $ctestArgs += @("-L", (Get-SS2SubsystemLabelRegex $Subsystem))
    }
    "full" {
        if (-not [string]::IsNullOrWhiteSpace($Subsystem)) {
            Write-Error "-Subsystem is valid only with -Tier subsystem."
            exit 2
        }
    }
}

if ($ListOnly) {
    $ctestArgs += "-N"
}

Write-Host "[test] tier=$Tier build=$build noBuild=$NoBuild listOnly=$ListOnly"
& ctest @ctestArgs
exit $LASTEXITCODE
