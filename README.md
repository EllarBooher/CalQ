# CalQ

CalQ is a scientific calculator made in C++ with Qt6 and QtWidgets.
I started this project since I wanted a quick and simple scientific calculator built for my personal use,
and I saw it as a good opportunity to practice creating a graphical application in Qt which I had not done before.

## Features

- Fixed but very high precision caclulations, using arbitrary precision numbers
  - Uses GNU's [MPFR](https://www.mpfr.org/) which is build on [GMP](https://gmplib.org/)
- Interprets user text input as a mathematical expression and processes into a numeric result
- Log of previous results and their input strings
- Various unary elementary functions such as `sin`, `pow`, `log`, etc

### Planned

- Binary functions
- Help manual
- Tables
- Graphing

## Building

### Prerequisites

Version numbers are the versions I have built and tested on, not necessarily the absolute minimum possible.

- Qt 6.9.0
- Possibly QtWayland for Qt6 on Ubuntu
  - This was an issue for my WSL Ubuntu install, package is named `qt6-wayland`. I need to investigate the exact requirement further.
- CMake 3.20 (Minimum is explicitly required)
- vcpkg 2025-04-16
- C++ compiler supporting C++23

Supported platforms

- Linux with gcc (Ubuntu 24.04 on WSL)
- MinGW with gcc (Windows 11, using MinGW 13.1.0 optionally installed with Qt)
- MSVC (Windows 11, using Visual Studio Community 2022 17.13.6)

### Building from Commandline

QtCreator does this process automatically once you load one of the CMake presets, but you may wish to compile manually. See the `scripts` folders for scripts that take in all the required parameters and handle the build environment. Use `Get-Help build.ps1` or `build.bash --help` for descriptions of the parameters.

First, clone the repo:

```bash
git clone https://github.com/EllarBooher/CalQ
```

Then, navigate to the project root:

```bash
cd /path/to/repo/CalQ
```

A quick note: Ahead, you can specify an install prefix like `cmake --install ./build/dir --prefix /install/dir`. If you don't, then the Qt deploy scripts will install to an absolute root (For example something like C:/ProgramFiles on Windows). I found this path needed to be absolute, I'm not sure if this is an issue with Qt's CMake module or CMake itself.

### Linux

Ensure Qt, your generator (Ninja is used by default for `x64-linux` preset), and gcc are on `PATH`, like follows.
Also, set environment `VCPKG_ROOT` to your install of vcpkg.
Qt6 is likely on your `PATH` if installed normally, but I used [aqtinstall](https://github.com/miurahr/aqtinstall) so I needed to manually specify it. In bash:

```bash
# Assume gcc and ninja installed to /usr/bin, replace folders as needed.
export PATH="/usr/bin:/path/to/Qt/6.9.0/gcc_64:$PATH"
export VCPKG_ROOT="/opt/vcpkg/"
```

To build and install, run CMake as follows, replacing `./build/x64-linux` with whatever output directory you'd like:

```bash
# configure
cmake -B "./build/x64-linux" --preset "x64-linux"
# build
cmake --build "./build/x64-linux" --config Debug
# install
cmake --install "./build/x64-linux" --prefix "/absolute/path/to/install/directory/" --config Debug
# Installed binaries will be in /absolute/path/to/install/directory/%CONFIG%/bin
```

### Windows

Ensure Qt, your generator (Ninja is used by default for `x64-windows-mingw` preset), and compilers are on `PATH`, like follows.
Also, set environment `VCPKG_ROOT` to your install of vcpkg. In powershell:

```powershell
# For MinGW: Folders with Qt install, MinGW binaries, and Ninja
$env:PATH = "C:\Qt\6.9.0\mingw_64;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;$env:PATH"
# For MSVC: Assuming MSVC and MSBuild are on PATH, just need to specify Qt
$env:PATH = "C:\Qt\6.9.0\msvc2022_64;$env:PATH"

# For both, replace with path to vcpkg
$env:VCPKG_ROOT="C:\vcpkg"
```

To build and install, run CMake as follows, replacing `./build/x64-windows-mingw` with whatever output directory you'd like:

```powershell
# configure
cmake -B ".\build\x64-windows-mingw" --preset "x64-windows-mingw" # or x64-windows-msvc
# build
cmake --build ".\build\x64-windows-mingw" --config Debug
# install
cmake --install ".\build\x64-windows-mingw" --prefix "C:\absolute\path\to\install\directory\" --config Debug
# Installed binaries will be in /absolute/path/to/install/directory/%CONFIG%/bin
```
