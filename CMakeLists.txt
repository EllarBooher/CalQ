cmake_minimum_required(VERSION 3.20)
project(CalQ LANGUAGES CXX)

################################################################################
# USAGE
################################################################################
#
# 1) When using vcpkg preset, set environment variable VCPKG_ROOT to the root
#    of your vcpkg installation.
# 2) Make sure the QT_DIR cmake scripts folder with "Qt6Config.cmake" is in your
#    env's PATH or otherwise discoverable by CMake. Optionally, pass in this
#    folder to the CMake variable QT_DIR.
#    For example, if the folder is "/home/user/qt/6.9.0/gcc_64/lib/cmake/Qt6"
#    then invoke CMake with "-DQT_DIR=/home/user/qt/6.9.0/gcc_64/lib/cmake/Qt6"
#
# Optional steps
# a) Make sure CLANG_TIDY is on your PATH in a way that can be discovered by
#    CMake. Alternatively, once configured, set the CLANG_TIDY cache variable
#    to the path of your desired clang-tidy executable, and set CLANG_TIDY_ENABLE
#    to ON.
#
################################################################################

# Prefix a prioritized QT install
# TODO: do we need to also add specific modules e.g. QtWidgets to the path?
if(DEFINED QT_DIR)
  message("Using explicit Qt installation at: ${QT_DIR}")
  list(PREPEND CMAKE_PREFIX_PATH "${QT_DIR}")
endif()
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
  src/interpreter/mathinterpreter.h     src/interpreter/mathinterpreter.cpp
  src/interpreter/mathfunction.h        src/interpreter/mathfunction.cpp
  src/interpreter/mathstatement.h       src/interpreter/mathstatement.cpp
  src/interpreter/lexer.h               src/interpreter/lexer.cpp
  src/interpreter/parser.h              src/interpreter/parser.cpp
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
  WIN32 MACOSX_BUNDLE
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
  NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

qt_generate_deploy_app_script(
  TARGET CalQTest
  OUTPUT_SCRIPT deploy_script
  NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
