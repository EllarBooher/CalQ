cmake_minimum_required(VERSION 3.20)
project(SciCalc LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Test)

qt_standard_project_setup()

enable_testing(true)

qt_add_library(
    SciCalcMath
    math/mathinterpreter.h math/mathinterpreter.cpp
)
set_property(TARGET SciCalcMath PROPERTY CXX_STANDARD 23)

qt_add_executable(SciCalc
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)
set_property(TARGET SciCalc PROPERTY CXX_STANDARD 23)
target_link_libraries(SciCalc
    PRIVATE
        Qt::Core
        Qt::Widgets
        SciCalcMath
)

qt_add_executable(SciCalcTest
    WIN32 MACOSX_BUNDLE
    math/mathtest.cpp
)
set_property(TARGET SciCalcTest PROPERTY CXX_STANDARD 23)
target_link_libraries(SciCalcTest
    PRIVATE
        Qt::Core
        Qt::Test
        SciCalcMath
)

include(GNUInstallDirs)

install(TARGETS SciCalc SciCalcTest
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET SciCalc
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

qt_generate_deploy_app_script(
    TARGET SciCalcTest
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
