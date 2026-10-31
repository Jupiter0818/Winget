#pragma once

#include <windows.h>
#include <vector>
#include <string>

namespace Windget {

struct MonitorInfo
{
    HMONITOR handle = nullptr;
    std::wstring deviceName;
    RECT monitorRect{};    // 虚拟屏幕物理像素全屏区域 (可能包含负坐标)
    RECT workArea{};       // 避开系统任务栏的工作区
    bool isPrimary = false;
    UINT dpiX = 96;
    UINT dpiY = 96;
    float scaleFactor = 1.0f; // 1.0 (100%), 1.25 (125%), 1.5 (150%), 2.0 (200%)
};

class MonitorManager
{
public:
    static MonitorManager& GetInstance();

    // 重新扫描并更新所有活动显示器信息
    void RefreshMonitors();

    // 获取所有活动显示器列表
    const std::vector<MonitorInfo>& GetMonitors() const { return m_monitors; }

    // 获取主显示器
    MonitorInfo GetPrimaryMonitor() const;

    // 根据窗口句柄查找所在显示器
    MonitorInfo GetMonitorForWindow(HWND hwnd) const;

    // 根据坐标点查找所在显示器
    MonitorInfo GetMonitorForPoint(POINT pt) const;

    // 安全检查：如果小组件由于显示器拔除而位于所有屏幕之外，自动移动至可见工作区
    void EnsureVisibleOnScreen(int& x, int& y, int width, int height) const;

    // 获取多显示器虚拟屏幕的完整矩形范围
    RECT GetVirtualScreenRect() const;

private:
    MonitorManager();
    ~MonitorManager() = default;

    MonitorManager(const MonitorManager&) = delete;
    MonitorManager& operator=(const MonitorManager&) = delete;

    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);

private:
    std::vector<MonitorInfo> m_monitors;
};

} // namespace Windget
