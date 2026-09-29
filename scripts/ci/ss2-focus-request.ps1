param(
    [switch]$SelfTest,
    [string]$CommitMessage = "",
    [string]$OutputPath = ""
)

$ErrorActionPreference = "Stop"

function Get-SS2FocusRequest {
    param([string]$Message)

    $targets = New-Object System.Collections.Generic.List[string]
    $tests = New-Object System.Collections.Generic.List[string]
    $sawFocus = $false
    $invalid = $false

    foreach ($line in ($Message -split "\r?\n")) {
        if ($line -match '^SS2-Focus-Target:') {
            $sawFocus = $true
            if ($line -notmatch '^SS2-Focus-Target:\s*([A-Za-z0-9_.-]+)\s*$') {
                $invalid = $true
                continue
            }
            if (-not $targets.Contains($Matches[1])) {
                $targets.Add($Matches[1])
            }
            continue
        }

        if ($line -match '^SS2-Focus-Test:') {
            $sawFocus = $true
            if ($line -notmatch '^SS2-Focus-Test:\s*([A-Za-z0-9_.-]+)\s*$') {
                $invalid = $true
                continue
            }
            if (-not $tests.Contains($Matches[1])) {
                $tests.Add($Matches[1])
            }
        }
    }

    $state =
        if (-not $sawFocus) { "none" }
        elseif ($invalid -or $targets.Count -eq 0 -or $tests.Count -eq 0) { "invalid" }
        else { "valid" }

    return [pscustomobject]@{
        State = $state
        Targets = @($targets)
        Tests = @($tests)
    }
}

function Assert-SS2FocusState {
    param([string]$Message, [string]$Expected)

    $actual = (Get-SS2FocusRequest $Message).State
    if ($actual -ne $Expected) {
        throw "Focus parser self-test failed: expected '$Expected', got '$actual'."
    }
}

if ($SelfTest) {
    Assert-SS2FocusState "ordinary commit" "none"
    Assert-SS2FocusState @"
change

SS2-Focus-Target: e1_arc_numerical_stability_test
SS2-Focus-Test: e1.arc_numerical_stability
"@ "valid"
    Assert-SS2FocusState @"
SS2-Focus-Target:
SS2-Focus-Test: e1.arc_numerical_stability
"@ "invalid"
    Assert-SS2FocusState "SS2-Focus-Target: e1_arc_numerical_stability_test" "invalid"

    $dedupe = Get-SS2FocusRequest @"
SS2-Focus-Target: a
SS2-Focus-Target: a
SS2-Focus-Test: x.y
SS2-Focus-Test: x.y
"@
    if ($dedupe.Targets.Count -ne 1 -or $dedupe.Tests.Count -ne 1) {
        throw "Focus parser self-test did not deduplicate values."
    }

    Write-Host "[focus] parser self-test passed"
    exit 0
}

$request = Get-SS2FocusRequest $CommitMessage
Write-Host "[focus] state=$($request.State)"
Write-Host "[focus] targets=$($request.Targets -join ',')"
Write-Host "[focus] tests=$($request.Tests -join ',')"

if (-not [string]::IsNullOrWhiteSpace($OutputPath)) {
    Add-Content -LiteralPath $OutputPath -Value "state=$($request.State)" -Encoding utf8
    Add-Content -LiteralPath $OutputPath -Value "targets=$($request.Targets -join ',')" -Encoding utf8
    Add-Content -LiteralPath $OutputPath -Value "tests=$($request.Tests -join ',')" -Encoding utf8
}

if ($request.State -eq "invalid") {
    exit 2
}
exit 0
