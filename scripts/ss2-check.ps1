param(
    [string]$BuildDir = "build\dev",
    [string]$Config = "Debug",
    [string[]]$Target = @(),
    [string[]]$Test = @(),
    [switch]$NoConfigure,
    [switch]$SelfTest
)

$ErrorActionPreference = "Stop"

function Normalize-SS2FocusedValues {
    param(
        [Parameter(Mandatory=$true)][string[]]$Value,
        [Parameter(Mandatory=$true)][string]$Kind
    )

    $normalized = @(
        $Value |
            ForEach-Object { $_.Split(",") } |
            ForEach-Object { $_.Trim() } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
            Select-Object -Unique
    )

    if ($normalized.Count -eq 0) {
        throw "FOCUSED check requires at least one $Kind."
    }

    foreach ($item in $normalized) {
        if ($item -notmatch '^[A-Za-z0-9_.-]+$') {
            throw "Invalid focused $Kind '$item'."
        }
    }

    return $normalized
}

function Get-SS2ExactTestRegex {
    param([Parameter(Mandatory=$true)][string[]]$Names)

    $escaped = @($Names | ForEach-Object { [regex]::Escape($_) })
    return "^(" + ($escaped -join "|") + ")$"
}

if ($SelfTest) {
    $targets = Normalize-SS2FocusedValues @("b,a", "a") "target"
    if (($targets -join ",") -ne "b,a") {
        throw "Focused target normalization self-test failed."
    }

    $tests = Normalize-SS2FocusedValues @("sk07f.precision_input_state,e1.arc_numerical_stability") "test"
    $regex = Get-SS2ExactTestRegex $tests
    if ($regex -ne '^(sk07f\.precision_input_state|e1\.arc_numerical_stability)$') {
        throw "Focused exact-test regex self-test failed: $regex"
    }

    $rejected = $false
    try {
        $null = Normalize-SS2FocusedValues @("bad target") "target"
    } catch {
        $rejected = $true
    }
    if (-not $rejected) {
        throw "Focused value validation self-test did not reject malformed input."
    }

    Write-Host "[check] focused dispatcher self-test passed"
    exit 0
}

$targets = Normalize-SS2FocusedValues $Target "target"
$tests = Normalize-SS2FocusedValues $Test "test"

. (Join-Path $PSScriptRoot "ss2-common.ps1")
Import-SS2LocalEnvironment
$cmake = Get-SS2CMakeExe
$root = Get-SS2Root

$configure = Join-Path $PSScriptRoot "ss2-configure.ps1"
if (-not $NoConfigure) {
    & $configure -BuildDir $BuildDir
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$build = Resolve-SS2BuildPath $BuildDir

Write-Host "[check] targets=$($targets -join ',')"
Write-Host "[check] tests=$($tests -join ',')"
Write-Host "[check] build=$build config=$Config noConfigure=$NoConfigure"

$buildArgs = @("--build", $build, "--config", $Config, "--target") + $targets
if ($env:SS2_BUILD_PARALLELISM) {
    $jobs = 0
    if (-not [int]::TryParse($env:SS2_BUILD_PARALLELISM, [ref]$jobs) -or $jobs -lt 1) {
        throw "SS2_BUILD_PARALLELISM must be a positive integer."
    }
    $buildArgs += @("--parallel", "$jobs")
}
& $cmake @buildArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$showArgs = @(
    "--test-dir", $build,
    "-C", $Config,
    "--show-only=json-v1"
)
$jsonText = (& ctest @showArgs | Out-String)
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

try {
    $catalog = $jsonText | ConvertFrom-Json
} catch {
    throw "Unable to parse CTest test catalog JSON."
}

$registered = @($catalog.tests | ForEach-Object { "$($_.name)" })
foreach ($name in $tests) {
    if ($registered -notcontains $name) {
        throw "Requested focused CTest test '$name' is not registered."
    }
}

$testRegex = Get-SS2ExactTestRegex $tests
$ctestArgs = @(
    "--test-dir", $build,
    "-C", $Config,
    "--output-on-failure",
    "--interactive-debug-mode", "0",
    "-R", $testRegex
)
if ($env:SS2_TEST_PARALLELISM) {
    $jobs = 0
    if (-not [int]::TryParse($env:SS2_TEST_PARALLELISM, [ref]$jobs) -or $jobs -lt 1) {
        throw "SS2_TEST_PARALLELISM must be a positive integer."
    }
    $ctestArgs += @("--parallel", "$jobs")
}
& ctest @ctestArgs
exit $LASTEXITCODE
