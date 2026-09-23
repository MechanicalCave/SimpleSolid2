param(
    [switch]$Check,
    [switch]$SelfTest,
    [string]$Root = ""
)

$ErrorActionPreference = "Stop"
if (-not $Root) {
    $Root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

. (Join-Path $PSScriptRoot "docs\\ss2-docs-core.ps1")
. (Join-Path $PSScriptRoot "docs\\ss2-docs-validation.ps1")
. (Join-Path $PSScriptRoot "docs\\ss2-docs-selftest.ps1")

$repoRoot = Get-DocsRoot

if ($SelfTest) {
    Write-Host "=== SS2 documentation validator self-test ==="
    Invoke-DocumentationSelfTest
}

if ($Check) {
    Write-Host "=== SS2 documentation verification ==="
    $errors = Get-DocumentationErrors $repoRoot
    if ($errors.Count -gt 0) {
        foreach ($error in $errors) { Write-Error $error }
        Write-Error "SS2 documentation verification FAILED."
        exit 1
    }
    Write-Host "SS2 documentation verification PASSED."
    exit 0
}

$preErrors = New-Object System.Collections.Generic.List[string]
Test-RequiredStructure $repoRoot $preErrors $false
Test-MetadataAndPairs $repoRoot $preErrors
Test-LocalMarkdownLinks $repoRoot $preErrors
Test-DocumentationImpact $repoRoot $preErrors
if ($preErrors.Count -gt 0) {
    foreach ($error in $preErrors) { Write-Error $error }
    Write-Error "Cannot generate Browser while canonical documentation is invalid."
    exit 1
}

Write-Browser $repoRoot
$after = Get-DocumentationErrors $repoRoot
if ($after.Count -gt 0) {
    foreach ($error in $after) { Write-Error $error }
    exit 1
}
exit 0
