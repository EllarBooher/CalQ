<#
.SYNOPSIS
    This configures, builds, and locally installs all targets for a given target platform.
    This script exists to allow for granular control of the build environment, since Qt requires tooling built for it.
    The default MinGW CMake preset will be used.

.EXAMPLE
    .\scripts\build.ps1 -QtDir C:\Qt\6.9.0 -CMake C:\Qt\Tools\CMake_64\bin\cmake.exe -BuildConfig Debug -TargetPlatform MinGW -ForceClean -PathPrefix "C:\Qt\Tools\Ninja;C:\Qt\Tools\mingw1310_64\bin"
        When ran inside the project root CalQ, configures, builds, and installs targeting MinGW with Qt 6.9.0.
        Note the argument for PathPrefix: This allows vcpkg to discover the generator/compilers if they are not on env:PATH.
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
    # Root directory of Qt installation, with version number.
    $QtDir,
    [Parameter(Mandatory)]
    [string]
    # Path to CMake to use in operations. Consider using the copy of CMake you can install with Qt.
    $CMake,
    [Parameter(Mandatory)]
    [string]
    # Which build config to use, e.g. Debug. Passed to CMake as --config $BuildConfig
    $BuildConfig,
    [switch]
    # If set and the build tree already exists, it will be wiped before building. In any other case, CMake clean target is executed.
    $ForceClean,
    [Parameter(Mandatory)]
    [ValidateSet("MinGW", "MSVC")]
    [string]
    # The platform to target: MinGW or MSVC.
    $TargetPlatform,
    [switch]
    # Log all commands instead of executing them.
    $DryRun,
    [string]
    # A path to prefix to env:PATH, to help discovery of tools.
    $PathPrefix,
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

$preset = if($TargetPlatform -eq "MinGW") { "x64-windows-mingw" } else { "x64-windows-msvc" }
if($OutDir -eq "") {
    $OutDir = Join-Path "./build/" $preset
}
if(-not $(Test-Path $OutDir)) {
    "Output directory does not exist, creating '$OutDir'."
    if($DryRun) {
        ">>> New-Item $OutDir -Type `"Directory`""
        "[[[ -DryRun skipped ]]]"
    } else {
        New-Item $OutDir -Type "Directory" | Out-Null
    }
}

if(-not $DryRun) {
    $OutDir = Resolve-Path $OutDir
}

if(-not $(Test-Path $SrcDir)) {
    "Could not find source directory at '$SrcDir' - Folder does not exist. Now exiting."
    Exit
}
$SrcDir = Resolve-Path $SrcDir

if(-not $(Test-Path $QtDir)) {
    "Could not find Qt installed at '$QtDir' - Folder does not exist. Now exiting."
    Exit
}
$QtDir = Resolve-Path $QtDir

if($TargetPlatform -eq "MinGW") {
    $QtDir = Join-Path $QtDir "mingw_64"
} else {
# We could search for a specific version based on the Qt version, but this is simpler
    "Searching for Qt MSVC install dir, with regex pattern 'msvc.*_64'"
    $QtVersions = (Get-ChildItem $QtDir) -match "msvc.*_64" | Sort-Object -Descending
    "Found $($QtVersions.Length), sorted in descending order:"
    "    $QtVersions"
    "Choosing highest version number."
    $QtDir = Join-Path $QtDir $QtVersions[0]
}

if(-not $(Test-Path $QtDir)) {
    "Could not find Qt for selected platform at '$QtDir' - Folder does not exist. Now exiting."
    Exit
}

$Path = "$PathPrefix;$QtDir;$PSHome"

$CMake = Resolve-Path $CMake
$InstallDir = Join-Path $OutDir -ChildPath "deploy" | Join-Path -ChildPath $BuildConfig;

@"
Using:
    Source Directory   : $SrcDir
    Build Directory    : $OutDir
    Install Directory  : $InstallDir
    Build Config       : $BuildConfig
    Qt                 : $QtDir
    Qt CMake           : $CMake
    env:PATH           : $Path;$env:PATH
"@

$OldEnvPath = $env:PATH
$env:PATH = "$Path;$env:PATH"

try{
""
"*************** CLEAN ***************"
""
# We check for the count of children, and not existence of the directory, since we (potentially) created it earlier.
if($ForceClean -and -not (Get-ChildItem $OutDir).Length -eq 0) {
    "Output directory '$OutDir' already exists, -ForceClean enabled so deleting its contents."
    if($DryRun) {
        ">>> Remove-Item $OutDir -Recurse"
        ">>> New-Item $OutDir -Type `"Directory`""
        "[[[ -DryRun skipped ]]]"
    } else {
        # Split this into two commands, so the user gets just a single prompt for removing the existing directory
        Remove-Item $OutDir -Recurse
        New-Item $OutDir -Type "Directory"
    }
} else {
    ">>> & $CMake ``
        --build $OutDir ``
        --target clean"
    if($DryRun) {
        "[[[ -DryRun skipped ]]]"
        ""
    } else {
        & $CMake --build $OutDir --target clean | MyLog
    }
}

""
"************* CONFIGURE *************"
""

">>> & $CMake ``
        -S $SrcDir ``
        -B $OutDir ``
        --preset $preset"
if($DryRun) {
    "[[[ -DryRun skipped ]]]"
} else {
    & $CMake -S $SrcDir -B $OutDir --preset $preset | MyLog
}
if($LASTEXITCODE -ne 0)
{
    ""
    "Configuring failed, deleting build directory and now exiting."
    if($DryRun) {
        ">>> Remove-Item $OutDir -Recurse"
        "[[[ -DryRun skipped ]]]"
    } else {
        Remove-Item $OutDir -Recurse
    }
    exit
}

""
"*************** BUILD ***************"
""

">>> & $CMake ``
    --build $OutDir ``
    --config $BuildConfig"
if($DryRun) {
    "[[[ -DryRun skipped ]]]"
} else {
    & $CMake --build $OutDir --config $BuildConfig | MyLog
    if($LASTEXITCODE -ne 0)
    {
        ""
        "Building failed, now exiting."
        exit
    }
}

""
"************** INSTALL **************"
""

if(Test-Path $InstallDir) {
    "Install directory already exists, deleting."
    if($DryRun) {
        ">>> Remove-Item $InstallDir -Recurse"
        "[[[ -DryRun skipped ]]]"
        ""
    } else {
        Remove-Item $InstallDir -Recurse
    }
}
if($DryRun) {
    "New-Item $InstallDir -ItemType `"Directory`" | Out-Null"
    "[[[ -DryRun skipped ]]]"
    ""
} else {
    New-Item $InstallDir -ItemType "Directory" | Out-Null
}

">>> & $CMake `
    --install $OutDir `
    --prefix $InstallDir `
    --config $BuildConfig | MyLog"
if($DryRun) {
    "[[[ -DryRun skipped ]]]"
} else {
    & $CMake `
        --install $OutDir `
        --prefix $InstallDir `
        --config $BuildConfig | MyLog
    if($LASTEXITCODE -ne 0)
    {
        ""
        "Installing failed, now exiting."
        exit
    }
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
