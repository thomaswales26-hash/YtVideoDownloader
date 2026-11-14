cmake_minimum_required(VERSION 3.21)
project(VMStudio VERSION 1.0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(VMStudio MACOSX_BUNDLE
    main.cpp
)
target_link_libraries(VMStudio PRIVATE Qt6::Widgets)

set_target_properties(VMStudio PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.vmstudio"
    MACOSX_BUNDLE_BUNDLE_NAME "VM Studio"
)
