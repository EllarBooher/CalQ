cmake_minimum_required(VERSION 3.20)
project(CalQ LANGUAGES CXX)

################################################################################
# USAGE
################################################################################
#
# See ./scripts/ for interactive powershell scripts that assist with building.
#
# 1) When using vcpkg preset, set environment variable VCPKG_ROOT to the root
#    of your vcpkg installation.
# 2) Ensure all tooling is on your PATH environment variable, depending on the
#    build environment. This includes:
#     - Qt CMake script discoverable by FindPackage
#     - Generator compatible with Qt
#     - C++ compiler
#     - powershell (as required by vcpkg)
#
#    Example of a minimal PATH variable when building for vcpkg plus MinGW:
#        PATH = C:\Qt\Tools\mingw1310_64\bin;
#               C:\Qt\Tools\Ninja;
#               C:\Qt\6.9.0\mingw_64;
#               C:\Windows\System32\WindowsPowerShell\v1.0
#
# Optional steps
# a) Make sure CLANG_TIDY is on your PATH in a way that can be discovered by
#    CMake. Alternatively, once configured, set the CLANG_TIDY cache variable
#    to the path of your desired clang-tidy executable, and set CLANG_TIDY_ENABLE
#    to ON.
#
################################################################################

# Load our FindGMP script for find_package
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Test)

set(MPFR_USE_STATIC_LIBS ON)
find_package(MPFR REQUIRED)
set(vendor_include_dir "${CMAKE_SOURCE_DIR}/vendor")

qt_standard_project_setup()

################################################################################
# CalQMath
#   Math backend with underlying opaque number type used in calculations
#   Also provides functions such as log and pow
################################################################################

qt_add_library(CalQMath
  STATIC
  src/math/number.h      src/math/number.cpp
  src/math/numberimpl.h
  src/math/functions.h   src/math/functions.cpp
)
set_target_properties(CalQMath PROPERTIES CXX_STANDARD 23)
target_include_directories(CalQMath SYSTEM PRIVATE ${vendor_include_dir})
target_include_directories(CalQMath PRIVATE src/)
target_link_libraries(CalQMath PRIVATE MPFR)

################################################################################
# CalQInterpreter
#   Interpretation of user input, to produce numeric results for the calculator
################################################################################

qt_add_library(CalQInterpreter
  STATIC
  src/interpreter/lexer.h              src/interpreter/lexer.cpp
  src/interpreter/expression.h         src/interpreter/expression.cpp
  src/interpreter/function_database.h  src/interpreter/function_database.cpp
  src/interpreter/parser.h             src/interpreter/parser.cpp
  src/interpreter/interpreter.h        src/interpreter/interpreter.cpp
)
set_target_properties(CalQInterpreter PROPERTIES CXX_STANDARD 23)
target_include_directories(CalQInterpreter PRIVATE src/)
target_link_libraries(CalQInterpreter PRIVATE CalQMath)

################################################################################
# CalQApp
#   Calculator Qt GUI frontend
################################################################################

qt_add_executable(CalQApp
  WIN32 MACOSX_BUNDLE
  src/app/main.cpp
  src/app/mainwindow.cpp src/app/mainwindow.h src/app/mainwindow.ui
)
set_target_properties(CalQApp PROPERTIES CXX_STANDARD 23 OUTPUT_NAME CalQ)
target_include_directories(CalQApp PRIVATE src/)
target_link_libraries(CalQApp PRIVATE Qt::Core Qt::Widgets CalQInterpreter)

################################################################################
# CalQTest
#   Unit testing for all CalQ targets
################################################################################

qt_add_executable(CalQTest
  src/test/mathtest.cpp
)
set_target_properties(CalQTest PROPERTIES CXX_STANDARD 23)
target_link_libraries(CalQTest PRIVATE Qt::Core Qt::Test CalQInterpreter)
target_include_directories(CalQTest PRIVATE src/)

################################################################################

option(CLANG_TIDY_ENABLE "Enable clang-tidy - this impacts compilation time." OFF)
include(clang-tidy)
if(CLANG_TIDY AND CLANG_TIDY_ENABLE)
  set_target_properties(
    CalQMath CalQInterpreter CalQApp # CalQTest
    PROPERTIES
    CXX_CLANG_TIDY "${CLANG_TIDY}"
  )
endif()

include(GNUInstallDirs)

install(TARGETS CalQApp CalQTest
  BUNDLE  DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
  TARGET CalQApp
  OUTPUT_SCRIPT deploy_script
  NO_TRANSLATIONS
  NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

qt_generate_deploy_app_script(
  TARGET CalQTest
  OUTPUT_SCRIPT deploy_script
  NO_TRANSLATIONS
  NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
