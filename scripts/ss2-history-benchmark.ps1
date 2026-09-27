param(
    [string]$BuildDir = "build\c2-history-benchmark",
    [string]$Config = "Release",
    [string]$OutputDir = "artifacts\c2-history-benchmark",
    [int]$Samples = 20,
    [int]$Warmup = 3,
    [int]$MaxWorkingSetMB = 4096,
    [int]$CellTimeoutMinutes = 5
)

$ErrorActionPreference = "Stop"

if ($Config -ne "Release") { throw "C2 canonical evidence requires -Config Release." }
if ($Samples -lt 1) { throw "-Samples must be at least 1." }
if ($Warmup -lt 0) { throw "-Warmup cannot be negative." }
if ($MaxWorkingSetMB -lt 512) { throw "-MaxWorkingSetMB must be at least 512." }
if ($CellTimeoutMinutes -lt 1) { throw "-CellTimeoutMinutes must be at least 1." }

. (Join-Path $PSScriptRoot "ss2-common.ps1")
Import-SS2LocalEnvironment

$root = Get-SS2Root
$cmake = Get-SS2CMakeExe
$build = if ([IO.Path]::IsPathRooted($BuildDir)) { [IO.Path]::GetFullPath($BuildDir) } else { [IO.Path]::GetFullPath((Join-Path $root $BuildDir)) }
$output = if ([IO.Path]::IsPathRooted($OutputDir)) { [IO.Path]::GetFullPath($OutputDir) } else { [IO.Path]::GetFullPath((Join-Path $root $OutputDir)) }

New-Item -ItemType Directory -Force -Path $output | Out-Null

$sha = (git -C $root rev-parse HEAD).Trim()
if ([string]::IsNullOrWhiteSpace($sha)) { throw "Unable to resolve benchmark Git SHA." }

Write-Host "[c2] configure Release benchmark: $sha"
$configureArgs = @(
    "-S", $root,
    "-B", $build,
    "-DSS2_ENABLE_C2_HISTORY_BENCHMARK=ON"
)
if ($env:CMAKE_PREFIX_PATH) {
    $configureArgs += "-DCMAKE_PREFIX_PATH=$env:CMAKE_PREFIX_PATH"
}
& $cmake @configureArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $cmake --build $build --config $Config --target c2_history_benchmark
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$candidates = @(
    (Join-Path $build "benchmarks\$Config\c2_history_benchmark.exe"),
    (Join-Path $build "benchmarks\c2_history_benchmark.exe")
)
$exe = $candidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
if (-not $exe) { throw "Unable to locate c2_history_benchmark.exe under $build." }

Write-Host "[c2] benchmark self-test"
& $exe --self-test
if ($LASTEXITCODE -ne 0) { throw "C2 benchmark self-test failed." }

Write-Host "[c2] benchmark smoke"
& $exe --entities 100 --depth 5 --scenario all --samples 2 --warmup 1 --git-sha $sha
if ($LASTEXITCODE -ne 0) { throw "C2 benchmark smoke failed." }

Write-Host "[c2] benchmark invalid-argument rejection"
& $exe --entities 0 --depth 5 --scenario add --samples 1 --warmup 0 --git-sha $sha
if ($LASTEXITCODE -eq 0) { throw "C2 benchmark accepted invalid --entities 0." }

$os = $null
$cpu = $null
$computer = $null
try { $os = Get-CimInstance Win32_OperatingSystem } catch {}
try { $cpu = Get-CimInstance Win32_Processor | Select-Object -First 1 } catch {}
try { $computer = Get-CimInstance Win32_ComputerSystem } catch {}

$requestedMaxBytes = [int64]$MaxWorkingSetMB * 1MB
$physicalMemoryBytes = if ($computer) { [uint64]$computer.TotalPhysicalMemory } else { 0 }
$quarterPhysicalBytes =
    if ($physicalMemoryBytes -gt 0) {
        [int64]([double]$physicalMemoryBytes * 0.25)
    } else {
        $requestedMaxBytes
    }
$fourGiBBytes = [int64]4096 * 1MB
$maxBytes = [Math]::Min(
    $requestedMaxBytes,
    [Math]::Min($quarterPhysicalBytes, $fourGiBBytes))
$effectiveMaxWorkingSetMB = [Math]::Max(512, [Math]::Floor([double]$maxBytes / 1MB))
$maxBytes = [int64]$effectiveMaxWorkingSetMB * 1MB

$metadata = [ordered]@{
    git_sha = $sha
    configuration = $Config
    runner_class = "self-hosted-windows-x64-simplesolid2-native"
    os = if ($os) { "$($os.Caption) $($os.Version) build $($os.BuildNumber)" } else { "unknown" }
    cpu = if ($cpu) { $cpu.Name.Trim() } else { "unknown" }
    logical_processors = if ($computer) { [int]$computer.NumberOfLogicalProcessors } else { 0 }
    physical_memory_bytes = $physicalMemoryBytes
    samples = $Samples
    warmup = $Warmup
    requested_max_working_set_mb = $MaxWorkingSetMB
    effective_max_working_set_mb = $effectiveMaxWorkingSetMB
    cell_timeout_minutes = $CellTimeoutMinutes
    generated_utc = [DateTime]::UtcNow.ToString("o")
}

$entitiesValues = @(1000, 10000)
$depthValues = @(10, 100, 1000)
$cells = @()

foreach ($entities in $entitiesValues) {
    foreach ($depth in $depthValues) {
        $stem = "e$entities-h$depth"
        $cellFile = Join-Path $output "$stem.json"
        $stdoutFile = Join-Path $output "$stem.stdout.txt"
        $stderrFile = Join-Path $output "$stem.stderr.txt"

        Remove-Item -Force -ErrorAction SilentlyContinue $cellFile, $stdoutFile, $stderrFile

        $arguments = @(
            "--entities", "$entities",
            "--depth", "$depth",
            "--scenario", "all",
            "--samples", "$Samples",
            "--warmup", "$Warmup",
            "--output", ('"' + $cellFile + '"'),
            "--git-sha", $sha
        )

        Write-Host "[c2] run entities=$entities history=$depth safety=$effectiveMaxWorkingSetMB MB timeout=$CellTimeoutMinutes min"
        $process = Start-Process -FilePath $exe -ArgumentList $arguments -PassThru -NoNewWindow -RedirectStandardOutput $stdoutFile -RedirectStandardError $stderrFile

        $watch = [Diagnostics.Stopwatch]::StartNew()
        $observedPeak = [int64]0
        $cutoffReason = $null

        while (-not $process.WaitForExit(250)) {
            try {
                $process.Refresh()
                $working = [int64]$process.WorkingSet64
                if ($working -gt $observedPeak) { $observedPeak = $working }
                if ($working -gt $maxBytes) {
                    $cutoffReason = "benchmark_working_set_safety_ceiling"
                    break
                }
            } catch {}

            if ($watch.Elapsed.TotalMinutes -gt $CellTimeoutMinutes) {
                $cutoffReason = "benchmark_cell_timeout"
                break
            }
        }

        if ($cutoffReason) {
            try { if (-not $process.HasExited) { $process.Kill() } } catch {}
            try { $process.WaitForExit() } catch {}

            $cell = [ordered]@{
                status = "cutoff"
                git_sha = $sha
                entities = $entities
                history_depth = $depth
                samples_requested = $Samples
                warmup_samples = $Warmup
                cutoff_reason = $cutoffReason
                observed_working_set_bytes = $observedPeak
                elapsed_seconds = [Math]::Round($watch.Elapsed.TotalSeconds, 3)
                results = @()
            }
            $cell | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $cellFile -Encoding UTF8
            $cells += [pscustomobject]$cell
            Write-Warning "[c2] cutoff entities=$entities history=$depth reason=$cutoffReason"
            continue
        }

        $process.WaitForExit()
        if (-not (Test-Path -LiteralPath $cellFile -PathType Leaf)) {
            $stderr = if (Test-Path $stderrFile) { Get-Content -Raw -LiteralPath $stderrFile } else { "" }
            $stdout = if (Test-Path $stdoutFile) { Get-Content -Raw -LiteralPath $stdoutFile } else { "" }
            throw "C2 benchmark process ended without completed cell output for entities=$entities history=$depth. STDOUT: $stdout STDERR: $stderr"
        }

        try {
            $cell = Get-Content -Raw -LiteralPath $cellFile | ConvertFrom-Json
        } catch {
            throw "C2 benchmark produced invalid cell JSON for entities=$entities history=$depth: $($_.Exception.Message)"
        }
        if ($cell.status -ne "completed") { throw "C2 benchmark returned unexpected status '$($cell.status)' for entities=$entities history=$depth." }
        if ($cell.git_sha -ne $sha) { throw "C2 benchmark SHA mismatch for entities=$entities history=$depth." }
        if ([int]$cell.entities -ne $entities -or [int]$cell.history_depth -ne $depth) {
            throw "C2 benchmark cell coordinates do not match request for entities=$entities history=$depth."
        }
        if ([int]$cell.samples_requested -ne $Samples) {
            throw "C2 benchmark sample-count mismatch for entities=$entities history=$depth."
        }
        $cells += $cell

        Write-Host "[c2] completed entities=$entities history=$depth peak=$([Math]::Round([double]$cell.peak_working_set_bytes / 1MB, 1)) MB"
    }
}

$aggregate = [ordered]@{
    schema = "ss2-c2-history-benchmark-v1"
    metadata = $metadata
    cells = $cells
}
$aggregatePath = Join-Path $output "matrix.json"
$aggregate | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $aggregatePath -Encoding UTF8

$rows = foreach ($cell in $cells) {
    if ($cell.status -eq "cutoff") {
        [pscustomobject]@{
            entities = $cell.entities
            history_depth = $cell.history_depth
            scenario = "cell"
            status = "cutoff"
            samples_completed = 0
            median_us = $null
            p95_us = $null
            max_us = $null
            peak_working_set_bytes = $cell.observed_working_set_bytes
            post_setup_working_set_bytes = $null
            setup_ms = $null
            cutoff_reason = $cell.cutoff_reason
        }
        continue
    }

    foreach ($result in $cell.results) {
        [pscustomobject]@{
            entities = $cell.entities
            history_depth = $cell.history_depth
            scenario = $result.scenario
            status = "completed"
            samples_completed = $result.samples_completed
            median_us = $result.median_us
            p95_us = $result.p95_us
            max_us = $result.max_us
            peak_working_set_bytes = $cell.peak_working_set_bytes
            post_setup_working_set_bytes = $cell.post_setup_working_set_bytes
            setup_ms = $cell.setup_ms
            cutoff_reason = ""
        }
    }
}

$csvPath = Join-Path $output "matrix.csv"
$rows | Export-Csv -NoTypeInformation -Encoding UTF8 -LiteralPath $csvPath

Write-Host "[c2] evidence: $aggregatePath"
Write-Host "[c2] summary: $csvPath"
