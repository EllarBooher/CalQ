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

qt_add_library(CalQMath
  STATIC
  src/math/mathinterpreter.h     src/math/mathinterpreter.cpp
  src/math/mathfunction.h        src/math/mathfunction.cpp
  src/math/mathstatement.h       src/math/mathstatement.cpp
  src/math/backend/number.h      src/math/backend/number.cpp
  src/math/backend/numberimpl.h
  src/math/backend/functions.h   src/math/backend/functions.cpp
  src/math/lexer.h               src/math/lexer.cpp
  src/math/parser.h              src/math/parser.cpp
)
set_property(TARGET CalQMath PROPERTY CXX_STANDARD 23)
target_include_directories(CalQMath SYSTEM PUBLIC ${vendor_include_dir})
target_link_libraries(CalQMath PUBLIC MPFR)

qt_add_executable(CalQ
  WIN32 MACOSX_BUNDLE
  src/main.cpp
  src/mainwindow.cpp src/mainwindow.h src/mainwindow.ui
)
set_property(TARGET CalQ PROPERTY CXX_STANDARD 23)
target_link_libraries(CalQ
  PRIVATE
  Qt::Core
  Qt::Widgets
  CalQMath
)

qt_add_executable(CalQTest
  WIN32 MACOSX_BUNDLE
  src/math/mathtest.cpp
)
set_property(TARGET CalQTest PROPERTY CXX_STANDARD 23)
target_link_libraries(CalQTest
  PRIVATE
  Qt::Core
  Qt::Test
  CalQMath
)

include(clang-tidy)
if(CLANG_TIDY)
  set_target_properties(
    CalQ CalQMath # CalQTest
    PROPERTIES
    CXX_CLANG_TIDY "${CLANG_TIDY}"
  )
endif()

include(GNUInstallDirs)

install(TARGETS CalQ CalQTest
  BUNDLE  DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
  TARGET CalQ
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
