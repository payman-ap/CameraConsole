cmake_minimum_required(VERSION 3.19)
project(intercom LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/forms
)

qt_add_executable(intercom
    WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/mainwindow.cpp
    include/mainwindow.h
    forms/mainwindow.ui
)

target_include_directories(intercom PRIVATE
    include
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(intercom
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS intercom
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET intercom
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
