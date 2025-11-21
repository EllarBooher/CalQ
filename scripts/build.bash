#!/bin/bash

usage="
Builds CalQ. Assumes Linux conventions, things may break if using WSL on Windows.

usage:
    $(basename "$0") -h | --help
    $(basename "$0") --qt-dir=<qt directory> --cmake=<cmake binary> --cmake-preset=<cmake preset> --vcpkg-root=<vcpkg root>
               [--out-dir=<output directory> --src-dir=<source directory> --build-config=<build config> --cmake-prefix=<list of paths>]
               [--force-clean]

options:
    -h --help                      Displays this information.

    --cmake         Path to CMake binary to use in operations. Consider using the copy of CMake you can install with Qt.

    --build-config  Which build config to use, e.g. Debug. Passed to CMake with '--config'. [ Default: 'Release' ]

    --cmake-preset  Which CMake preset to use, specified in CMakePresets.json or CMakeUserPresets.json.
                    Passed to CMake with '--preset'.

    --qt-dir        Root directory of a specific Qt installation's CMake config scripts.

    --out-dir       The root directory to use as build and install trees. [ Default: './build/<preset>' ]

    --src-dir       The source directory, passed to CMake. [ Default: '.' ]

    --vcpkg-root    The vcpkg installation to use. Sets environment VCPKG_ROOT to the passed directory,
                    as required by our default CMake presets.

    --cmake-prefix  Sets CMAKE_PREFIX_PATH. Set to a semicolon separated list of directories/binaries to prioritize searching in for tooling.

    --force-clean   If set and the build tree already exists, it will be wiped before building.
                    Mutually exclusive with --parallel.

    --wsl-interop   If set, CMake is treated as a Windows binary that expects Windows paths (e.g., 'C:/' instead of '/mnt/c').
                    If that is the case, BUILD_DIR and INSTALL_DIR are converted using 'wslpath' before being passed to the tooling.

    --parallel      If set, the console will not be prompted. For example, this avoids dangerous deletions that need a prompt.
                    Also copies the source tree to a seperate folder next to the build tree to allow building in parallel.
                    Mutually exclusive with --force-clean. When this option is set, the folder at --out-dir is required to be empty.

examples:
    Build in Powershell via WSL installed bash:
        bash ./scripts/build.bash --out-dir ./build/msvc/ --qt-dir C:/Qt/6.9.1/msvc2022_64 --cmake C:/Qt/Tools/CMake_64/bin/cmake.exe --cmake-preset x64-windows-msvc --vcpkg-root C:/vcpkg --wsl-interop --force-clean

        bash ./scripts/build.bash --out-dir ./build/mingw --qt-dir C:/Qt/6.9.1/mingw_64 --cmake C:/Qt/Tools/CMake_64/bin/cmake.exe --cmake-preset x64-windows-mingw --vcpkg-root C:/vcpkg --cmake-prefix C:/Qt/Tools/mingw1310_64/bin --wsl-interop  --force-clean

    Build in native Linux (tested on WSL):
        ./scripts/build.bash --out-dir ./build/linux --qt-dir /opt/qt/6.9.1/gcc_64 --cmake /opt/qt/Tools/CMake/bin/cmake --cmake-preset x64-linux --vcpkg-root /opt/vcpkg --force-clean
"

TEMP=$(getopt -l 'out-dir:,src-dir:,qt-dir:,cmake:,build-config:,cmake-preset:,vcpkg-root:,cmake-prefix:,force-clean,clean,help,wsl-interop,parallel' -n 'build.bash' -- "h" "$@")

if [ $? -ne 0 ]; then
	echo 'Terminating...' >&2
	exit 1
fi

eval set -- "$TEMP"
unset TEMP

PRESET=""
OUT_DIR="./build/$PRESET"
SRC_DIR="."
QT_DIR=""
CMAKE=""
PATH_PREFIX=""
BUILD_CONFIG="Release"
FORCE_CLEAN=false
WSL_INTEROP=false
PARALLEL=false

while true; do
    case "$1" in
    '--out-dir')
        if [ "$2" != "" ]; then
            OUT_DIR="$2"
        fi
        shift 2
        continue
    ;;
    '--src-dir')
        if [ "$2" != "" ]; then
            SRC_DIR="$2"
        fi
        shift 2
        continue
    ;;
    '--qt-dir')
        QT_DIR="$2"
        shift 2
        continue
    ;;
    '--cmake')
        CMAKE="$2"
        shift 2
        continue
    ;;
    '--build-config')
        BUILD_CONFIG="$2"
        shift 2
        continue
    ;;
    '--cmake-preset')
        PRESET="$2"
        shift 2
        continue
    ;;
    '--vcpkg-root')
        export VCPKG_ROOT="${2/#\~/${HOME}}"
        shift 2
        continue
    ;;
    '--cmake-prefix')
        PATH_PREFIX="$2"
        shift 2
        continue
    ;;
    '--force-clean')
        FORCE_CLEAN=true
        shift
        continue
    ;;
    '-h'|'--help')
        echo "$usage"
        exit
    ;;
    '--wsl-interop')
        WSL_INTEROP=true
        shift
        continue
    ;;
    '--parallel')
        PARALLEL=true
        shift
        continue
    ;;
    '--')
        shift
        break
    ;;
    *)
    # Unimplemented flag fallthrough
        echo "$1"
        echo 'Internal error!' >&2
        exit 1
    ;;
    esac
done

if [ $# -ne 0 ]; then
    echo "Additional arguments supplied, these are ignored: $@"
fi

if [ "$QT_DIR" = "" ]; then
    echo "Error: Required argument qt-dir is empty or missing. Try passing it as --qt-dir. Terminating..."
    exit 1
fi
if [ "$CMAKE" = "" ]; then
    echo "Error: Required argument cmake is empty or missing. Try passing it as --cmake. Terminating..."
    exit 1
fi
if [ "$VCPKG_ROOT" = "" ]; then
    echo "Error: VCPKG_ROOT not set. Try passing it as --vcpkg-root. Terminating..."
    exit 1
fi
if [ $FORCE_CLEAN = true ] && [ $PARALLEL = true ]; then
    echo "Error: Mutually exclusive --force-clean and --parallel options were both set. Remove one before running again."
    exit 1
fi

mkdir -p "$OUT_DIR"
OUT_DIR=$(realpath -qL "$OUT_DIR")
if [ $? -ne 0 ]; then
    echo "Error: OUT_DIR=$OUT_DIR is invalid, terminating..."
    exit 1
fi

SRC_DIR=$(realpath -qL "$SRC_DIR")
if [ $? -ne 0 ]; then
    echo "Error: SRC_DIR=$SRC_DIR is invalid, terminating..."
    exit 1
fi

# These paths need to stay relative to OUT_DIR.
BUILD_DIR="$OUT_DIR/$BUILD_CONFIG/build"
INSTALL_DIR="$OUT_DIR/$BUILD_CONFIG/install"

if command -v wslpath >/dev/null 2>&1; then
    realpath -qL "$CMAKE"
    if [ $? -ne 0 ]; then
        CMAKE=$(wslpath "$CMAKE")
    fi
fi

CMAKE_PREFIX_PATH="$QT_DIR;$PATH_PREFIX"

echo "Using:
    Source Directory   : $SRC_DIR
    Build Directory    : $BUILD_DIR
    Install Directory  : $INSTALL_DIR
    Build Config       : $BUILD_CONFIG
    Build Preset       : $PRESET
    CMake Binary       : $CMAKE
    PATH               : $PATH
    VCPKG_ROOT         : $VCPKG_ROOT
    CMAKE_PREFIX_PATH  : $CMAKE_PREFIX_PATH
"

if [ $PARALLEL = true ]; then
    echo "
*************** COPY ****************
"

    if [ -n "$(ls -A "$OUT_DIR")" ]; then
        echo "Error: --parallel is set, and OUT_DIR=$OUT_DIR is not empty, terminating..."
        exit 1
    fi


    SRC_DIR_temp="$OUT_DIR/$BUILD_CONFIG/source"
    echo "Copying source into '$SRC_DIR_temp'..."

    mkdir -p "$SRC_DIR_temp"
    cp -r "$SRC_DIR/src" "$SRC_DIR/cmake" .clang-tidy CMakeLists.txt CMakePresets.json resources.qrc vcpkg.json "$SRC_DIR_temp"
    SRC_DIR="$SRC_DIR_temp"

    mkdir -p $BUILD_DIR
    mkdir -p $INSTALL_DIR
else
    mkdir -p $BUILD_DIR
    mkdir -p $INSTALL_DIR

    echo "
*************** CLEAN ***************
"
    if [ -n "$(ls -A "$BUILD_DIR")" ]; then

    if [ $FORCE_CLEAN = true ]; then
        echo "--force-clean enabled, deleting output \"$BUILD_DIR\"."
        rm -rfI "$BUILD_DIR"
    elif [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
        echo ">>> $CMAKE \\
            --build $BUILD_DIR \\
            --target clean"
        $CMAKE --build $BUILD_DIR --target clean
        # swallow clean result since it shouldn't matter
    else
        echo "A build tree exists. However, --force-clean was not set and there is no CMakeCache.txt to run clean target for, indicating an unintended out directory was possibly passed. Terminating..."
        exit 1
    fi

    else
        echo "Out directory is empty, no cleaning to do."
    fi

fi

if command -v wslpath >/dev/null 2>&1; then
    if [ $WSL_INTEROP = true ]; then
        SRC_DIR=$(wslpath -w "$SRC_DIR")
        BUILD_DIR=$(wslpath -w "$BUILD_DIR")
        INSTALL_DIR=$(wslpath -w "$INSTALL_DIR")
    fi
fi

echo "
************* CONFIGURE *************
"

echo ">>> $CMAKE \\
    -S $SRC_DIR \\
    -B $BUILD_DIR \\
    --preset $PRESET \\
    -DCMAKE_PREFIX_PATH:STRING=\"$CMAKE_PREFIX_PATH\""

$CMAKE -S $SRC_DIR -B $BUILD_DIR --preset $PRESET -DCMAKE_PREFIX_PATH:STRING=$CMAKE_PREFIX_PATH

if [ $? -ne 0 ]; then
    echo "Configuring failed, terminating..."
    exit 1
fi

echo "
*************** BUILD ***************
"

echo ">>> $CMAKE \\
    --build $BUILD_DIR \\
    --config $BUILD_CONFIG"

$CMAKE --build $BUILD_DIR --config $BUILD_CONFIG

if [ $? -ne 0 ]; then
    echo "Building failed, terminating..."
    exit 1
fi

echo "
************** INSTALL **************
"

if [ -n "$(ls -A "$INSTALL_DIR")" ]; then
    if [ $PARALLEL = true ]; then
        # This shouldn't happen because we require empty earlier, but just in case
        echo "Error: INSTALL_DIR had files but --parallel was set."
        exit 1
    fi

    echo "Install directory already exists, deleting \"$INSTALL_DIR\"."

    rm -rfI "$INSTALL_DIR"
fi 

echo ">>> $CMAKE \\
    --install $BUILD_DIR \\
    --prefix $INSTALL_DIR \\
    --config $BUILD_CONFIG"

$CMAKE --install $BUILD_DIR --prefix $INSTALL_DIR --config $BUILD_CONFIG

if [ $? -ne 0 ]; then
    echo "Installing failed, terminating..."
    exit 1
fi

echo "
*************************************

Success. Check the following locations:
    Build: '$BUILD_DIR'
    Install: '$INSTALL_DIR'
"
