$ErrorActionPreference = "Stop"

function Get-SS2Root {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Find-ExecutableCandidate([string[]]$Candidates) {
    foreach ($candidate in $Candidates) {
        if ([string]::IsNullOrWhiteSpace($candidate)) { continue }
        if (Test-Path $candidate -PathType Leaf) {
            return (Resolve-Path $candidate).Path
        }
    }
    return $null
}


function Find-CMakeExecutable([string]$QtRoot = "D:\Qt") {
    $cmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $candidates = @(
        (Join-Path $QtRoot "Tools\CMake_64\bin\cmake.exe"),
        (Join-Path $QtRoot "Tools\CMake\bin\cmake.exe"),
        "C:\Program Files\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    )
    $found = Find-ExecutableCandidate $candidates
    if ($found) { return $found }

    # Query Visual Studio Installer if available.
    $vswhereCandidates = @(
        "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe",
        "C:\Program Files\Microsoft Visual Studio\Installer\vswhere.exe"
    )
    $vswhere = Find-ExecutableCandidate $vswhereCandidates
    if ($vswhere) {
        $installPaths = & $vswhere -products * -version "[17.0,18.0)" -property installationPath 2>$null
        foreach ($install in $installPaths) {
            if ([string]::IsNullOrWhiteSpace($install)) { continue }
            $candidate = Join-Path $install "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
            if (Test-Path $candidate) { return (Resolve-Path $candidate).Path }
        }
    }

    if (Test-Path $QtRoot) {
        $foundFile = Get-ChildItem -Path $QtRoot -Filter "cmake.exe" -File -Recurse -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match '\\Tools\\CMake' } |
            Select-Object -First 1
        if ($foundFile) { return $foundFile.FullName }
    }

    return $null
}


function Find-NinjaExecutable([string]$QtRoot = "D:\Qt") {
    $cmd = Get-Command ninja -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $candidates = @(
        (Join-Path $QtRoot "Tools\Ninja\ninja.exe"),
        (Join-Path $QtRoot "Tools\Ninja_64\ninja.exe"),
        "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
    )
    $found = Find-ExecutableCandidate $candidates
    if ($found) { return $found }

    $vswhereCandidates = @(
        "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe",
        "C:\Program Files\Microsoft Visual Studio\Installer\vswhere.exe"
    )
    $vswhere = Find-ExecutableCandidate $vswhereCandidates
    if ($vswhere) {
        $installPaths = & $vswhere -products * -version "[17.0,18.0)" -property installationPath 2>$null
        foreach ($install in $installPaths) {
            if ([string]::IsNullOrWhiteSpace($install)) { continue }
            $candidate = Join-Path $install "Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
            if (Test-Path $candidate) { return (Resolve-Path $candidate).Path }
        }
    }

    if (Test-Path $QtRoot) {
        $foundFile = Get-ChildItem -Path $QtRoot -Filter "ninja.exe" -File -Recurse -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match '\\Tools\\Ninja' } |
            Select-Object -First 1
        if ($foundFile) { return $foundFile.FullName }
    }

    return $null
}

function Import-SS2LocalEnvironment {
    $root = Get-SS2Root
    $envFile = Join-Path $root ".ss2-local\env.ps1"
    if (-not (Test-Path $envFile)) {
        throw "Missing $envFile. Run first: .\ss2.ps1 setup"
    }
    . $envFile
}

function Find-Qt6Config([string]$QtRoot) {
    if (-not (Test-Path $QtRoot)) { return $null }

    $versionDirs = Get-ChildItem -Path $QtRoot -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^\d+\.\d+' } |
        Sort-Object {
            try { [version]($_.Name -replace '[^\d\.].*$','') }
            catch { [version]'0.0' }
        } -Descending

    foreach ($v in $versionDirs) {
        $kits = Get-ChildItem -Path $v.FullName -Directory -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -match '^(msvc.*_64|clang_64|mingw.*_64)$' } |
            Sort-Object @{Expression={ if ($_.Name -like 'msvc*') {0} elseif ($_.Name -like 'clang*') {1} else {2} }}, Name
        foreach ($kit in $kits) {
            $candidate = Join-Path $kit.FullName "lib\cmake\Qt6\Qt6Config.cmake"
            if (Test-Path $candidate) { return $candidate }
        }
    }

    $found = Get-ChildItem -Path $QtRoot -Filter "Qt6Config.cmake" -File -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($found) { return $found.FullName }
    return $null
}

function Find-OCCTConfig([string]$OcctRoot) {
    $roots = New-Object System.Collections.Generic.List[string]

    function Add-OCCTSearchRoot([string]$Path) {
        if (-not $Path) { return }
        if (Test-Path $Path -PathType Leaf) {
            $name = Split-Path $Path -Leaf
            if ($name -ieq "OpenCASCADEConfig.cmake" -or
                $name -ieq "opencascade-config.cmake") {
                $resolved = (Resolve-Path $Path).Path
                if (-not $roots.Contains($resolved)) { $roots.Add($resolved) }
            }
            return
        }
        if (Test-Path $Path -PathType Container) {
            $resolved = (Resolve-Path $Path).Path
            if (-not $roots.Contains($resolved)) { $roots.Add($resolved) }
        }
    }

    Add-OCCTSearchRoot $env:OpenCASCADE_DIR
    Add-OCCTSearchRoot $env:CASROOT
    Add-OCCTSearchRoot $env:VCPKG_ROOT
    Add-OCCTSearchRoot $OcctRoot

    # Reuse the machine-local roots that the accepted SS1 native gates used on
    # the shared Windows CAD development host.
    $workspaceParent = Split-Path (Get-SS2Root) -Parent
    Add-OCCTSearchRoot $workspaceParent
    Add-OCCTSearchRoot "C:\SimpleSolid"
    Add-OCCTSearchRoot "C:\SimpleSolid\.simplesolid-env"

    if ($OcctRoot) {
        $driveRoot = [System.IO.Path]::GetPathRoot($OcctRoot)
        if ($driveRoot) {
            Add-OCCTSearchRoot (Join-Path $driveRoot ".simplesolid-env")
        }
    }

    foreach ($candidate in @("C:\vcpkg", "C:\OCCT", "C:\OpenCASCADE")) {
        Add-OCCTSearchRoot $candidate
    }
    if ($env:ProgramFiles) {
        Add-OCCTSearchRoot (Join-Path $env:ProgramFiles "OpenCASCADE")
        Add-OCCTSearchRoot (Join-Path $env:ProgramFiles "OCCT")
    }

    foreach ($root in $roots) {
        if (Test-Path $root -PathType Leaf) { return $root }

        foreach ($name in @("OpenCASCADEConfig.cmake", "opencascade-config.cmake")) {
            $found = Get-ChildItem -Path $root -Filter $name -File -Recurse -ErrorAction SilentlyContinue |
                Where-Object {
                    $_.FullName -notmatch "[\\/](buildtrees|packages|templates)[\\/]"
                } |
                Select-Object -First 1
            if ($found) { return $found.FullName }
        }
    }

    return $null
}

function Get-OCCTPrefixFromConfig([string]$OcctConfig) {
    if (-not $OcctConfig) { return "" }

    $configDir = Split-Path $OcctConfig -Parent
    $parent = Split-Path $configDir -Parent
    $parentName = Split-Path $parent -Leaf

    if ($parentName -ieq "share") {
        return (Split-Path $parent -Parent)
    }

    $p = $configDir
    for ($i = 0; $i -lt 3; $i++) { $p = Split-Path $p -Parent }
    return $p
}

function Write-SS2LocalEnvironment(
    [string]$QtRoot,
    [string]$OcctRoot,
    [string]$QtConfig,
    [string]$OcctConfig,
    [string]$CMakeExe,
    [string]$NinjaExe
) {
    $root = Get-SS2Root
    $localDir = Join-Path $root ".ss2-local"
    New-Item -ItemType Directory -Force -Path $localDir | Out-Null

    $qtPrefix = if ($QtConfig) {
        (Resolve-Path (Join-Path (Split-Path $QtConfig -Parent) "..\..\..")).Path
    } else { "" }

    $occtPrefix = Get-OCCTPrefixFromConfig $OcctConfig

    $prefixes = @()
    if ($qtPrefix) { $prefixes += $qtPrefix }
    if ($occtPrefix) { $prefixes += $occtPrefix }
    $prefixPath = ($prefixes -join ";")

    $extraPathParts = @()
    if ($CMakeExe) { $extraPathParts += (Split-Path $CMakeExe -Parent) }
    if ($NinjaExe) { $extraPathParts += (Split-Path $NinjaExe -Parent) }
    if ($qtPrefix) { $extraPathParts += (Join-Path $qtPrefix "bin") }
    if ($occtPrefix) {
        foreach ($relativeRuntime in @("bin", "debug\bin")) {
            $runtimePath = Join-Path $occtPrefix $relativeRuntime
            if (Test-Path $runtimePath -PathType Container) {
                $extraPathParts += (Resolve-Path $runtimePath).Path
            }
        }
    }
    $extraPath = ($extraPathParts | Select-Object -Unique) -join ";"

    $content = @"
# AUTO-GENERATED by SS2 bootstrap. Machine-local; intentionally gitignored.
`$env:SS2_WORKSPACE = '$root'
`$env:SS2_QT_ROOT = '$QtRoot'
`$env:SS2_OCCT_ROOT = '$OcctRoot'
`$env:SS2_QT_PREFIX = '$qtPrefix'
`$env:SS2_OCCT_PREFIX = '$occtPrefix'
`$env:SS2_CMAKE_EXE = '$CMakeExe'
`$env:SS2_NINJA_EXE = '$NinjaExe'
`$env:CMAKE_PREFIX_PATH = '$prefixPath'
if ('$extraPath') { `$env:PATH = '$extraPath;' + `$env:PATH }
"@
    Set-Content -Path (Join-Path $localDir "env.ps1") -Value $content -Encoding UTF8

    [pscustomobject]@{
        Workspace = $root
        QtConfig = $QtConfig
        QtPrefix = $qtPrefix
        OCCTConfig = $OcctConfig
        OCCTPrefix = $occtPrefix
        CMakeExe = $CMakeExe
        NinjaExe = $NinjaExe
        CMakePrefixPath = $prefixPath
    }
}

function Get-SS2CMakeExe {
    if ($env:SS2_CMAKE_EXE -and (Test-Path $env:SS2_CMAKE_EXE)) {
        return $env:SS2_CMAKE_EXE
    }
    $cmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    throw "CMake executable not found. Run: .\ss2.ps1 setup"
}
