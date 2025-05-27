<#
.SYNOPSIS
    Builds all targets supported by the project, using Ubuntu on WSL for native Linux targets. CalQTest is also ran and checked for success.

.EXAMPLE
    ./scripts/build_all.ps1 -OutDir ./build/build_all -PathPrefix "C:\Qt\Tools\QtCreator\bin\clang\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\mingw1310_64\bin" -QtDir C:\Qt\6.9.0 -CMake C:\Qt\Tools\CMake_64\bin\cmake.exe -WSLQtDir ~/qt/6.9.0 -WSLCMake ~/qt/Tools/CMake/bin/cmake -WSLVCPKGRoot /opt/vcpkg
#>

param (
[Parameter(Mandatory)]
[string]
# The destination directory to save the build tree, passed to CMake.
$OutDir,
[Parameter(Mandatory)]
[string]
# Root directory of Qt installation, with version number.
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
# Suppress non-error subprocess writes to output.
$Quiet
)

function MyLog() {
    process {
        if(-not $Quiet) {
            $_
        }
    }
}

if(Test-Path $OutDir) {
    Remove-Item $OutDir -Recurse -Confirm
}
New-Item $OutDir -Type "Directory" | Out-Null

""
"************** BUILDING ***************"
""
"Building x64-windows-msvc..."
Powershell -ExecutionPolicy Bypass -Command "& '$PSScriptRoot\build.ps1' -OutDir '$OutDir/x64-windows-msvc' -QtDir '$QtDir' -CMake '$CMake' -BuildConfig Debug -TargetPlatform MSVC -PathPrefix '$PathPrefix'" | MyLog
"Built x64-windows-msvc."

"Building x64-windows-mingw..."
Powershell -ExecutionPolicy Bypass -Command "& '$PSScriptRoot\build.ps1' -OutDir '$OutDir/x64-windows-mingw' -QtDir '$QtDir' -CMake '$CMake' -BuildConfig Debug -TargetPlatform MinGW -PathPrefix '$PathPrefix'" | MyLog
"Built x64-windows-mingw."

# Clearing path speeds up compilation.
$WSLBin = $(Get-Command wsl).Source
$OldPATH = $env:PATH
try {
    $env:PATH = ""
    "Building x64-linux..."
    $ScriptPathUnixed = "$PSScriptRoot\build.bash".Replace('\','\\')
    $WSLScriptPath = Invoke-Expression "& '$WSLBin' wslpath -a -u $ScriptPathUnixed"
    Invoke-Expression "& '$WSLBin' $WSLScriptPath --out-dir '$OutDir/x64-linux' --qt-dir '$WSLQtDir' --cmake '$WSLCMake' --vcpkg-root '$WSLVCPKGRoot' --build-config Debug" | MyLog
    "Built x64-linux."
} finally {
    $env:PATH = $OldPATH
}

""
"*************** TESTING ***************"
""

$MSVCBin = "$OutDir/x64-windows-msvc/deploy/Debug/bin/CalQTest.exe"
"Testing x64-windows-msvc at '$MSVCBin'..."
Invoke-Expression "& '$MSVCBin'"
"Tested x64-windows-msvc."

""

$MinGWBin = "$OutDir/x64-windows-msvc/deploy/Debug/bin/CalQTest.exe"
"Testing x64-windows-mingw at '$MinGWBin'..."
Invoke-Expression "& '$MinGWBin'"
"Tested x64-windows-mingw."

""

$LinuxBin = "$OutDir/x64-linux/deploy/Debug/bin/CalQTest"
"Testing x64-linux at '$LinuxBin'..."
wsl $(wsl wslpath -a -u "$LinuxBin".Replace('\','\\'))
"Tested x64-linux."

""
