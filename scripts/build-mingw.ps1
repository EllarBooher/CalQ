<#
.SYNOPSIS
    This powershell script configures, builds, and locally installs all targets.
    This script exists to allow for granular control of the build environment, since Qt requires tooling built for it.
    The default MinGW CMake preset will be used.
#>

param (
    [Parameter(Mandatory)]
    [string]
    # The destination directory to save the buildtree, passed to CMake.
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
    [switch]
    # Log all commands instead of executing them.
    $DryRun,
    [switch]
    # Suppress output of subprocesses
    $Quiet
)

# Logging function uses ExpandString, so avoid passing script blocks
# that use variable assignment since the LHS would be expanded.
function MyLog
{
    param
    (
        [scriptblock] $block
    )

    $expanded = $ExecutionContext.InvokeCommand.ExpandString($block.ToString())

    if($DryRun)
    {
        "[-DryRun] >>> $expanded"
    } else {
        ">>> $expanded"
        "Running command..."
        if($Quiet)
        {
            $block.Invoke() | Out-Null
        } else {
            $block.Invoke()
        }
        "Done."
    }
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

"Searching for Qt MinGW tooling, with regex pattern 'mingw.*_64'"
$QtMingwTools = (Get-ChildItem $ToolsDir) -match "mingw.*_64" | Sort-Object -Descending
"Found $($QtMingwTools.Length), sorted in descending order:"
"    $QtMingwTools"
$QtMingwToolDir = Join-Path $ToolsDir $QtMingwTools[0]
"Choosing highest version number."

$QtMingwBinDir = Join-Path $QtMingwToolDir "bin"
$NinjaDir = Join-Path $ToolsDir "Ninja"
$QtDir = Join-Path $QtRootDir -ChildPath $QtVersion | Join-Path -ChildPath "mingw_64";

# MinGW bin directory needed for VCPKG CMake toolchain to find the compilers
# Ninja dir needed so Qt compatible Ninja is used at build time
# Qt directory needed for find_package(Qt6)
# PSHome so powershell.exe is on path, a requirement of VCPKG CMake toolchain
$Path = "$QtMingwBinDir;$NinjaDir;$QtDir;$PSHome"

$CMakeBin = Join-Path $ToolsDir "/CMake_64/bin/cmake.exe"

# Separate if statement from the previous, so that the directory exists after being deleted.
if(-not (Test-Path $OutDir))
{
    MyLog {New-Item $OutDir -ItemType "Directory"}
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
    Qt compile tooling : $QtMingwToolDir
    Qt CMake           : $CMakeBin
    env:PATH           : $Path
"@

$OldEnvPath = $env:PATH
MyLog {Set-Item Env:Path $Path}
try{
""
"*************** CLEAN ***************"
""

if($ForceClean -and -not (Get-ChildItem $OutDir).Length -eq 0)
{
    "Output directory '$OutDir' already exists, -ForceClean enabled so deleting its contents."
    MyLog {Get-ChildItem $OutDir | Remove-Item -Recurse -Force}
} else {
    MyLog {& $CMakeBin --build $OutDir --target clean}
}

""
"************* CONFIGURE *************"
""

MyLog {& $CMakeBin `
    -S $SrcDir `
    -B $OutDir `
    --preset "x64-windows-mingw"}

""
"*************** BUILD ***************"
""

MyLog {& $CMakeBin --build $OutDir `
    --target all --config $BuildConfig}

""
"************** INSTALL **************"
""

if(Test-Path $InstallDir)
{
    "Install directory already exists, deleting."
    MyLog {Remove-Item $InstallDir -Recurse}
} else {
    MyLog {New-Item $InstallDir -ItemType "Directory"}
}

MyLog{& $CMakeBin `
    --install $OutDir `
    --prefix $InstallDir `
    --config $BuildConfig}

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