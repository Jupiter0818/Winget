#pragma once

#include <windows.h>
#include <vector>

namespace Windget {

class DesktopManager
{
public:
    static DesktopManager& GetInstance();

    // 将 Widget 窗口锚定到 Windows 桌面层 (归属 Progman 并锁定于 Z-order 底层)
    bool AttachToDesktop(HWND hwndWidget);

    // 从桌面层脱离
    void DetachFromDesktop(HWND hwndWidget);

    // 检查并处理 Explorer 重启消息 (TaskbarCreated)
    bool HandleExplorerRestart(HWND hwndWidget, UINT uMsg);

    // 获取 TaskbarCreated 消息 ID
    UINT GetTaskbarCreatedMsg() const { return m_taskbarCreatedMsg; }

    // 维持桌面层 Z-order，确保不被提升到普通应用窗口之上
    void EnforceBottomZOrder(WINDOWPOS* pos);

private:
    DesktopManager();
    ~DesktopManager();

    DesktopManager(const DesktopManager&) = delete;
    DesktopManager& operator=(const DesktopManager&) = delete;

private:
    std::vector<HWND> m_attachedWidgets;
    HWND m_hProgman = nullptr;
    UINT m_taskbarCreatedMsg = 0;
};

} // namespace Windget
