param(
    [string]$BuildDir = "build\dev",
    [string]$Config = "Debug"
)

$configure = Join-Path $PSScriptRoot "ss2-configure.ps1"
& $configure -BuildDir $BuildDir
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

. (Join-Path $PSScriptRoot "ss2-common.ps1")
Import-SS2LocalEnvironment
$cmake = Get-SS2CMakeExe
$root = Get-SS2Root
$build = Join-Path $root $BuildDir

& $cmake --build $build --config $Config
exit $LASTEXITCODE
