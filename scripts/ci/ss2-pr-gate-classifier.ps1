param(
    [switch]$SelfTest,
    [string]$Repository = "",
    [string]$WorkflowFile = "windows-pr-gate.yml",
    [string]$EventName = "pull_request",
    [string]$HeadSha = "",
    [string]$BaseSha = "",
    [string]$PullRequestDraft = "false",
    [ValidateSet("none","valid","invalid")]
    [string]$FocusRequestState = "none",
    [string]$GitHubToken = "",
    [string]$OutputPath = ""
)

$ErrorActionPreference = "Stop"

function Normalize-RepoPath {
    param([string]$Path)
    return (($Path -replace '\\', '/').Trim())
}

function Test-SS2OrdinaryTestContentPath {
    param([Parameter(Mandatory=$true)][string]$Path)
    return (
        $Path -match '^tests/[^/]+_test\.cpp$' -or
        $Path -match '^tests/kernel_native/[^/]+_test\.cpp$'
    )
}

function Test-SS2VerificationInfrastructurePath {
    param([Parameter(Mandatory=$true)][string]$Path)

    if (Test-SS2OrdinaryTestContentPath $Path) {
        return $false
    }

    return (
        $Path -match '^tests/' -or
        $Path -match '^scripts/' -or
        $Path -match '(^|/)CMakeLists\.txt$' -or
        $Path -eq 'ss2.ps1' -or
        $Path -eq 'ss2.cmd' -or
        $Path -match '^\.github/workflows/'
    )
}

function Test-SS2CleanFullForPaths {
    param([string[]]$Paths)

    foreach ($path in @($Paths)) {
        $normalized = Normalize-RepoPath $path
        if ($normalized -and (Test-SS2VerificationInfrastructurePath $normalized)) {
            return $true
        }
    }
    return $false
}

function Get-SS2GateModeForPaths {
    param(
        [string[]]$Paths,
        [bool]$Draft = $false,
        [ValidateSet("none","valid","invalid")]
        [string]$FocusState = "none"
    )

    $normalized = @(
        $Paths |
            ForEach-Object { Normalize-RepoPath $_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )

    if ($normalized.Count -eq 0) {
        return "closure"
    }

    $allClosure = $true
    $allDocs = $true

    foreach ($path in $normalized) {
        $isClosure = $path -match '^work/'
        $isDocs =
            $isClosure -or
            $path -match '^docs/' -or
            $path -match '^governance/' -or
            $path -eq 'README.md' -or
            $path -eq 'AGENTS.md'

        if (-not $isClosure) {
            $allClosure = $false
        }
        if (-not $isDocs) {
            $allDocs = $false
        }
    }

    if ($allClosure) {
        return "closure"
    }
    if ($allDocs) {
        return "docs"
    }

    foreach ($path in $normalized) {
        if (Test-SS2VerificationInfrastructurePath $path) {
            return "full"
        }
    }

    if ($FocusState -eq "invalid") {
        return "full"
    }

    if (-not $Draft) {
        return "full"
    }

    $hasIterationContent = $false
    foreach ($path in $normalized) {
        if (
            $path -match '^work/' -or
            $path -match '^docs/' -or
            $path -match '^governance/' -or
            $path -eq 'README.md' -or
            $path -eq 'AGENTS.md'
        ) {
            continue
        }

        if (
            $path -match '^src/' -or
            (Test-SS2OrdinaryTestContentPath $path)
        ) {
            $hasIterationContent = $true
            continue
        }

        return "full"
    }

    if ($hasIterationContent) {
        if ($FocusState -eq "valid") {
            return "focused"
        }
        return "fast"
    }

    return "full"
}

function Assert-GateMode {
    param(
        [string[]]$Paths,
        [string]$Expected,
        [bool]$Draft = $false,
        [ValidateSet("none","valid","invalid")]
        [string]$FocusState = "none"
    )

    $actual = Get-SS2GateModeForPaths -Paths $Paths -Draft $Draft -FocusState $FocusState
    if ($actual -ne $Expected) {
        throw "Gate classifier self-test failed: expected '$Expected', got '$actual' for draft=$Draft focus=$FocusState paths=[$($Paths -join ', ')]"
    }
}

function Invoke-ClassifierSelfTest {
    Assert-GateMode -Paths @('work/ACTIVE.yaml') -Expected 'closure' -Draft $true
    Assert-GateMode -Paths @('work/ACTIVE.yaml', 'work/CI-02_TEST.md') -Expected 'closure' -Draft $false
    Assert-GateMode -Paths @('docs/internal/BUILD_AND_TEST.md') -Expected 'docs' -Draft $true
    Assert-GateMode -Paths @('governance/DOCUMENTATION.md', 'work/ACTIVE.yaml') -Expected 'docs' -Draft $false
    Assert-GateMode -Paths @('README.md', 'AGENTS.md') -Expected 'docs' -Draft $true

    Assert-GateMode -Paths @('src/part/part_document.cpp') -Expected 'fast' -Draft $true
    Assert-GateMode -Paths @('src/sketch/sketch_model.cpp') -Expected 'focused' -Draft $true -FocusState 'valid'
    Assert-GateMode -Paths @('src/sketch/sketch_model.cpp') -Expected 'full' -Draft $true -FocusState 'invalid'
    Assert-GateMode -Paths @('src/sketch/sketch_model.cpp', 'docs/internal/SHARED_2D.md') -Expected 'fast' -Draft $true
    Assert-GateMode -Paths @('src/part/part_document.cpp') -Expected 'full' -Draft $false -FocusState 'valid'

    Assert-GateMode -Paths @('tests/example_test.cpp') -Expected 'fast' -Draft $true
    Assert-GateMode -Paths @('tests/example_test.cpp') -Expected 'focused' -Draft $true -FocusState 'valid'
    Assert-GateMode -Paths @('tests/kernel_native/pm00a_e05_similarity_evidence_test.cpp') -Expected 'fast' -Draft $true
    Assert-GateMode -Paths @('tests/kernel_native/pm00a_e05_similarity_evidence_test.cpp') -Expected 'focused' -Draft $true -FocusState 'valid'
    Assert-GateMode -Paths @('tests/kernel_native/CMakeLists.txt') -Expected 'full' -Draft $true
    Assert-GateMode -Paths @('tests/CMakeLists.txt') -Expected 'full' -Draft $true
    Assert-GateMode -Paths @('tests/verify_viewer_boundaries.cmake') -Expected 'full' -Draft $true
    Assert-GateMode -Paths @('scripts/ss2-build.ps1') -Expected 'full' -Draft $true
    Assert-GateMode -Paths @('src/CMakeLists.txt') -Expected 'full' -Draft $true
    Assert-GateMode -Paths @('.github/workflows/windows-pr-gate.yml') -Expected 'full' -Draft $true
    Assert-GateMode -Paths @('docs/internal/BUILD_AND_TEST.md', 'src/core/document.cpp') -Expected 'fast' -Draft $true
    Assert-GateMode -Paths @('docs/internal/BUILD_AND_TEST.md', 'src/core/document.cpp') -Expected 'full' -Draft $false
    Assert-GateMode -Paths @('unknown/runtime.file', 'work/ACTIVE.yaml') -Expected 'full' -Draft $true
    Assert-GateMode -Paths @() -Expected 'closure' -Draft $true

    if (-not (Test-SS2CleanFullForPaths @('tests/CMakeLists.txt'))) {
        throw "Gate classifier self-test expected tests/CMakeLists.txt to require CLEAN FULL."
    }
    if (-not (Test-SS2CleanFullForPaths @('scripts/ss2-build.ps1'))) {
        throw "Gate classifier self-test expected build script changes to require CLEAN FULL."
    }
    if (Test-SS2CleanFullForPaths @('src/sketch/sketch_model.cpp', 'tests/example_test.cpp')) {
        throw "Gate classifier self-test incorrectly required CLEAN FULL for ordinary source/test content."
    }

    if (Test-SS2CleanFullForPaths @('tests/kernel_native/pm00a_e05_similarity_evidence_test.cpp')) {
        throw "Gate classifier self-test incorrectly required CLEAN FULL for ordinary kernel-native test content."
    }

    Write-Host '[gate] classifier self-test passed'
}

function Write-GateOutputs {
    param(
        [string]$Mode,
        [string]$TrustedFullSha,
        [string]$DiffStartSha,
        [bool]$CleanFull = $false
    )

    Write-Host "[gate] selected mode: $Mode"
    Write-Host "[gate] trusted FULL SHA: $TrustedFullSha"
    Write-Host "[gate] diff start SHA: $DiffStartSha"
    Write-Host "[gate] clean FULL: $CleanFull"

    if (-not [string]::IsNullOrWhiteSpace($OutputPath)) {
        Add-Content -LiteralPath $OutputPath -Value "mode=$Mode" -Encoding utf8
        Add-Content -LiteralPath $OutputPath -Value "trusted_full_sha=$TrustedFullSha" -Encoding utf8
        Add-Content -LiteralPath $OutputPath -Value "diff_start_sha=$DiffStartSha" -Encoding utf8
        Add-Content -LiteralPath $OutputPath -Value "head_sha=$HeadSha" -Encoding utf8
        Add-Content -LiteralPath $OutputPath -Value "clean_full=$($CleanFull.ToString().ToLowerInvariant())" -Encoding utf8
    }
}

function Get-ChangedPaths {
    param(
        [string]$FromSha,
        [string]$ToSha
    )

    $range = "$FromSha..$ToSha"
    $result = @(& git diff --name-only $range --)
    if ($LASTEXITCODE -ne 0) {
        throw "git diff failed for $range"
    }

    return @(
        $result |
            ForEach-Object { Normalize-RepoPath "$_" } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )
}

function Test-SuccessfulFullGate {
    param([string]$Sha)

    if ([string]::IsNullOrWhiteSpace($GitHubToken) -or
        [string]::IsNullOrWhiteSpace($Repository)) {
        return $false
    }

    $headers = @{
        Authorization = "Bearer $GitHubToken"
        Accept = 'application/vnd.github+json'
        'X-GitHub-Api-Version' = '2022-11-28'
        'User-Agent' = 'SimpleSolid2-CI'
    }

    try {
        $workflow = [Uri]::EscapeDataString($WorkflowFile)
        $runsUri = "https://api.github.com/repos/$Repository/actions/workflows/$workflow/runs?head_sha=$Sha&event=pull_request&status=success&per_page=20"
        $runs = Invoke-RestMethod -Method Get -Uri $runsUri -Headers $headers

        foreach ($run in @($runs.workflow_runs)) {
            $jobs = Invoke-RestMethod -Method Get -Uri $run.jobs_url -Headers $headers
            foreach ($job in @($jobs.jobs)) {
                if ($job.name -eq 'windows-msvc-full' -and
                    $job.conclusion -eq 'success') {
                    Write-Host "[gate] found successful FULL evidence on $Sha (run $($run.id))"
                    return $true
                }
            }
        }
    } catch {
        Write-Warning "Unable to verify prior FULL evidence for SHA $Sha - $($_.Exception.Message)"
        return $false
    }

    return $false
}

if ($SelfTest) {
    Invoke-ClassifierSelfTest
    exit 0
}

if ($EventName -ne 'pull_request') {
    Write-Host '[gate] non-PR invocation: FULL is mandatory'
    Write-GateOutputs -Mode 'full' -TrustedFullSha '' -DiffStartSha $BaseSha -CleanFull $true
    exit 0
}

if ([string]::IsNullOrWhiteSpace($HeadSha) -or
    [string]::IsNullOrWhiteSpace($BaseSha)) {
    Write-Warning '[gate] missing PR SHA context; failing closed to FULL'
    Write-GateOutputs -Mode 'full' -TrustedFullSha '' -DiffStartSha $BaseSha -CleanFull $true
    exit 0
}

$draft =
    $PullRequestDraft.Trim().ToLowerInvariant() -eq 'true'
Write-Host "[gate] pull request draft: $draft"
Write-Host "[gate] focus request state: $FocusRequestState"

$trustedFullSha = ''

try {
    $parentExpression = "$HeadSha^"
    $ancestors = @(& git rev-list --first-parent $parentExpression "^$BaseSha")
    if ($LASTEXITCODE -ne 0) {
        throw 'git rev-list failed'
    }

    foreach ($candidate in $ancestors) {
        if (Test-SuccessfulFullGate "$candidate") {
            $trustedFullSha = "$candidate"
            break
        }
    }
} catch {
    Write-Warning "[gate] unable to inspect FULL ancestors: $($_.Exception.Message)"
    $trustedFullSha = ''
}

$diffStartSha =
    if ([string]::IsNullOrWhiteSpace($trustedFullSha)) {
        $BaseSha
    } else {
        $trustedFullSha
    }

$cleanFull = $false
try {
    $paths = Get-ChangedPaths -FromSha $diffStartSha -ToSha $HeadSha
    Write-Host '[gate] classified paths:'
    foreach ($path in $paths) {
        Write-Host "  $path"
    }
    $mode = Get-SS2GateModeForPaths -Paths $paths -Draft $draft -FocusState $FocusRequestState
    $cleanFull = Test-SS2CleanFullForPaths $paths
} catch {
    Write-Warning "[gate] classification failed; failing closed to FULL: $($_.Exception.Message)"
    $mode = 'full'
    $cleanFull = $true
}

Write-GateOutputs -Mode $mode -TrustedFullSha $trustedFullSha -DiffStartSha $diffStartSha -CleanFull $cleanFull
exit 0
