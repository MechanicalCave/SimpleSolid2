param(
    [string]$BuildDir = ""
)

$ErrorActionPreference = "Stop"

$root = [IO.Path]::GetFullPath(
    (Join-Path $PSScriptRoot "..\.."))
$activePath = Join-Path $root "work\ACTIVE.yaml"
$active = Get-Content -Raw -LiteralPath $activePath

if ($active -notmatch '(?m)^status:\s*active\s*$' -or
    $active -notmatch '(?m)^active_work:\s*["'']?work/SR-02_SKETCH_INTERACTION_PRESENTATION_LATENCY\.md["'']?\s*$') {
    Write-Host "[sr02-benchmark] skipped; SR-02 is not the active Work Contract"
    exit 0
}

. (Join-Path $root "scripts\ss2-common.ps1")
Import-SS2LocalEnvironment

$cmake = Get-SS2CMakeExe
$sha = (git -C $root rev-parse HEAD).Trim()
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $short = $sha.Substring(0, 12)
    $BuildDir = Join-Path $env:SS2_CI_BUILD_ROOT "sr02-latency-$short-release"
}
$build = [IO.Path]::GetFullPath($BuildDir)

if (Test-Path -LiteralPath $build) {
    Remove-Item -LiteralPath $build -Recurse -Force
}

Write-Host "[sr02-benchmark] exact_sha=$sha"
Write-Host "[sr02-benchmark] machine=$env:COMPUTERNAME"
try {
    $cpu = Get-CimInstance Win32_Processor |
        Select-Object -First 1 -ExpandProperty Name
    Write-Host "[sr02-benchmark] cpu=$cpu"
} catch {
    Write-Host "[sr02-benchmark] cpu=<unavailable>"
}
try {
    $os = Get-CimInstance Win32_OperatingSystem
    Write-Host "[sr02-benchmark] os=$($os.Caption) $($os.Version)"
    Write-Host "[sr02-benchmark] ram_gib=$([Math]::Round($os.TotalVisibleMemorySize / 1MB, 2))"
} catch {
    Write-Host "[sr02-benchmark] os=<unavailable>"
}

$args = @(
    "-S", $root,
    "-B", $build,
    "-DSS2_BUILD_DESKTOP=ON",
    "-DSS2_ENABLE_SR02_LATENCY_BENCHMARK=ON"
)
if ($env:CMAKE_PREFIX_PATH) {
    $args += "-DCMAKE_PREFIX_PATH=$env:CMAKE_PREFIX_PATH"
}

Write-Host "[sr02-benchmark] configure"
& $cmake @args
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "[sr02-benchmark] build Release"
& $cmake --build $build --config Release --target sr02_interaction_latency_benchmark
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$exe = Get-ChildItem -LiteralPath $build -Recurse -File -Filter "sr02_interaction_latency_benchmark.exe" |
    Select-Object -First 1
if ($null -eq $exe) {
    throw "SR-02 latency benchmark executable was not produced."
}

Write-Host "[sr02-benchmark] run=$($exe.FullName)"
& $exe.FullName
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "[sr02-benchmark] PASS"
exit 0
