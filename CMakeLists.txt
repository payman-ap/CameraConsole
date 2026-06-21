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
    # Core
    src/main.cpp
    # MainWindow core
    src/ui/mainwindow.cpp
    # UI modules
    src/ui/mainwindow_audio.cpp
    src/ui/mainwindow_video.cpp
    src/ui/mainwindow_render.cpp
    src/ui/mainwindow_stats.cpp
    src/ui/mainwindow_styles.cpp
    src/ui/pages/audio_settings_page.cpp
    src/ui/pages/video_settings_page.cpp
    # Utilities
    src/ui/qt_utils.cpp
    # Backend
    src/video/camera_pipeline.cpp
    src/video/video_device_manager.cpp
    src/audio/audio_player.cpp
    src/audio/audio_recorder.cpp
    src/audio/audio_pipeline.cpp
    src/audio/audio_device_manager.cpp
    # Includes for UI
    include/ui/mainwindow.h
    include/video/camera_pipeline.h
    include/audio/audio_player.hpp
    include/audio/audio_recorder.hpp
    include/video/capture_thread.hpp
    include/audio/playback_thread.hpp
    include/audio/ring_buffer.hpp
    include/audio/audio_pipeline.hpp
    include/audio/audio_device_manager.hpp
    include/video/video_device_manager.hpp
    include/ui/mainwindow_audio.hpp
    include/ui/mainwindow_video.hpp
    include/ui/mainwindow_styles.hpp
    include/ui/qt_utils.hpp
    include/ui/pages/audio_settings_page.hpp
    include/ui/pages/video_settings_page.hpp
    # UI designer
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
