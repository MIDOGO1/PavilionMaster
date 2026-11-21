cmake_minimum_required(VERSION 3.16)
project(15GControlCetner VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Widgets)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(app15GControlCetner
    main.cpp
    SystemBackend.h
    SystemBackend.cpp
    FanControl.h
    FanControl.cpp
    PowerPlans.h
    SingleInstance.h
)

# Fonts: every .ttf/.otf in ./fonts is embedded in the executable (see fonts/README.txt)
file(GLOB APP_FONT_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/fonts/*.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/fonts/*.otf
    ${CMAKE_CURRENT_SOURCE_DIR}/fonts/*.TTF
    ${CMAKE_CURRENT_SOURCE_DIR}/fonts/*.OTF
)

qt_add_qml_module(app15GControlCetner
    URI GControlCetner
    VERSION 1.0
    QML_FILES
        Frame_1.qml
    RESOURCES
        assets/electronics.png
        assets/fan_Speed.png
        assets/full_Battery.png
        assets/heart_with_Pulse.png
        assets/image_1.png
        assets/image_2.png
        assets/leaf.png
        assets/memory_Slot.png
        assets/motherboard.png
        assets/no_symbol.png
        assets/rectangle_2.png
        assets/rectangle_3.png
        assets/rectangle_4.png
        assets/rectangle_5.png
        assets/scales.png
        assets/switches.png
        assets/video_Card.png
        assets/voltage.png
        assets/voltage_dark.png
        assets/scales_dark.png
        assets/leaf_dark.png
        assets/switches_dark.png
        ${APP_FONT_FILES}
)

target_link_libraries(app15GControlCetner
    PRIVATE
    Qt6::Quick
    Qt6::Widgets
    pdh
    advapi32
    dwmapi
)

target_include_directories(app15GControlCetner PRIVATE
    "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.2/include"
)

# NVML (nvml.dll) is loaded at runtime by SystemBackend, so it is intentionally not linked here.

set_target_properties(app15GControlCetner PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS app15GControlCetner
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
