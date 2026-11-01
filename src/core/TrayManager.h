#pragma once

#include <windows.h>
#include <shellapi.h>
#include <string>

namespace Windget {

class TrayManager
{
public:
    static TrayManager& GetInstance();

    // 初始化系统托盘常驻图标与后台监听机制
    bool Initialize(HINSTANCE hInstance);

    // 释放并移除托盘图标
    void Shutdown();

    // 发送系统气泡通知
    void ShowBalloonNotification(const std::wstring& title, const std::wstring& message);

private:
    TrayManager();
    ~TrayManager();

    TrayManager(const TrayManager&) = delete;
    TrayManager& operator=(const TrayManager&) = delete;

    bool CreateMessageWindow(HINSTANCE hInstance);
    bool AddOrUpdateTrayIcon();
    void RemoveTrayIcon();
    void ShowContextMenu();
    HICON CreateAppTrayIcon();

    static LRESULT CALLBACK StaticTrayWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    static constexpr wchar_t TRAY_CLASS_NAME[] = L"Windget_TrayReceiverWindow";
    static constexpr UINT WM_TRAYICON = WM_USER + 200;

    enum MenuItemId {
        IDM_TRAY_TITLE = 2000,
        IDM_TRAY_ADD_CLOCK = 2001,
        IDM_TRAY_REFRESH_ALL = 2002,
        IDM_TRAY_TOGGLE_SHOW = 2003,
        IDM_TRAY_RESET_POSITION = 2004,
        IDM_TRAY_AUTO_START = 2005,
        IDM_TRAY_OPEN_CONFIG = 2006,
        IDM_TRAY_GITHUB = 2007,
        IDM_TRAY_EXIT = 2008
    };

    HINSTANCE m_hInstance = nullptr;
    HWND m_hwnd = nullptr;
    HICON m_hIcon = nullptr;
    NOTIFYICONDATAW m_nid{};
    bool m_iconAdded = false;
    UINT m_taskbarCreatedMsg = 0;
};

} // namespace Windget
