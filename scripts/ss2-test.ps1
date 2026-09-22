param(
    [string]$BuildDir = "build\dev",
    [string]$Config = "Debug"
)

. (Join-Path $PSScriptRoot "ss2-common.ps1")
Import-SS2LocalEnvironment
$cmake = Get-SS2CMakeExe
$root = Get-SS2Root
$build = Join-Path $root $BuildDir

if (-not (Test-Path $build)) {
    & (Join-Path $PSScriptRoot "ss2-build.ps1") -BuildDir $BuildDir -Config $Config
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

& $cmake --build $build --config $Config
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& ctest --test-dir $build -C $Config --output-on-failure
exit $LASTEXITCODE
