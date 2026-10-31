#include "core/MonitorManager.h"
#include <algorithm>

namespace Windget {

MonitorManager& MonitorManager::GetInstance()
{
    static MonitorManager instance;
    return instance;
}

MonitorManager::MonitorManager()
{
    RefreshMonitors();
}

void MonitorManager::RefreshMonitors()
{
    m_monitors.clear();
    EnumDisplayMonitors(
        nullptr,
        nullptr,
        MonitorEnumProc,
        reinterpret_cast<LPARAM>(this)
    );

    // 如果未枚举到任何显示器 (如受限终端环境)，构造默认虚拟显示器
    if (m_monitors.empty())
    {
        MonitorInfo fallback;
        fallback.handle = nullptr;
        fallback.deviceName = L"DefaultMonitor";
        fallback.monitorRect = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
        fallback.workArea = fallback.monitorRect;
        fallback.isPrimary = true;
        fallback.dpiX = 96;
        fallback.dpiY = 96;
        fallback.scaleFactor = 1.0f;
        m_monitors.push_back(fallback);
    }
}

BOOL CALLBACK MonitorManager::MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT /*lprcMonitor*/, LPARAM dwData)
{
    auto* pThis = reinterpret_cast<MonitorManager*>(dwData);
    if (!pThis)
    {
        return FALSE;
    }

    MONITORINFOEXW mi{};
    mi.cbSize = sizeof(MONITORINFOEXW);
    if (!GetMonitorInfoW(hMonitor, &mi))
    {
        return TRUE;
    }

    MonitorInfo info{};
    info.handle = hMonitor;
    info.deviceName = mi.szDevice;
    info.monitorRect = mi.rcMonitor;
    info.workArea = mi.rcWork;
    info.isPrimary = ((mi.dwFlags & MONITORINFOF_PRIMARY) != 0);
    info.dpiX = 96;
    info.dpiY = 96;

    // 动态调用 GetDpiForMonitor 获取每个显示器的独立物理 DPI
    HMODULE shcore = LoadLibraryW(L"Shcore.dll");
    if (shcore)
    {
        typedef HRESULT(WINAPI *GetDpiForMonitorProc)(HMONITOR, int, UINT*, UINT*);
        auto pfnGetDpi = (GetDpiForMonitorProc)GetProcAddress(shcore, "GetDpiForMonitor");
        if (pfnGetDpi)
        {
            UINT dpiX = 96, dpiY = 96;
            // 0 代表 MDT_EFFECTIVE_DPI (包含系统与显示缩放的有效 DPI)
            if (SUCCEEDED(pfnGetDpi(hMonitor, 0, &dpiX, &dpiY)) && dpiX > 0)
            {
                info.dpiX = dpiX;
                info.dpiY = dpiY;
            }
        }
        FreeLibrary(shcore);
    }
    else if (hdcMonitor)
    {
        int dpiX = GetDeviceCaps(hdcMonitor, LOGPIXELSX);
        int dpiY = GetDeviceCaps(hdcMonitor, LOGPIXELSY);
        if (dpiX > 0) info.dpiX = static_cast<UINT>(dpiX);
        if (dpiY > 0) info.dpiY = static_cast<UINT>(dpiY);
    }

    info.scaleFactor = static_cast<float>(info.dpiX) / 96.0f;
    pThis->m_monitors.push_back(info);
    return TRUE;
}

MonitorInfo MonitorManager::GetPrimaryMonitor() const
{
    for (const auto& mon : m_monitors)
    {
        if (mon.isPrimary)
        {
            return mon;
        }
    }

    if (!m_monitors.empty())
    {
        return m_monitors.front();
    }

    MonitorInfo fallback{};
    fallback.monitorRect = { 0, 0, 1920, 1080 };
    fallback.workArea = fallback.monitorRect;
    fallback.isPrimary = true;
    fallback.dpiX = 96;
    fallback.dpiY = 96;
    fallback.scaleFactor = 1.0f;
    return fallback;
}

MonitorInfo MonitorManager::GetMonitorForWindow(HWND hwnd) const
{
    if (hwnd && IsWindow(hwnd))
    {
        HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        for (const auto& mon : m_monitors)
        {
            if (mon.handle == hMon)
            {
                return mon;
            }
        }
    }
    return GetPrimaryMonitor();
}

MonitorInfo MonitorManager::GetMonitorForPoint(POINT pt) const
{
    HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    for (const auto& mon : m_monitors)
    {
        if (mon.handle == hMon)
        {
            return mon;
        }
    }
    return GetPrimaryMonitor();
}

RECT MonitorManager::GetVirtualScreenRect() const
{
    RECT rect{};
    rect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    rect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    rect.right = rect.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
    rect.bottom = rect.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
    return rect;
}

void MonitorManager::EnsureVisibleOnScreen(int& x, int& y, int width, int height) const
{
    // 检查小组件中心点所在显示器
    POINT center{ x + width / 2, y + height / 2 };
    HMONITOR hMon = MonitorFromPoint(center, MONITOR_DEFAULTTONULL);

    if (!hMon)
    {
        // 尝试检查四角
        POINT corners[4] = {
            { x, y },
            { x + width, y },
            { x, y + height },
            { x + width, y + height }
        };

        for (int i = 0; i < 4; ++i)
        {
            hMon = MonitorFromPoint(corners[i], MONITOR_DEFAULTTONULL);
            if (hMon) break;
        }
    }

    // 若完全脱离所有显示器工作区（如外接副屏拔除断开），自动移至主显示器安全可见区域
    if (!hMon)
    {
        MonitorInfo primary = GetPrimaryMonitor();
        x = primary.workArea.left + 50;
        y = primary.workArea.top + 50;
    }
}

} // namespace Windget
