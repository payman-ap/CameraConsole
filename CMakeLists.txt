cmake_minimum_required(VERSION 3.19)
project(camera_console LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ALSA REQUIRED alsa)

qt_standard_project_setup()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/forms
)

qt_add_executable(camera_console
    WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/mainwindow.cpp
    include/mainwindow.h
    src/camera_pipeline.cpp
    include/camera_pipeline.h
    src/audio_player.cpp
    src/audio_recorder.cpp
    src/audio_pipeline.cpp
    include/audio_player.hpp
    include/audio_recorder.hpp
    include/capture_thread.hpp
    include/playback_thread.hpp
    include/ring_buffer.hpp
    include/audio_pipeline.hpp
    forms/mainwindow.ui
)

target_include_directories(camera_console PRIVATE
    include
    ${OpenCV_INCLUDE_DIRS}
    ${ALSA_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(camera_console
    PRIVATE
    Qt::Core
    Qt::Widgets
    ${OpenCV_LIBS}
    ${ALSA_LIBRARIES}
)

install(TARGETS camera_console
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET camera_console
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
