#!/bin/bash

# Builds all CalQ targets.
#
# Targets a dual Windows/WSL environment.
#
# Due to the complexity of interop when compiling all environments
# manually like this, assumes all Unix path conventions.
#
# Thus this script most likely needs to be ran with WSL.

exec 1>&2

set -a
source .githooks/.env
set +a

echo "Building all in parallel"
rm -rf $OutDir
mkdir -p $OutDir
parallel --progress --files --tmpdir "$OutDir" --tagstring "{4}" --link './scripts/build.bash --out-dir {1} --qt-dir {2} --cmake {3} --cmake-preset {4} --vcpkg-root {5} --cmake-prefix {6} {7} --parallel --cmake-args -DCLANG_TIDY_ENABLE:BOOL=ON' \
    ::: "$OutDir/linux" "$OutDir/msvc" "$OutDir/mingw" \
    ::: "$LinuxQtDir" "$MSVCQtDir" "$MinGWQtDir" \
    ::: "$LinuxCMake" "$WinCMake" "$WinCMake" \
    ::: x64-linux x64-windows-msvc x64-windows-mingw \
    ::: "$LinuxVCPKGRoot" "$WinVCPKGRoot" "$WinVCPKGRoot" \
    ::: "" "$MSVCCMakePrefix" "$MinGWCMakePrefix" \
    ::: "" "--wsl-interop" "--wsl-interop"
if [[ $? -ne 0 ]]; then
  echo "pre-push: build_all failed one or more."
  exit 1
fi

#./scripts/build.bash --out-dir /mnt/f/proj/CalQ/build/build_all/mingw --qt-dir 'F:/Qt/6.10.1/mingw_64' --cmake /mnt/f/Qt/Tools/CMake_64/bin/cmake.exe --cmake-preset x64-windows-mingw --vcpkg-root /mnt/f/vcpkg --cmake-prefix 'F:/Qt/Tools/mingw1310_64;F:/Qt/Tools/QtCreator/bin/clang;F:/Qt/Tools/Ninja' --wsl-interop --parallel --cmake-args -DCLANG_TIDY_ENABLE:BOOL=ON

# parallel didn't seem able to capture stdout from wsl-interop .exe's, so run them manually
# which somehow works

echo "Running CalQTest binaries
"

"$OutDir/linux/Release/install/bin/CalQTest"
TEST_LINUX_CODE=$?

echo ""

"$OutDir/mingw/Release/install/bin/CalQTest.exe"
TEST_MINGW_CODE=$?

echo ""

"$OutDir/msvc/Release/install/bin/CalQTest.exe"
TEST_MSVC_CODE=$?

echo ""

if [[ $TEST_LINUX_CODE -ne 0 || $TEST_MINGW_CODE -ne 0 || $TEST_MSVC_CODE -ne 0 ]]; then
  echo "build_all: One or more tests failed."
  exit 1
fi

echo "build_all: build and test succeeded."
exit 0
