cmake_minimum_required(VERSION 3.20)
project(CalQ LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Test)

set(CMAKE_CXX_FLAGS "-Werror")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

qt_standard_project_setup()

enable_testing(true)

qt_add_library(
    CalQMath
    math/mathinterpreter.h math/mathinterpreter.cpp
)
set_property(TARGET CalQMath PROPERTY CXX_STANDARD 23)

qt_add_executable(CalQ
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
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
    math/mathtest.cpp
)
set_property(TARGET CalQTest PROPERTY CXX_STANDARD 23)
target_link_libraries(CalQTest
    PRIVATE
        Qt::Core
        Qt::Test
        CalQMath
)

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

include(cmake/clang-tidy.cmake)
set_target_properties(
    CalQ CalQTest CalQMath
    PROPERTIES
        CXX_CLANG_TIDY
            "${CLANG_TIDY_PATH}"
)
