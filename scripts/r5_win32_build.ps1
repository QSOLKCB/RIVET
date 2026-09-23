param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("x64", "x86")]
    [string]$Arch
)

$ErrorActionPreference = "Stop"

$programFilesX86 = [Environment]::GetFolderPath("ProgramFilesX86")
$vswhere = Join-Path $programFilesX86 "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found"
}

$installation = (& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1)
if (-not $installation) {
    throw "Visual C++ toolchain not found"
}

$devcmd = Join-Path $installation "Common7\Tools\VsDevCmd.bat"
if (-not (Test-Path $devcmd)) {
    throw "VsDevCmd.bat not found"
}

New-Item -ItemType Directory -Force -Path "build" | Out-Null

$lines = @(
    "@echo off",
    ('call "' + $devcmd + '" -no_logo -arch=' + $Arch + ' -host_arch=x64'),
    "if errorlevel 1 exit /b %errorlevel%",
    "cl /nologo /W4 /WX /TC /std:c11 /DRIVET_PLATFORM_TESTING /Iinclude core\rivet.c platform\platform.c platform\win32\platform_win32.c tests\test_platform.c /Fe:build\test-platform-win32.exe",
    "if errorlevel 1 exit /b %errorlevel%",
    "cl /nologo /W4 /WX /TC /std:c11 /Iinclude core\rivet.c platform\platform.c platform\win32\platform_win32.c examples\r5_platform_proof.c /Fe:build\rivet-platform-win32.exe",
    "if errorlevel 1 exit /b %errorlevel%"
)

$cmdPath = Join-Path "build" "r5-win32-build.cmd"
Set-Content -Path $cmdPath -Value $lines -Encoding Ascii

& cmd.exe /d /c $cmdPath
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
