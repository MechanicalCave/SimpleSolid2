param(
    [string]$BuildDir = "build\dev",
    [string]$Config = "Debug",
    [ValidateSet("fast","subsystem","full")]
    [string]$Tier = "full",
    [string[]]$Subsystem = @(),
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

function Get-SS2RequestedSubsystems {
    param([Parameter(Mandatory=$true)][string[]]$Value)

    $requested = @(
        $Value |
            ForEach-Object { $_.Split(",") } |
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

    return $requested
}

function Get-SS2SubsystemLabelRegex {
    param([Parameter(Mandatory=$true)][string[]]$Value)

    $requested = @(Get-SS2RequestedSubsystems $Value)
    $escaped = @(
        $requested |
            ForEach-Object { [regex]::Escape($_) }
    )
    return "^subsystem-(" + ($escaped -join "|") + ")$"
}

function Get-SS2TierBuildTargets {
    param(
        [Parameter(Mandatory=$true)][string]$SelectedTier,
        [string[]]$SelectedSubsystem = @()
    )

    switch ($SelectedTier) {
        "fast" {
            return @("ss2_tests_fast")
        }
        "subsystem" {
            return @(
                Get-SS2RequestedSubsystems $SelectedSubsystem |
                    ForEach-Object { "ss2_tests_subsystem_$_" }
            )
        }
        "full" {
            return @("ss2_tests_full")
        }
        default {
            throw "Unknown SS2 test tier '$SelectedTier'."
        }
    }
}

if ($SelfTest) {
    if ($Tier -ne "full") {
        throw "ss2-test self-test expects the dispatcher default Tier=full."
    }

    $probe = Get-SS2SubsystemLabelRegex "sketch, viewer,ui,sketch"
    if ($probe -ne "^subsystem-(sketch|viewer|ui)$") {
        throw "Subsystem selector self-test produced unexpected regex: $probe"
    }

    $probeTargets = @(Get-SS2TierBuildTargets "subsystem" @("sketch,viewer", "ui"))
    if (($probeTargets -join ",") -ne "ss2_tests_subsystem_sketch,ss2_tests_subsystem_viewer,ss2_tests_subsystem_ui") {
        throw "Subsystem build-target self-test produced unexpected targets: $($probeTargets -join ',')"
    }

    if ((@(Get-SS2TierBuildTargets "fast") -join ",") -ne "ss2_tests_fast") {
        throw "FAST build-target self-test failed."
    }
    if ((@(Get-SS2TierBuildTargets "full") -join ",") -ne "ss2_tests_full") {
        throw "FULL build-target self-test failed."
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

$nonEmptySubsystem = @($Subsystem | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
if ($Tier -ne "subsystem" -and $nonEmptySubsystem.Count -gt 0) {
    Write-Error "-Subsystem is valid only with -Tier subsystem."
    exit 2
}

$buildTargets = @(Get-SS2TierBuildTargets $Tier $Subsystem)

. (Join-Path $PSScriptRoot "ss2-common.ps1")
Import-SS2LocalEnvironment
$root = Get-SS2Root

$build = Resolve-SS2BuildPath $BuildDir

if ($NoBuild) {
    if (-not (Test-Path -LiteralPath $build -PathType Container)) {
        Write-Error "SS2 test -NoBuild requires an existing configured build directory: $build"
        exit 2
    }
} else {
    & (Join-Path $PSScriptRoot "ss2-build.ps1") -BuildDir $BuildDir -Config $Config -Target $buildTargets
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$ctestArgs = @(
    "--test-dir", $build,
    "-C", $Config,
    "--output-on-failure",
    "--interactive-debug-mode", "0"
)

switch ($Tier) {
    "fast" {
        $ctestArgs += @("-L", "^tier-fast$")
    }
    "subsystem" {
        $ctestArgs += @("-L", (Get-SS2SubsystemLabelRegex $Subsystem))
    }
    "full" {
        # Intentionally unfiltered.
    }
}

if ($env:SS2_TEST_PARALLELISM) {
    $jobs = 0
    if (-not [int]::TryParse($env:SS2_TEST_PARALLELISM, [ref]$jobs) -or $jobs -lt 1) {
        throw "SS2_TEST_PARALLELISM must be a positive integer."
    }
    $ctestArgs += @("--parallel", "$jobs")
}

if ($ListOnly) {
    $ctestArgs += "-N"
}

Write-Host "[test] tier=$Tier build=$build buildTargets=$($buildTargets -join ',') noBuild=$NoBuild listOnly=$ListOnly"
& ctest @ctestArgs
exit $LASTEXITCODE
