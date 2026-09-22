param(
    [string]$QtRoot = "D:\Qt",
    [string]$OcctRoot = "D:\SimpleSolid2\.simplesolid-env"
)

. (Join-Path $PSScriptRoot "ss2-common.ps1")

$root = Get-SS2Root

$git = Get-Command git -ErrorAction SilentlyContinue
if (-not $git) {
    throw "Git was not found in PATH."
}

Write-Host "SS2 workspace : $root"
Write-Host "Qt root       : $QtRoot"
Write-Host "OCCT root     : $OcctRoot"
Write-Host ""

$cmake = Find-CMakeExecutable $QtRoot
$ninja = Find-NinjaExecutable $QtRoot
$qt = Find-Qt6Config $QtRoot
$occt = Find-OCCTConfig $OcctRoot

if ($cmake) { Write-Host "[OK] CMake: $cmake" }
else { Write-Warning "CMake was not found in PATH, Qt Tools, or Program Files." }

if ($ninja) { Write-Host "[OK] Ninja: $ninja" }
else { Write-Host "[INFO] Ninja not found. Visual Studio generator can still be used." }

if ($qt) { Write-Host "[OK] Qt6Config.cmake: $qt" }
else { Write-Warning "Qt6Config.cmake not found under $QtRoot. Project Hub GUI will require Qt." }

if ($occt) { Write-Host "[OK] OpenCASCADE config: $occt" }
else { Write-Host "[INFO] OCCT config not found under $OcctRoot. This does not block Project Hub Genesis." }

if (-not $cmake) {
    throw "CMake could not be located automatically in PATH, Qt Tools, Visual Studio 2022, or Program Files. Install CMake via Qt Maintenance Tool (Developer and Designer Tools -> CMake) or Visual Studio Installer (C++ CMake tools for Windows), then rerun setup."
}

$info = Write-SS2LocalEnvironment `
    -QtRoot $QtRoot `
    -OcctRoot $OcctRoot `
    -QtConfig $qt `
    -OcctConfig $occt `
    -CMakeExe $cmake `
    -NinjaExe $ninja

Write-Host ""
Write-Host "[OK] Local config written: .ss2-local\env.ps1"
Write-Host "CMAKE_PREFIX_PATH = $($info.CMakePrefixPath)"
Write-Host ""
Write-Host "Next:"
Write-Host "  .\ss2.ps1 verify"
Write-Host "  .\ss2.ps1 build"
