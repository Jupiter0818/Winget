cmake_minimum_required(VERSION 3.20)

project(Windget LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Windget WIN32
    src/main.cpp
    src/graphics/D2DRenderer.cpp
    src/core/DesktopManager.cpp
    src/core/WidgetWindow.cpp
    src/core/WidgetManager.cpp
    src/core/ConfigManager.cpp
    src/core/MonitorManager.cpp
    src/core/TrayManager.cpp
    src/core/AutoStartManager.cpp
    src/core/MediaManager.cpp
    src/core/ThemeManager.cpp
    src/widgets/ClockWidget.cpp
    src/widgets/CalendarWidget.cpp
    src/widgets/SystemMonitorWidget.cpp
    src/widgets/MusicWidget.cpp
    src/utils/JsonUtils.cpp
)

target_include_directories(Windget PRIVATE
    src
)

target_link_libraries(Windget PRIVATE
    user32
    gdi32
    d2d1
    dwrite
    windowscodecs
    ole32
    shell32
    advapi32
)

# 静态链接 MinGW 运行时，消除对 libgcc / libstdc++ 的外部 DLL 依赖
if(MINGW)
    target_link_options(Windget PRIVATE -static -static-libgcc -static-libstdc++ -s)
endif()

# 设置发行包输出目录为项目根目录下的 dist/
set_target_properties(Windget PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist"
)
