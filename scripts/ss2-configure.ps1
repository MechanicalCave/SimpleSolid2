param(
    [string]$BuildDir = "build\dev"
)

. (Join-Path $PSScriptRoot "ss2-common.ps1")
Import-SS2LocalEnvironment

$cmake = Get-SS2CMakeExe
$root = Get-SS2Root
$build = Join-Path $root $BuildDir

$args = @("-S", $root, "-B", $build)
if ($env:CMAKE_PREFIX_PATH) {
    $args += "-DCMAKE_PREFIX_PATH=$env:CMAKE_PREFIX_PATH"
}

Write-Host "$cmake $($args -join ' ')"
& $cmake @args
exit $LASTEXITCODE
