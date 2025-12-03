cmake_minimum_required(VERSION 3.21)
project(CalQ LANGUAGES CXX)

################################################################################
# USAGE
################################################################################
#
# See ./scripts/ for interactive powershell scripts that assist with building,
# and README.md for a walkthrough of building with CMake manually.
#
################################################################################

# Load our FindGMP script for find_package
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Widgets Quick Test OpenGLWidgets)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

set(MPFR_USE_STATIC_LIBS ON)
find_package(MPFR REQUIRED)
set(vendor_include_dir "${CMAKE_SOURCE_DIR}/vendor")

qt_standard_project_setup()

# Use Qt resources system to avoid having to track where .qml files go
qt_add_resources(RCC_SOURCES resources.qrc)

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
  src/app/calqgraph.h src/app/calqgraph.cpp
)
set_target_properties(CalQApp PROPERTIES CXX_STANDARD 23 OUTPUT_NAME CalQ)
target_include_directories(CalQApp PRIVATE src/)
target_link_libraries(CalQApp PRIVATE Qt::Core Qt::Widgets Qt::OpenGLWidgets CalQInterpreter )

################################################################################
# CalQTest / CalQBenchmark
#   Unit testing for all CalQ targets
################################################################################

qt_add_executable(CalQTest
  src/test/test.cpp
)
qt_add_executable(CalQBenchmark
  src/test/benchmark.cpp
)

set_target_properties(CalQTest PROPERTIES CXX_STANDARD 23)
target_link_libraries(CalQTest PRIVATE Qt::Core Qt::Test CalQInterpreter CalQMath)
target_include_directories(CalQTest PRIVATE src/)

set_target_properties(CalQBenchmark PROPERTIES CXX_STANDARD 23)
target_link_libraries(CalQBenchmark PRIVATE Qt::Core Qt::Test CalQInterpreter CalQMath)
target_include_directories(CalQBenchmark PRIVATE src/)

################################################################################

option(CLANG_TIDY_ENABLE "Enable clang-tidy - this impacts compilation time." OFF)
include(clang-tidy)
if(CLANG_TIDY AND CLANG_TIDY_ENABLE)
  cmake_path(APPEND clang_tidy_config_path "${CMAKE_SOURCE_DIR}" ".clang-tidy")
  message(STATUS "Enabling clang-tidy. Using: ${CLANG_TIDY}. Config at: ${clang_tidy_config_path}")
  set_target_properties(
    CalQMath CalQInterpreter CalQApp
    PROPERTIES
    CXX_CLANG_TIDY "${CLANG_TIDY};--config-file=${clang_tidy_config_path};--header-filter=^.*\/src\/.*$"
  )
endif()

include(GNUInstallDirs)

install(TARGETS CalQApp CalQTest CalQBenchmark
  BUNDLE  DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Patch in compiler DLLs for Clang on Windows, obtained via LLVM MinGW install.
# windeployqt only explicitely supports MSVC/g++ as of 6.9.1.
if((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND WIN32)
  message(STATUS "Installing DLLs for Clang.")
  get_filename_component(llvm_path ${CMAKE_CXX_COMPILER} PATH)
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${llvm_path}/libc++.dll ${llvm_path}/libunwind.dll)
endif()
include(InstallRequiredSystemLibraries)

qt_generate_deploy_app_script(
  TARGET CalQApp
  OUTPUT_SCRIPT deploy_script
  NO_TRANSLATIONS
)
install(SCRIPT ${deploy_script})

qt_generate_deploy_app_script(
  TARGET CalQTest
  OUTPUT_SCRIPT deploy_script
  NO_TRANSLATIONS
)
install(SCRIPT ${deploy_script})
