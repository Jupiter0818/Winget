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
    src/widgets/ClockWidget.cpp
    src/widgets/CalendarWidget.cpp
    src/widgets/SystemMonitorWidget.cpp
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
    ole32
    shell32
    advapi32
)
