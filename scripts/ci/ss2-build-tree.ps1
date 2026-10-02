param(
    [ValidateSet("prepare","record")]
    [string]$Action = "prepare",
    [string]$BuildDir = "",
    [ValidateSet("desktop","core","kernel")]
    [string]$Mode = "desktop",
    [string]$Config = "Debug",
    [switch]$ForceClean,
    [switch]$SelfTest,
    [string]$OutputPath = ""
)

$ErrorActionPreference = "Stop"

function Compare-SS2Fingerprint {
    param(
        [Parameter(Mandatory=$true)]$Expected,
        [Parameter(Mandatory=$true)]$Actual
    )

    $keys = @(
        "schema",
        "sourceRoot",
        "cmakeExe",
        "cmakeVersion",
        "generatorHint",
        "architecture",
        "qtPrefix",
        "occtPrefix",
        "mode",
        "config",
        "buildOptions"
    )

    foreach ($key in $keys) {
        if ("$($Expected.$key)" -ne "$($Actual.$key)") {
            return "fingerprint:$key"
        }
    }
    return ""
}

if ($SelfTest) {
    $a = [pscustomobject]@{
        schema = 1
        sourceRoot = "A"
        cmakeExe = "C"
        cmakeVersion = "1"
        generatorHint = "G"
        architecture = "x64"
        qtPrefix = "Q"
        occtPrefix = "O"
        mode = "desktop"
        config = "Debug"
        buildOptions = "desktop"
    }
    $b = $a | Select-Object *
    if (Compare-SS2Fingerprint $a $b) {
        throw "Build-tree fingerprint self-test rejected equal fingerprints."
    }
    $b.mode = "kernel"
    if ((Compare-SS2Fingerprint $a $b) -ne "fingerprint:mode") {
        throw "Build-tree fingerprint self-test did not detect mode mismatch."
    }
    Write-Host "[build-tree] self-test passed"
    exit 0
}

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    throw "CI-04 build-tree manager requires -BuildDir."
}

. (Join-Path (Split-Path $PSScriptRoot -Parent) "ss2-common.ps1")
Import-SS2LocalEnvironment

$build = Resolve-SS2BuildPath $BuildDir
$root = [IO.Path]::GetFullPath((Get-SS2Root))
$cmake = [IO.Path]::GetFullPath((Get-SS2CMakeExe))
$fingerprintPath = Join-Path $build ".ss2-build-fingerprint.json"
$cachePath = Join-Path $build "CMakeCache.txt"

function Get-SS2CMakeVersion {
    $line = (& $cmake --version | Select-Object -First 1)
    if ("$line" -match 'cmake version\s+(.+)$') { return $Matches[1].Trim() }
    return "$line".Trim()
}

function Get-SS2DefaultGeneratorHint {
    foreach ($line in @(& $cmake --help 2>$null)) {
        if ("$line" -match '^\*\s+(.+?)\s+=') {
            return $Matches[1].Trim()
        }
    }
    return ""
}

function Get-SS2CacheValue {
    param([Parameter(Mandatory=$true)][string]$Key)
    if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) { return "" }
    $escaped = [regex]::Escape($Key)
    foreach ($line in Get-Content -LiteralPath $cachePath) {
        if ("$line" -match "^${escaped}:[^=]*=(.*)$") {
            return $Matches[1]
        }
    }
    return ""
}

function Get-SS2FileVersion {
    param([string]$Path)
    if (-not $Path -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) { return "" }
    try {
        return [Diagnostics.FileVersionInfo]::GetVersionInfo($Path).FileVersion
    } catch {
        return ""
    }
}

function New-SS2StaticFingerprint {
    $desktop = $Mode -eq "desktop"
    $kernel = $Mode -eq "kernel"
    $buildOptions = switch ($Mode) {
        "desktop" {
            "SS2_BUILD_DESKTOP=ON;SS2_BUILD_KERNEL_NATIVE=OFF"
        }
        "kernel" {
            "SS2_BUILD_DESKTOP=OFF;SS2_BUILD_KERNEL_NATIVE=ON;CMAKE_DISABLE_FIND_PACKAGE_Qt6=TRUE"
        }
        "core" {
            "SS2_BUILD_DESKTOP=OFF;SS2_BUILD_KERNEL_NATIVE=OFF;CMAKE_DISABLE_FIND_PACKAGE_Qt6=TRUE;CMAKE_DISABLE_FIND_PACKAGE_OpenCASCADE=TRUE"
        }
        default {
            throw "Unknown SS2 build-tree mode '$Mode'."
        }
    }

    return [ordered]@{
        schema = 1
        sourceRoot = $root
        cmakeExe = $cmake
        cmakeVersion = Get-SS2CMakeVersion
        generatorHint = Get-SS2DefaultGeneratorHint
        architecture = "$env:PROCESSOR_ARCHITECTURE"
        qtPrefix = if ($desktop) { "$env:SS2_QT_PREFIX" } else { "" }
        occtPrefix = if ($desktop -or $kernel) { "$env:SS2_OCCT_PREFIX" } else { "" }
        mode = $Mode
        config = $Config
        buildOptions = $buildOptions
    }
}

function Write-SS2State {
    param([string]$State, [string]$Reason)
    Write-Host "[build-cache] state=$State"
    Write-Host "[build-cache] build=$build"
    if ($Reason) { Write-Host "[build-cache] reason=$Reason" }
    if ($OutputPath) {
        Add-Content -LiteralPath $OutputPath -Value "state=$State" -Encoding utf8
        Add-Content -LiteralPath $OutputPath -Value "build_dir=$build" -Encoding utf8
        Add-Content -LiteralPath $OutputPath -Value "reason=$Reason" -Encoding utf8
    }
}

if ($Action -eq "prepare") {
    $state = "cold"
    $reason = "missing-build-tree"

    if ($ForceClean) {
        if (Test-Path -LiteralPath $build) {
            Remove-Item -LiteralPath $build -Recurse -Force
        }
        $state = "forced-clean"
        $reason = "requested"
    } elseif (Test-Path -LiteralPath $build -PathType Container) {
        if (-not (Test-Path -LiteralPath $fingerprintPath -PathType Leaf)) {
            Remove-Item -LiteralPath $build -Recurse -Force
            $state = "invalidated"
            $reason = "missing-fingerprint"
        } else {
            try {
                $stored = Get-Content -LiteralPath $fingerprintPath -Raw | ConvertFrom-Json
                $current = [pscustomobject](New-SS2StaticFingerprint)
                $mismatch = Compare-SS2Fingerprint $stored $current

                if (-not $mismatch -and "$($stored.compilerPath)") {
                    $compilerVersion = Get-SS2FileVersion "$($stored.compilerPath)"
                    if (-not $compilerVersion) {
                        $mismatch = "compiler-missing"
                    } elseif ($compilerVersion -ne "$($stored.compilerVersion)") {
                        $mismatch = "compiler-version"
                    }
                }

                if (-not $mismatch -and (Test-Path -LiteralPath $cachePath -PathType Leaf)) {
                    $cacheSource = Get-SS2CacheValue "CMAKE_HOME_DIRECTORY"
                    if ($cacheSource -and
                        -not [string]::Equals(
                            [IO.Path]::GetFullPath($cacheSource),
                            $root,
                            [StringComparison]::OrdinalIgnoreCase)) {
                        $mismatch = "cmake-source-root"
                    }
                    $cacheGenerator = Get-SS2CacheValue "CMAKE_GENERATOR"
                    if (-not $mismatch -and "$($stored.actualGenerator)" -and
                        $cacheGenerator -ne "$($stored.actualGenerator)") {
                        $mismatch = "cmake-generator"
                    }
                }

                if ($mismatch) {
                    Remove-Item -LiteralPath $build -Recurse -Force
                    $state = "invalidated"
                    $reason = $mismatch
                } else {
                    $state = "warm"
                    $reason = "fingerprint-match"
                }
            } catch {
                Remove-Item -LiteralPath $build -Recurse -Force
                $state = "invalidated"
                $reason = "unreadable-fingerprint"
            }
        }
    }

    New-Item -ItemType Directory -Force -Path $build | Out-Null
    Write-SS2State $state $reason
    exit 0
}

if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) {
    throw "Cannot record CI-04 build fingerprint without CMakeCache.txt: $cachePath"
}

$static = New-SS2StaticFingerprint
$compilerPath = Get-SS2CacheValue "CMAKE_CXX_COMPILER"
$record = [ordered]@{}
foreach ($entry in $static.GetEnumerator()) {
    $record[$entry.Key] = $entry.Value
}
$record["actualGenerator"] = Get-SS2CacheValue "CMAKE_GENERATOR"
$record["compilerPath"] = $compilerPath
$record["compilerVersion"] = Get-SS2FileVersion $compilerPath

$json = $record | ConvertTo-Json -Depth 4
Set-Content -LiteralPath $fingerprintPath -Value $json -Encoding UTF8
Write-Host "[build-cache] recorded=$fingerprintPath"
exit 0
