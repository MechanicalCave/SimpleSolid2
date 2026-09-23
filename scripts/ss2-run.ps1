param(
    [string]$BuildDir = "build\dev",
    [string]$Config = "Debug"
)

$buildScript = Join-Path $PSScriptRoot "ss2-build.ps1"
& $buildScript -BuildDir $BuildDir -Config $Config
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

. (Join-Path $PSScriptRoot "ss2-common.ps1")
Import-SS2LocalEnvironment

$root = Get-SS2Root
$build = Join-Path $root $BuildDir
$candidates = @(
    (Join-Path $build "src\$Config\SimpleSolid2.exe"),
    (Join-Path $build "src\SimpleSolid2.exe")
)

$exe = $candidates | Where-Object { Test-Path $_ -PathType Leaf } | Select-Object -First 1
if (-not $exe) {
    Write-Error "SimpleSolid2 executable not found after build."
    exit 1
}

Write-Host "Starting $exe"
& $exe
exit $LASTEXITCODE
