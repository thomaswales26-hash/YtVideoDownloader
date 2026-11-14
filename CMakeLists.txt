cmake_minimum_required(VERSION 3.21)
project(VideoDownloader VERSION 1.0 LANGUAGES CXX)

if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES arm64)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(VideoDownloader MACOSX_BUNDLE
    main.cpp
)
target_link_libraries(VideoDownloader PRIVATE Qt6::Widgets)

set_target_properties(VideoDownloader PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.videodownloader"
    MACOSX_BUNDLE_BUNDLE_NAME "Video Downloader"
)
