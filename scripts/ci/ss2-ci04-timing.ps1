param(
    [Parameter(Mandatory=$true)][string]$BaselineRef,
    [string]$EvidenceRoot = "",
    [string]$QtRoot = "D:\Qt",
    [string]$OcctRoot = "D:\SimpleSolid2\.simplesolid-env"
)

$ErrorActionPreference = "Stop"

. (Join-Path (Split-Path $PSScriptRoot -Parent) "ss2-common.ps1")
Import-SS2LocalEnvironment

$root = [IO.Path]::GetFullPath((Get-SS2Root))
if ([string]::IsNullOrWhiteSpace($EvidenceRoot)) {
    $EvidenceRoot = Join-Path ([IO.Path]::GetTempPath()) "ss2-ci04-evidence"
}
$EvidenceRoot = [IO.Path]::GetFullPath($EvidenceRoot)


function Invoke-SS2GitCommand {
    param(
        [Parameter(Mandatory=$true)][string[]]$ArgumentList,
        [switch]$AllowFailure
    )

    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        & git @ArgumentList 2>&1 | ForEach-Object { Write-Host "[git] $_" }
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousPreference
    }

    if (-not $AllowFailure -and $code -ne 0) {
        throw "Git command failed with exit code $code: git $($ArgumentList -join ' ')"
    }
    return $code
}

function Remove-SS2EvidencePath {
    param([Parameter(Mandatory=$true)][string]$Path)
    if (Test-Path -LiteralPath $Path) {
        Remove-Item -LiteralPath $Path -Recurse -Force
    }
}

function Invoke-SS2Timed {
    param(
        [Parameter(Mandatory=$true)][string]$Label,
        [Parameter(Mandatory=$true)][scriptblock]$Action
    )

    $sw = [Diagnostics.Stopwatch]::StartNew()
    & $Action
    $code = $LASTEXITCODE
    $sw.Stop()

    if ($null -ne $code -and $code -ne 0) {
        throw "CI-04 timing scenario '$Label' failed with exit code $code."
    }

    $seconds = [Math]::Round($sw.Elapsed.TotalSeconds, 2)
    Write-Host "[timing] $Label=$seconds s"
    return $seconds
}

function Touch-SS2SourceAndMeasure {
    param(
        [Parameter(Mandatory=$true)][string]$SourcePath,
        [Parameter(Mandatory=$true)][scriptblock]$BuildAction,
        [Parameter(Mandatory=$true)][string]$Label
    )

    $item = Get-Item -LiteralPath $SourcePath
    $original = $item.LastWriteTimeUtc
    try {
        [IO.File]::SetLastWriteTimeUtc($SourcePath, [DateTime]::UtcNow.AddSeconds(2))
        return Invoke-SS2Timed $Label $BuildAction
    } finally {
        [IO.File]::SetLastWriteTimeUtc($SourcePath, $original)
    }
}

function Invoke-SS2SetupAt {
    param([Parameter(Mandatory=$true)][string]$SourceRoot)
    & (Join-Path $SourceRoot "ss2.ps1") setup -QtRoot $QtRoot -OcctRoot $OcctRoot
    if ($LASTEXITCODE -ne 0) {
        throw "CI-04 evidence setup failed for $SourceRoot"
    }
}

New-Item -ItemType Directory -Force -Path $EvidenceRoot | Out-Null
$baselineSource = Join-Path $EvidenceRoot "baseline-src"

Write-Host "[timing] fetching baseline ref $BaselineRef"
$null = Invoke-SS2GitCommand @("-C", $root, "fetch", "--no-tags", "--depth=1", "origin", $BaselineRef)

$null = Invoke-SS2GitCommand @("-C", $root, "worktree", "prune")
if (Test-Path -LiteralPath $baselineSource) {
    $removeCode = Invoke-SS2GitCommand @("-C", $root, "worktree", "remove", "--force", $baselineSource) -AllowFailure
    if ($removeCode -ne 0) {
        Remove-SS2EvidencePath $baselineSource
        $null = Invoke-SS2GitCommand @("-C", $root, "worktree", "prune")
    }
}

$null = Invoke-SS2GitCommand @("-C", $root, "worktree", "add", "--detach", $baselineSource, $BaselineRef)

$results = [ordered]@{
    baseline_ref = $BaselineRef
    current_ref = (& git -C $root rev-parse HEAD).Trim()
    machine = "$env:COMPUTERNAME"
    representative_source = "src/sketch/interaction_state.cpp"
}

try {
    Invoke-SS2SetupAt $baselineSource

    $baselineProduct = Join-Path $baselineSource "build\ci04-product"
    $baselineFocused = Join-Path $baselineSource "build\ci04-focused"
    $baselineFast = Join-Path $baselineSource "build\ci04-fast"
    Remove-SS2EvidencePath $baselineProduct
    Remove-SS2EvidencePath $baselineFocused
    Remove-SS2EvidencePath $baselineFast

    Push-Location $baselineSource
    try {
        $results["before_product_clean_s"] = Invoke-SS2Timed "before.product.clean" {
            & .\scripts\ss2-build.ps1 -BuildDir "build\ci04-product" -Config Debug
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }
        $results["before_product_noop_s"] = Invoke-SS2Timed "before.product.noop" {
            & .\scripts\ss2-build.ps1 -BuildDir "build\ci04-product" -Config Debug
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }
        $results["before_product_one_cpp_s"] = Touch-SS2SourceAndMeasure -SourcePath (Join-Path $baselineSource "src\sketch\interaction_state.cpp") -Label "before.product.one_cpp" -BuildAction {
            & .\scripts\ss2-build.ps1 -BuildDir "build\ci04-product" -Config Debug
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }

        $results["before_focused_clean_s"] = Invoke-SS2Timed "before.focused.clean" {
            & .\scripts\ss2-check.ps1 -BuildDir "build\ci04-focused" -Config Debug -Target e1_arc_numerical_stability_test -Test e1.arc_numerical_stability
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }

        $results["before_fast_clean_s"] = Invoke-SS2Timed "before.fast.clean" {
            & .\scripts\ss2-test.ps1 -BuildDir "build\ci04-fast" -Config Debug -Tier fast
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }
    } finally {
        Pop-Location
    }

    $currentProduct = Join-Path $EvidenceRoot "current-product"
    $currentFocused = Join-Path $EvidenceRoot "current-focused"
    $currentFast = Join-Path $EvidenceRoot "current-fast"
    Remove-SS2EvidencePath $currentProduct
    Remove-SS2EvidencePath $currentFocused
    Remove-SS2EvidencePath $currentFast

    Push-Location $root
    try {
        $results["after_product_clean_s"] = Invoke-SS2Timed "after.product.clean" {
            & .\scripts\ss2-build.ps1 -BuildDir $currentProduct -Config Debug
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }
        $results["after_product_noop_s"] = Invoke-SS2Timed "after.product.noop" {
            & .\scripts\ss2-build.ps1 -BuildDir $currentProduct -Config Debug
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }
        $results["after_product_one_cpp_s"] = Touch-SS2SourceAndMeasure -SourcePath (Join-Path $root "src\sketch\interaction_state.cpp") -Label "after.product.one_cpp" -BuildAction {
            & .\scripts\ss2-build.ps1 -BuildDir $currentProduct -Config Debug
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }

        $results["after_focused_clean_s"] = Invoke-SS2Timed "after.focused.clean" {
            & .\scripts\ss2-check.ps1 -BuildDir $currentFocused -Config Debug -Target e1_arc_numerical_stability_test -Test e1.arc_numerical_stability
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }

        $results["after_fast_clean_s"] = Invoke-SS2Timed "after.fast.clean" {
            & .\scripts\ss2-test.ps1 -BuildDir $currentFast -Config Debug -Tier fast
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }
    } finally {
        Pop-Location
    }

    foreach ($name in @("product_clean", "product_noop", "product_one_cpp", "focused_clean", "fast_clean")) {
        $beforeKey = "before_" + $name + "_s"
        $afterKey = "after_" + $name + "_s"
        $speedupKey = $name + "_speedup_x"
        $before = [double]$results[$beforeKey]
        $after = [double]$results[$afterKey]
        if ($after -gt 0) {
            $results[$speedupKey] = [Math]::Round($before / $after, 2)
        }
    }

    Write-Host "[timing] result-json"
    $json = $results | ConvertTo-Json -Depth 4
    Write-Host $json

    if ($env:GITHUB_STEP_SUMMARY) {
        Add-Content -LiteralPath $env:GITHUB_STEP_SUMMARY -Value "## CI-04 timing evidence"
        Add-Content -LiteralPath $env:GITHUB_STEP_SUMMARY -Value ""
        Add-Content -LiteralPath $env:GITHUB_STEP_SUMMARY -Value "| Scenario | Before (s) | After (s) | Speedup |"
        Add-Content -LiteralPath $env:GITHUB_STEP_SUMMARY -Value "|---|---:|---:|---:|"
        foreach ($name in @("product_clean", "product_noop", "product_one_cpp", "focused_clean", "fast_clean")) {
            $beforeKey = "before_" + $name + "_s"
            $afterKey = "after_" + $name + "_s"
            $speedupKey = $name + "_speedup_x"
            $line = "| " + $name + " | " + $results[$beforeKey] + " | " + $results[$afterKey] + " | " + $results[$speedupKey] + "x |"
            Add-Content -LiteralPath $env:GITHUB_STEP_SUMMARY -Value $line
        }
    }
} finally {
    if (Test-Path -LiteralPath $baselineSource) {
        $null = Invoke-SS2GitCommand @("-C", $root, "worktree", "remove", "--force", $baselineSource) -AllowFailure
    }
    $null = Invoke-SS2GitCommand @("-C", $root, "worktree", "prune") -AllowFailure
}

exit 0
