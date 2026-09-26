param(
    [Parameter(Position=0)]
    [ValidateSet("setup","verify","configure","build","run","test","docs","clean","status","git-init")]
    [string]$Command = "status",

    [string]$RemoteUrl = "",
    [string]$QtRoot = "D:\Qt",
    [string]$OcctRoot = "D:\SimpleSolid2\.simplesolid-env",
    [switch]$CommitGenesis,
    [switch]$All,

    [ValidateSet("fast","subsystem","full")]
    [string]$Tier = "full",
    [string[]]$Subsystem = @(),
    [switch]$NoBuild,
    [switch]$ListOnly,
    [switch]$SelfTest
)

$map = @{
    "setup"     = "scripts\ss2-setup.ps1"
    "verify"    = "scripts\ss2-verify.ps1"
    "configure" = "scripts\ss2-configure.ps1"
    "build"     = "scripts\ss2-build.ps1"
    "run"       = "scripts\ss2-run.ps1"
    "test"      = "scripts\ss2-test.ps1"
    "docs"      = "scripts\ss2-docs.ps1"
    "clean"     = "scripts\ss2-clean.ps1"
    "status"    = "scripts\ss2-status.ps1"
    "git-init"  = "scripts\ss2-git-init.ps1"
}

if (-not $map.ContainsKey($Command)) {
    Write-Error "SS2 command '$Command' has no dispatcher mapping."
    exit 2
}

$script = Join-Path $PSScriptRoot $map[$Command]
if (-not (Test-Path -LiteralPath $script -PathType Leaf)) {
    Write-Error "SS2 command '$Command' maps to a missing script: $script"
    exit 2
}

switch ($Command) {
    "setup"    { & $script -QtRoot $QtRoot -OcctRoot $OcctRoot }
    "git-init" { & $script -RemoteUrl $RemoteUrl -CommitGenesis:$CommitGenesis }
    "clean"    { & $script -All:$All }
    "test"     { & $script -Tier $Tier -Subsystem $Subsystem -NoBuild:$NoBuild -ListOnly:$ListOnly -SelfTest:$SelfTest }
    default    { & $script }
}
exit $LASTEXITCODE
