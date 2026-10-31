#include <windows.h>
#include "core/WidgetManager.h"
#include "core/TrayManager.h"

// ===============================================
// Windget - Windows Desktop Widget Engine
// ===============================================

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int
)
{
    // 1. 初始化 COM 库
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    // 2. 启用 Per-Monitor V2 高 DPI 支持
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32)
    {
        typedef BOOL(WINAPI *SetProcessDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
        auto setDpiAwareness = (SetProcessDpiAwarenessContextProc)GetProcAddress(
            user32,
            "SetProcessDpiAwarenessContext"
        );
        if (setDpiAwareness)
        {
            setDpiAwareness(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }
    }

    // 3. 初始化 WidgetManager 管理器 (自动恢复配置中的小组件)
    auto& widgetManager = Windget::WidgetManager::GetInstance();
    widgetManager.Initialize(hInstance);

    // 4. 初始化系统托盘图标及常驻菜单
    auto& trayManager = Windget::TrayManager::GetInstance();
    trayManager.Initialize(hInstance);

    // 5. 运行全局消息循环
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // 6. 退出并清理全部系统托盘、小组件及 COM 环境
    trayManager.Shutdown();
    widgetManager.Shutdown();
    CoUninitialize();

    return 0;
}
