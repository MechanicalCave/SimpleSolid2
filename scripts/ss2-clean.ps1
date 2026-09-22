param(
    [switch]$All
)

. (Join-Path $PSScriptRoot "ss2-common.ps1")
$root = Get-SS2Root
$build = Join-Path $root "build"

if (Test-Path $build) {
    Remove-Item -Recurse -Force $build
    Write-Host "[OK] Usunięto $build"
}
if ($All) {
    $local = Join-Path $root ".ss2-local"
    if (Test-Path $local) {
        Remove-Item -Recurse -Force $local
        Write-Host "[OK] Usunięto $local"
    }
}
