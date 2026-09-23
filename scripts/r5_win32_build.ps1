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

$testSources = @(
    "core\rivet.c",
    "platform\platform.c",
    "tests\test_platform.c"
) -join " "

$proofSources = @(
    "core\rivet.c",
    "platform\platform.c",
    "platform\win32\platform_win32.c",
    "examples\r5_platform_proof.c"
) -join " "

$prefix = '"' + $devcmd + '" -no_logo -arch=' + $Arch + ' -host_arch=x64 && '
$common = 'cl /nologo /W4 /WX /TC /std:c11 /Iinclude '

$commands = @(
    $prefix + $common + $testSources + ' /Fe:build\test-platform-win32.exe',
    $prefix + $common + $proofSources + ' /Fe:build\rivet-platform-win32.exe'
)

foreach ($command in $commands) {
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}
