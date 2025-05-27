#!/bin/bash

usage="
Builds CalQ targeting x64 Linux. This script was developed and tested on WSL running Ubuntu 24.04.2 LTS.

usage:
    $(basename "$0") -h | --help
    $(basename "$0") --qt-dir=<qt directory> --cmake=<cmake binary> --build-config=<build config> 
               [--out-dir=<output directory> --src-dir=<source directory>] 
               [--force-clean --dry-run]

options:
    -h --help                      Displays this information.
    --qt-dir=<qt directory>        Root directory of a specific Qt installation, e.g. "/some/path/Qt/6.9.0/".
    --cmake=<cmake binary>         Path to CMake to use in operations. Consider using the copy of CMake you can install with Qt.
    --build-config=<build config>  Which build config to use, e.g. Debug. Passed to CMake as --config \$BUILD_CONFIG
    --out-dir=<output directory>   The destination directory to save the build tree, passed to CMake. [ Default: './build/<build config>' ]
    --src-dir=<source directory>   The source directory, passed to CMake. [ Default: '.' ]
    --force-clean                  If set and the build tree already exists, it will be wiped before building. 
                                   In any other case, CMake clean target is executed.
    --dry-run                      If set, all commands are logged instead of executing them.

example:
    ./scripts/build.bash --qt-dir ~/qt/6.9.0 --cmake ~/qt/Tools/CMake/bin/cmake --build-config Debug
"

TEMP=$(getopt -l 'out-dir:,src-dir:,qt-dir:,cmake:,build-config:,force-clean,dry-run,help' -n 'build.bash' -- "h" "$@")

if [ $? -ne 0 ]; then
	echo 'Terminating...' >&2
	exit 1
fi

eval set -- "$TEMP"
unset TEMP

PRESET="x64-linux"
OUT_DIR="./build/$PRESET"
SRC_DIR="."
QT_DIR=""
CMAKE=""
BUILD_CONFIG=""
FORCE_CLEAN=false
DRY_RUN=false

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
        '--force-clean')
            FORCE_CLEAN=true
            shift
            continue
        ;;
        '--dry-run')
            DRY_RUN=true
            shift
            continue
        ;;
        '--quiet')
            QUIET=true
            shift
            continue
        ;;
        '-h'|'--help')
            echo "$usage"
            exit
        ;;
		'--')
			shift
			break
		;;
		*)
            # Unimplemented flag fallthrough
        	echo 'Internal error!' >&2
			exit 1
		;;
	esac
done

if [ $# -ne 0 ]; then
    echo "Additional arguments supplied, these are ignored: $@"
fi

if [ "$QT_DIR" = "" ]; then
    echo "Error: Required argument qt-dir is empty or missing, terminating..."
    exit 1
fi
if [ "$CMAKE" = "" ]; then
    echo "Error: Required argument cmake is empty or missing, terminating..."
    exit 1
fi
if [ "$BUILD_CONFIG" = "" ]; then
    echo "Error: Required argument build-config is empty or missing, terminating..."
    exit 1
fi
if [ "$VCPKG_ROOT" = "" ]; then
    echo "Error: VCPKG_ROOT not set, terminating..."
    exit 1
fi

mkdir -p "$OUT_DIR"
OUT_DIR=$(realpath -qL "$OUT_DIR")
if [ $? -ne 0 ]; then
    echo "OUT_DIR=$OUT_DIR is invalid, terminating..."
    exit 1
fi
SRC_DIR=$(realpath -qL "$SRC_DIR")
if [ $? -ne 0 ]; then
    echo "SRC_DIR=$SRC_DIR is invalid, terminating..."
    exit 1
fi
QT_DIR=$(realpath -qL "$QT_DIR/gcc_64")
if [ $? -ne 0 ]; then
    echo "QT_DIR=$QT_DIR is invalid, terminating..."
    exit 1
elif [ ! -d "$QT_DIR" ]; then
    echo "Qt install directory at '$QT_DIR' does not exist, terminating..."
    exit 1
fi
CMAKE=$(realpath -q -L "$CMAKE")
if [ $? -ne 0 ]; then
    echo "CMAKE=$CMAKE is an invalid path, terminating..."
    exit 1
elif [ ! -f "$CMAKE" ]; then
    echo "CMake binary at '$CMAKE' does not exist, terminating..."
    exit 1
fi

INSTALL_DIR="$OUT_DIR/deploy/$BUILD_CONFIG"
export PATH="$QT_DIR:$PATH"

echo "Using:
    Source Directory   : $SRC_DIR
    Build Directory    : $OUT_DIR
    Install Directory  : $INSTALL_DIR
    Build Config       : $BUILD_CONFIG
    Qt                 : $QT_DIR
    Qt CMake           : $CMAKE
    PATH               : $PATH
"

echo "
*************** CLEAN ***************
"
if [ $FORCE_CLEAN = true ]; then
    echo "--force-clean enabled, deleting output \"$OUT_DIR\"."
    if [ $DRY_RUN = true ]; then 
        echo "[[[ -DryRun skipped ]]] rm -rfI \"$OUT_DIR\""
    else
        rm -rfI "$OUT_DIR"
    fi
else
    echo ">>> $CMAKE \\
        --build $OUT_DIR \\
        --target clean"
    if [ $DRY_RUN = true ]; then 
        echo "[[[ -DryRun skipped ]]]"
    else
        $CMAKE --build $OUT_DIR --target clean
        # swallow clean result since it shouldn't matter
    fi
fi

echo "
************* CONFIGURE *************
"

echo ">>> & $CMAKE \\
    -S $SRC_DIR \\
    -B $OUT_DIR \\
    --preset $PRESET \\"
if [ $DRY_RUN = true ]; then 
    echo "[[[ -DryRun skipped ]]]"
else
    $CMAKE -S $SRC_DIR -B $OUT_DIR --preset $PRESET
fi
if [ $? -ne 0 ]; then
    echo "
    Configuring failed, deleting \"$OUT_DIR\"."
    if [ $DRY_RUN = true ]; then
        echo "[-DryRun] rm -rfI \"$OUT_DIR\""
    else
        rm -rfI "$OUT_DIR"
    fi
    exit 1
fi

echo "
*************** BUILD ***************
"

echo ">>> $CMAKE \\
    --build $OUT_DIR \\
    --config $BUILD_CONFIG \\"
if [ $DRY_RUN = true ]; then
    echo "[[[ -DryRun skipped ]]]"
else
    $CMAKE --build $OUT_DIR --config $BUILD_CONFIG
    if [ $? -ne 0 ]; then
        echo "
        Building failed, now exiting."
        exit 1
    fi
fi

echo "
************** INSTALL **************
"

if [ -d $INSTALL_DIR ]; then
    echo "Install directory already exists, deleting \"$INSTALL_DIR\"."
    if [ $DRY_RUN = true ]; then
        echo "[[[ -DryRun skipped ]]] rm -rfI \"$INSTALL_DIR\""
    else
        rm -rfI "$INSTALL_DIR"
    fi
fi 
if [ $DRY_RUN = true ]; then
    echo "[[[ -DryRun skipped ]]] mkdir -p $INSTALL_DIR"
else
    mkdir -p $INSTALL_DIR
fi

echo ">>> & $CMAKE \\
    --install $OUT_DIR \\
    --prefix $INSTALL_DIR \\
    --config $BUILD_CONFIG"
if [ $DRY_RUN = true ]; then
    echo "[[[ -DryRun skipped ]]]"
else
    $CMAKE --install $OUT_DIR --prefix $INSTALL_DIR --config $BUILD_CONFIG
    if [ $? -ne 0 ]; then
        echo "Installing failed, terminating..."
        exit 1
    fi    
fi

echo "
*************************************

Success. Check the following locations:
    Build: '$OUT_DIR'
    Install: '$INSTALL_DIR'
"
