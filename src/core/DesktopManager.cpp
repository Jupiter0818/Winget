#include "core/DesktopManager.h"
#include <algorithm>

namespace Windget {

DesktopManager& DesktopManager::GetInstance()
{
    static DesktopManager instance;
    return instance;
}

DesktopManager::DesktopManager()
{
    m_taskbarCreatedMsg = RegisterWindowMessageW(L"TaskbarCreated");
}

DesktopManager::~DesktopManager()
{
    m_attachedWidgets.clear();
}

bool DesktopManager::AttachToDesktop(HWND hwndWidget)
{
    if (!hwndWidget || !IsWindow(hwndWidget))
    {
        return false;
    }

    if (std::find(m_attachedWidgets.begin(), m_attachedWidgets.end(), hwndWidget) == m_attachedWidgets.end())
    {
        m_attachedWidgets.push_back(hwndWidget);
    }

    // 1. 查找 Windows Explorer 程序管理器窗口 Progman
    m_hProgman = FindWindowW(L"Progman", L"Program Manager");
    if (!m_hProgman)
    {
        m_hProgman = FindWindowW(L"Progman", nullptr);
    }

    // 2. 将 Progman 设为 Widget 顶级窗口的 Owner (所有者)
    // 通过 GWLP_HWNDPARENT 为顶级 WS_POPUP 窗口指定 Progman 作为 Owner，
    // 使得窗口在逻辑上归属于桌面层，Win + D (显示桌面) 时永远保持在桌面上不被最小化！
    if (m_hProgman && IsWindow(m_hProgman))
    {
        SetWindowLongPtrW(
            hwndWidget,
            GWLP_HWNDPARENT,
            reinterpret_cast<LONG_PTR>(m_hProgman)
        );
    }

    // 3. 将窗口放置在桌面 Z-order 最底层 (HWND_BOTTOM)
    // 使得任何普通应用程序打开或激活时，自然覆盖在小组件上方
    SetWindowPos(
        hwndWidget,
        HWND_BOTTOM,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
    );

    return true;
}

void DesktopManager::DetachFromDesktop(HWND hwndWidget)
{
    if (hwndWidget && IsWindow(hwndWidget))
    {
        SetWindowLongPtrW(hwndWidget, GWLP_HWNDPARENT, 0);
        auto it = std::find(m_attachedWidgets.begin(), m_attachedWidgets.end(), hwndWidget);
        if (it != m_attachedWidgets.end())
        {
            m_attachedWidgets.erase(it);
        }
    }
}

bool DesktopManager::HandleExplorerRestart(HWND hwndWidget, UINT uMsg)
{
    if (uMsg != 0 && uMsg == m_taskbarCreatedMsg)
    {
        // Explorer 重启，重新将所有小组件锚定至新的 Progman 桌面层
        for (HWND h : m_attachedWidgets)
        {
            if (h && IsWindow(h))
            {
                AttachToDesktop(h);
            }
        }
        return true;
    }
    return false;
}

void DesktopManager::EnforceBottomZOrder(WINDOWPOS* pos)
{
    if (!pos)
    {
        return;
    }

    // 阻止窗口被提升到普通应用之上，始终保持在底部层级
    if (!(pos->flags & SWP_NOZORDER))
    {
        pos->hwndInsertAfter = HWND_BOTTOM;
    }
}

} // namespace Windget
