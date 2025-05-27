<#
.SYNOPSIS
    This configures, builds, and locally installs all targets for a given target platform.
    This script exists to allow for granular control of the build environment, since Qt requires tooling built for it.
    The default MinGW CMake preset will be used.
#>

param (
    [string]
    # The destination directory to save the build tree, passed to CMake.
    $OutDir,
    [string]
    # The source directory, passed to CMake. Is the working directory by default.
    $SrcDir = ".",
    [Parameter(Mandatory)]
    [string]
    # Root directory of Qt installation, e.g. "C:/Qt".
    $QtRootDir,
    [Parameter(Mandatory)]
    [string]
    # Which installed semver of qt to use, e.g. "6.9.0".
    $QtVersion,
    [Parameter(Mandatory)]
    [string]
    # Which build config to use, e.g. Debug. Passed to CMake as --config $BuildConfig
    $BuildConfig,
    [switch]
    # If the buildtree already exists, remove it. Otherwise, CMake --clean option is used. Leaving this off effectively performs a rebuild.
    $ForceClean,
    [Parameter(Mandatory)]
    [ValidateSet("MinGW", "MSVC")]
    [string]
    # The platform to target: MinGW or MSVC.
    $TargetPlatform,
    [switch]
    # Log all commands instead of executing them.
    $DryRun,
    [switch]
    # Suppress output of subprocesses
    $Quiet
)

function MyLog() {
    process {
        if(-not $Quiet) {
            $_
        }
    }
}

$preset = if($TargetPlatform -eq "MinGW") { "x64-windows-mingw" } else { "x64-windows-msvc"}
if($OutDir -eq "")
{
    $OutDir = Join-Path "./build/" $preset
}

if(-not $(Test-Path $SrcDir)) {
    "Could not find source directory at '$SrcDir' - Folder does not exist. Now exiting."
    Exit
}
$SrcDir = Resolve-Path $SrcDir

if(-not $(Test-Path $QtRootDir)) {
    "Could not find Qt installed at '$QtRootDir' - Folder does not exist. Now exiting."
    Exit
}

$ToolsDir = Join-Path (Resolve-Path $QtRootDir) "/Tools"
if(-not $(Test-Path $ToolsDir)) {
    "Could not find Qt tools at '$ToolsDir' - Folder does not exist. Now exiting."
    Exit
}

if($TargetPlatform -eq "MinGW")
{
"Searching for Qt MinGW tooling, with regex pattern 'mingw.*_64'"
$QtMingwTools = (Get-ChildItem $ToolsDir) -match "mingw.*_64" | Sort-Object -Descending
"Found $($QtMingwTools.Length), sorted in descending order:"
"    $QtMingwTools"
"Choosing highest version number."

$QtMingwBinDir = Join-Path $ToolsDir -ChildPath $QtMingwTools[0] | Join-Path -ChildPath "bin"
$NinjaDir = Join-Path $ToolsDir "Ninja"
$QtDir = Join-Path $QtRootDir -ChildPath $QtVersion | Join-Path -ChildPath "mingw_64";

# MinGW bin directory needed for VCPKG CMake toolchain to find the compilers
# Ninja dir needed so Qt compatible Ninja is used at build time
# Qt directory needed for find_package(Qt6)
# PSHome so powershell.exe is on path, a requirement of VCPKG CMake toolchain
$Path = "$QtMingwBinDir;$NinjaDir;$QtDir;$PSHome"
} else {
"Searching for Qt MSVC install dir, with regex pattern 'msvc.*_64'"
$QtMSVCs = (Get-ChildItem (Join-Path $QtRootDir -ChildPath $QtVersion)) -match "msvc.*_64" | Sort-Object -Descending
"Found $($QtMSVCs.Length), sorted in descending order:"
"    $QtMSVCs"
"Choosing highest version number."

# MSVC compiler externally installed, there is no point in adding hints so we just add Qt.
$QtDir = Join-Path $QtRootDir -ChildPath $QtVersion | Join-Path -ChildPath $QtMSVCs[0]

# Qt directory needed for find_package(Qt6)
# PSHome so powershell.exe is on path, a requirement of VCPKG CMake toolchain
$Path = "$QtDir;$PSHome"
}

$CMakeBin = Join-Path $ToolsDir "/CMake_64/bin/cmake.exe"

# Separate if statement from the previous, so that the directory exists after being deleted.
if(-not (Test-Path $OutDir))
{
    New-Item $OutDir -ItemType "Directory" | Out-Null
}

$OutDir = Resolve-Path $OutDir
$InstallDir = Join-Path $OutDir -ChildPath "deploy" | Join-Path -ChildPath $BuildConfig;

@"
Using:
    Source Directory   : $SrcDir
    Build Directory    : $OutDir
    Install Directory  : $InstallDir
    Build Config       : $BuildConfig
    Qt                 : $QtDir
"@
if($TargetPlatform -eq "MinGW") {
@"
    Compile tooling    : $QtMingwToolDir
"@
}
@"
    Qt CMake           : $CMakeBin
    env:PATH           : $Path
"@

$OldEnvPath = $env:PATH
$env:PATH = "$Path;$env:PATH"

try{
""
"*************** CLEAN ***************"
""
if($ForceClean -and -not (Get-ChildItem $OutDir).Length -eq 0)
{
    "Output directory '$OutDir' already exists, -ForceClean enabled so deleting its contents."
    Get-ChildItem $OutDir | Remove-Item -Recurse -Force
} else {
    $ExecutionContext.InvokeCommand.ExpandString(">>> & $CMakeBin ``
        --build $OutDir ``
        --target clean")
    & $CMakeBin --build $OutDir --target clean | MyLog
}

""
"************* CONFIGURE *************"
""

$ExecutionContext.InvokeCommand.ExpandString(">>> & $CMakeBin `
    -S $SrcDir `
    -B $OutDir `
    --preset $preset")
& $CMakeBin -S $SrcDir -B $OutDir --preset $preset | MyLog
if($LASTEXITCODE -ne 0)
{
    ""
    "Configuring failed, deleting build directory and now exiting."
    Get-ChildItem $OutDir | Remove-Item -Recurse -Force
    exit
}

""
"*************** BUILD ***************"
""

$ExecutionContext.InvokeCommand.ExpandString(">>> & $CMakeBin --build $OutDir `
    --target all --config $BuildConfig")
& $CMakeBin --build $OutDir `
    --config $BuildConfig | MyLog
if($LASTEXITCODE -ne 0)
{
    ""
    "Building failed, now exiting."
    exit
}

""
"************** INSTALL **************"
""

if(Test-Path $InstallDir)
{
    "Install directory already exists, deleting."
    Remove-Item $InstallDir -Recurse -Force
}
New-Item $InstallDir -ItemType "Directory" | Out-Null

& $CMakeBin `
    --install $OutDir `
    --prefix $InstallDir `
    --config $BuildConfig | MyLog

if($LASTEXITCODE -ne 0)
{
    ""
    "Installing failed, now exiting."
    exit
}

""
"*************************************"
""
"Success. Check the following locations:
    Build: '$OutDir'
    Install: '$InstallDir'
"
} finally {
    $env:PATH = $OldEnvPath
}
