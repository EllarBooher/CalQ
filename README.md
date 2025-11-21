# CalQ

CalQ is a scientific calculator made in C++ with Qt6 and QtWidgets.
I started this project since I wanted a quick and simple scientific calculator built for my personal use,
and I saw it as a good opportunity to practice creating a graphical application in Qt which I had not done before.

## Features

- Fixed but very high precision calculations
- Interprets user text input as a mathematical expression and processes into a numeric result
- Log of previous results and their input strings
- Various unary elementary functions such as `sin`, `pow`, `log`, etc

### Planned

- Binary functions
- Help manual
- Tables
- Graphing

## Building

### Prerequisites and dependencies

- Qt 6.9.1+ with QtCore, QtTest, QtWidgets, QtGraphs, and all their required dependencies preinstalled
- CMake 3.21+
- vcpkg
- C++ compiler supporting C++23

Supported 64-bit x86 platforms, listed with operating system and Qt installation used for testing and development:

- Linux + gcc
    - Ubuntu 24.04 on WSL
    - Qt 6.9.1 GCC 10.3.1
- MinGW + clang
    - Windows 11
    - Qt 6.9.1 LLVM 17.0.6 MinGW
- MSVC
    - Windows 11
    - Qt 6.9.1 MSVC 2022

### Notes

Qt provides a gcc-compiled MinGW binary, but this one utilizes an out-of-date DirectX 12 header (d3d12.h) that prints warnings at runtime. 
Otherwise, CalQ has no known restrictions on which MinGW platform to use.

At runtime, the GUI application may log a warning about a missing QPA platform plugin.
As long as the app starts, this can be safely ignored since it may be checking for others too.
At runtime, environment variable `QT_QPA_PLATFORM_PLUGIN_PATH` is used as a hint for where to search,
and `QT_QPA_PLATFORM` controls a list of suitable platform plugins to use e.g. `xcb;wayland`.
The `QT_QPA_DEFAULT_PLATFORM` environment/CMake cache variable also exists for configuring the platform at build time.
If you have a specific platform you wish to use, see https://doc.qt.io/qt-6/qpa.html for the list of possible plugins that you need to install.

Ahead, you can specify an install prefix like `cmake --install ./build/dir --prefix /install/dir`. 
If you don't, then the Qt deploy scripts will install to an absolute root (For example something like C:/ProgramFiles on Windows). If specified, `--install` needs to be absolute.

### Building from Commandline

QtCreator handles the build environment automatically and supports loading CMake presets and pre-built trees, but you may wish to compile manually. 
See the `scripts/build.bash` for a script that takes in all the required parameters. Use `build.bash --help` for descriptions of the parameters and some examples.

Otherwise, first clone the repo:

```bash
git clone https://github.com/EllarBooher/CalQ
```

Then, navigate to the project root:

```bash
cd /path/to/repo/CalQ
```

Follow whichever section ahead matches your OS.

### Linux

The `x64-linux` preset uses `Ninja` and `gcc` by default. 
Ensure they are on PATH alongside your Qt installation, perhaps as follows. Replace any paths as needed.

```bash
export PATH="/usr/bin:/path/to/Qt/6.9.1/gcc_64:$PATH"
export VCPKG_ROOT="/opt/vcpkg/"
```

To build and install, run CMake as follows, replacing `<build directory>` and `<install directory>` with whatever you want:

```bash
# configure
cmake -B "<build directory>" --preset "x64-linux"
# build
cmake --build "<build directory>" --config Debug
# install
cmake --install "<build directory>" --prefix "<install directory>" --config Debug
```

Installed binaries will be in `<install directory>/<build config>/bin`.

### Windows

Ensure Qt, your generator (Ninja is used by default for `x64-windows-mingw` preset), and compilers are on `PATH`, perhaps as follows. Replace any paths as needed.

```powershell
# For MinGW
#     Folders with Qt install, MinGW binaries, and Ninja
$env:PATH = "C:\Qt\6.9.1\llvm-mingw_64;C:\Qt\Tools\llvm-mingw1706_64\bin;C:\Qt\Tools\Ninja;$env:PATH"

# For MSVC
#     Assuming MSVC and MSBuild are on PATH, just need to specify Qt
$env:PATH = "C:\Qt\6.9.1\msvc2022_64;$env:PATH"

# For both, replace with path to vcpkg
$env:VCPKG_ROOT="C:\vcpkg"
```

To build and install, run CMake as follows, replacing `./build/x64-windows-mingw` with whatever output directory you'd like:

```powershell
# configure
cmake -B "<build directory>" --preset "x64-windows-mingw" # or x64-windows-msvc
# build
cmake --build "<build directory>" --config Debug
# install
cmake --install "<build directory>" --prefix "<install directory>" --config Debug
```

Installed binaries will be in `<install directory>/<build config>/bin`.

## Dependencies

- [Qt](https://www.qt.io/) is available under LGPLv3
- [MPFR](https://www.mpfr.org/) is available under LGPLv3
- [GMP](https://gmplib.org/) is available under LGPLv3
