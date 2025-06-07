<#
.SYNOPSIS
    Builds all targets supported by the project, using Ubuntu on WSL for native Linux targets. CalQTest is also ran and checked for success, although this script does not reflect success/failure in its return value.

.EXAMPLE
    Building Debug for all platforms:
        ./scripts/build_all.ps1
            -OutDir ./build/build_all
            -PathPrefix "C:\Qt\Tools\QtCreator\bin\clang\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\llvm-mingw1706_64\bin"
            -QtDir C:\Qt\6.9.1
            -CMake C:\Qt\Tools\CMake_64\bin\cmake.exe
            -WSLQtDir ~/qt/6.9.1/gcc_64
            -WSLCMake ~/qt/Tools/CMake/bin/cmake
            -WSLVCPKGRoot /opt/vcpkg
            -ForceClean
            -BuildConfig Debug

.EXAMPLE
    Building Release for all platforms in parallel by first mirroring the repo:
        ./scripts/build_all.ps1
            -OutDir ./build/build_all
            -PathPrefix "C:\Qt\Tools\QtCreator\bin\clang\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\llvm-mingw1706_64\bin"
            -QtDir C:\Qt\6.9.1
            -CMake C:\Qt\Tools\CMake_64\bin\cmake.exe
            -WSLQtDir ~//qt/6.9.1/gcc_64
            -WSLCMake ~//qt/Tools/CMake/bin/cmake
            -WSLVCPKGRoot /opt/vcpkg
            -ForceClean
            -BuildConfig Release
            -Mirror
#>

param (
[Parameter(Mandatory)]
[string]
# The destination directory to save the build tree, passed to CMake.
$OutDir,
[Parameter(Mandatory)]
[string]
# The CMake build config e.g. Debug, Release.
$BuildConfig,
[Parameter(Mandatory)]
[string]
# Root directory of Qt installations, with version number. Should contain all necessary installations.
$QtDir,
[Parameter(Mandatory)]
[string]
# Path to CMake to use in operations. Consider using the copy of CMake you can install with Qt.
$CMake,
[string]
# A path to prefix to env:PATH, to help discovery of tools.
$PathPrefix,
[Parameter(Mandatory)]
[string]
# Root directory of Qt installation on WSL installation, with version number.
$WSLQtDir,
[Parameter(Mandatory)]
[string]
# Path to CMake on WSL installation to use in operations. Consider using the copy of CMake you can install with Qt.
$WSLCMake,
[string]
# VCPKG root on WSL installation.
$WSLVCPKGRoot,
[switch]
# If set, for each target: if the build tree already exists, it will be wiped before building. Overrides -Clean.
$ForceClean,
[switch]
# If set, for each target: if the build tree already exists, runs CMake clean target.
$Clean,
[switch]
# Suppress non-error subprocess writes to output.
$Quiet,
[switch]
# OutDir becomes a copy of the source tree, and working directory becomes that folder, as if you ran the script on a clone of the repo.
# Useful for a parallel CI build. Ignores hidden directories and build folder.
$Mirror
)

function MyLog() {
    process {
        if(-not $Quiet) {
            $_
        }
    }
}

$GitDir =  Invoke-Expression "& 'git' rev-parse --show-toplevel"
if(-not $?)
{
    "This is not a git repo, make sure to run this script in the root of the project."
    exit 1
}

$SrcDir = ".";
$Location = Get-Location

try
{
if($MirrorDir -ne "")
{
    if($ForceClean)
    {
        Remove-Item -Recurse './build/build_all'
    }

    Get-ChildItem "." -Exclude 'build' -Attributes !Hidden | %{ Copy-Item $_ -Destination './build/build_all' -Recurse }
    Set-Location './build/build_all'
}

""
"***************************************"
"************ BUILDING ALL *************"
"***************************************"
""

"Building x64-windows-msvc msvc2022_64 install..."
""
$build_command = @"
& '$PSScriptRoot\build.ps1'
    -OutDir '$OutDir/x64-windows-msvc'
    -QtDir '$QtDir/msvc2022_64'
    -CMake '$CMake'
    -BuildConfig '$BuildConfig'
    -CMakePreset x64-windows-msvc
    -PathPrefix '$PathPrefix'
    -ForceClean:`$$ForceClean
    -Clean:`$$Clean
"@
">>> $build_command"
Powershell -ExecutionPolicy Bypass -Command $build_command.replace("`n","").replace("`r","") `
    | %{ "[x64-windows-msvc]    $_" } `
    | MyLog
if($?) {
    $MSVCSuccess = $true
    ""
    "Built x64-windows-msvc."
} else {
    ""
    "Failed to build x64-windows-msvc."
}

""

"Building x64-windows-mingw with llvm-mingw_64 install..."
""
$build_command = @"
& '$PSScriptRoot\build.ps1'
    -OutDir '$OutDir/x64-windows-mingw'
    -QtDir '$QtDir/llvm-mingw_64'
    -CMake '$CMake'
    -BuildConfig '$BuildConfig'
    -CMakePreset x64-windows-mingw
    -PathPrefix '$PathPrefix'
    -ForceClean:`$$ForceClean
    -Clean:`$$Clean
"@
">>> $build_command"
Powershell -ExecutionPolicy Bypass -Command $build_command.replace("`n","").replace("`r","") `
    | %{ "[x64-windows-mingw]    $_" } `
    | MyLog
if($?) {
    $MinGWSuccess = $true
    ""
    "Built x64-windows-mingw."
} else {
    ""
    "Faild to build x64-windows-mingw."
}
""

# Clearing path speeds up compilation, and WSL installations should not depend on host-side system libraries.
$WSLBin = $(Get-Command wsl).Source
$OldPATH = $env:PATH
try {
    $env:PATH = ""
    "Building x64-linux..."
    ""
    $ScriptPathUnixed = "$PSScriptRoot\build.bash".Replace('\','\\')
    $WSLScriptPath = Invoke-Expression "& '$WSLBin' wslpath -a -u $ScriptPathUnixed"
    $ForceCleanFlag = if($ForceClean) { "--force-clean" }
    $CleanFlag = if($Clean) { "--clean" }
    $build_command = @"
& '$WSLBin' $WSLScriptPath
    --out-dir '$OutDir/x64-linux'
    --qt-dir '$WSLQtDir'
    --cmake '$WSLCMake'
    --vcpkg-root '$WSLVCPKGRoot'
    --build-config $BuildConfig
    $ForceCleanFlag
    $CleanFlag
"@
    ">>> $build_command"
    Invoke-Expression $build_command.replace("`n","").replace("`r","") `
        | %{ "[x64-linux]    $_" } `
        | MyLog
    if($LASTEXITCODE -eq 0) {
        $LinuxSuccess = $true
        ""
        "Built x64-linux."
    } else {
        ""
        "Failed to build x64-linux."
    }
} finally {
    $env:PATH = $OldPATH
}

""
"***************************************"
"*************** TESTING ***************"
"***************************************"
""

if($MSVCSuccess) {
    $MSVCBin = "$OutDir/x64-windows-msvc/deploy/$BuildConfig/bin/CalQTest.exe"
    "Testing x64-windows-msvc at '$MSVCBin'..."
    ""
    Invoke-Expression "& '$MSVCBin'"
    ""
    "Tested x64-windows-msvc."
} else {
    "x64-windows-msvc build failed, skipping test."
}

""

if ($MinGWSuccess) {
    $MinGWBin = "$OutDir/x64-windows-msvc/deploy/$BuildConfig/bin/CalQTest.exe"
    "Testing x64-windows-mingw at '$MinGWBin'..."
    ""
    Invoke-Expression "& '$MinGWBin'"
    ""
    "Tested x64-windows-mingw."
} else {
    "x64-windows-mingw build failed, skipping test."
}

""

if($LinuxSuccess) {
    $LinuxBin = "$OutDir/x64-linux/deploy/$BuildConfig/bin/CalQTest"
    "Testing x64-linux at '$LinuxBin'..."
    ""
    wsl $(wsl wslpath -a -u "$LinuxBin".Replace('\','\\'))
    ""
    "Tested x64-linux."
} else {
    "x64-linux build failed, skipping test."
}
} finally {
    Set-Location $Location.Path
}
