#include "core/TrayManager.h"
#include "core/WidgetManager.h"
#include "core/ConfigManager.h"
#include "core/ThemeManager.h"
#include "core/AutoStartManager.h"
#include <cmath>
#include <cstdint>
#include <vector>

namespace Windget {

TrayManager& TrayManager::GetInstance()
{
    static TrayManager instance;
    return instance;
}

TrayManager::TrayManager()
{
    m_taskbarCreatedMsg = RegisterWindowMessageW(L"TaskbarCreated");
}

TrayManager::~TrayManager()
{
    Shutdown();
}

bool TrayManager::Initialize(HINSTANCE hInstance)
{
    m_hInstance = hInstance;

    if (!CreateMessageWindow(hInstance))
    {
        return false;
    }

    m_hIcon = CreateAppTrayIcon();
    bool added = AddOrUpdateTrayIcon();
    if (added)
    {
        ShowBalloonNotification(L"Windget 桌面组件", L"已常驻在系统托盘，右键或双击托盘图标可进行管理。");
    }
    return added;
}

void TrayManager::Shutdown()
{
    RemoveTrayIcon();

    if (m_hIcon)
    {
        DestroyIcon(m_hIcon);
        m_hIcon = nullptr;
    }

    if (m_hwnd && IsWindow(m_hwnd))
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

bool TrayManager::CreateMessageWindow(HINSTANCE hInstance)
{
    WNDCLASSW wc{};
    wc.lpfnWndProc = StaticTrayWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TRAY_CLASS_NAME;

    RegisterClassW(&wc);

    // 使用隐藏无边框顶级窗口以正确接收系统广播 (如 TaskbarCreated)
    m_hwnd = CreateWindowExW(
        0,
        TRAY_CLASS_NAME,
        L"Windget_TrayReceiver",
        WS_POPUP,
        0, 0, 0, 0,
        nullptr,
        nullptr,
        hInstance,
        this
    );

    return (m_hwnd != nullptr && IsWindow(m_hwnd));
}

HICON TrayManager::CreateAppTrayIcon()
{
    int size = 32;
    HDC hdc = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(hdc);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size;
    bmi.bmiHeader.biHeight = -size; // top-down 坐标
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    uint32_t* pixels = nullptr;
    HBITMAP hColorBmp = CreateDIBSection(
        memDC,
        &bmi,
        DIB_RGB_COLORS,
        reinterpret_cast<void**>(&pixels),
        nullptr,
        0
    );

    // 单色 1-bit mask bitmap，每行 32 位对齐
    int maskPitch = ((size + 31) / 32) * 4;
    std::vector<BYTE> maskBits(maskPitch * size, 0x00);
    HBITMAP hMaskBmp = CreateBitmap(size, size, 1, 1, maskBits.data());

    if (hColorBmp && pixels)
    {
        float cx = 15.5f;
        float cy = 15.5f;
        float radius = 13.5f;

        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                float dx = x - cx;
                float dy = y - cy;
                float dist = std::sqrt(dx * dx + dy * dy);
                int idx = y * size + x;

                if (dist <= radius)
                {
                    uint8_t a = 255;
                    uint8_t r = 24;
                    uint8_t g = 28;
                    uint8_t b = 38;

                    // 浅青外环
                    if (dist >= radius - 2.0f)
                    {
                        r = 0; g = 168; b = 255;
                    }
                    // 时针向 10 点
                    else if (dx <= 0 && dy <= 0 && std::abs(dx - dy) <= 1.2f && dist <= 7.0f)
                    {
                        r = 255; g = 255; b = 255;
                    }
                    // 分针向 2 点
                    else if (dx >= 0 && dy <= 0 && std::abs(dx + dy) <= 1.2f && dist <= 9.0f)
                    {
                        r = 255; g = 255; b = 255;
                    }
                    // 中心圆点
                    else if (dist <= 2.0f)
                    {
                        r = 0; g = 210; b = 255;
                    }

                    pixels[idx] = (static_cast<uint32_t>(a) << 24) |
                                  (static_cast<uint32_t>(r) << 16) |
                                  (static_cast<uint32_t>(g) << 8) |
                                  static_cast<uint32_t>(b);
                }
                else
                {
                    pixels[idx] = 0;
                }
            }
        }
    }

    ICONINFO ii{};
    ii.fIcon = TRUE;
    ii.hbmColor = hColorBmp;
    ii.hbmMask = hMaskBmp;
    HICON hIcon = CreateIconIndirect(&ii);

    if (hColorBmp) DeleteObject(hColorBmp);
    if (hMaskBmp) DeleteObject(hMaskBmp);
    DeleteDC(memDC);
    ReleaseDC(nullptr, hdc);

    if (!hIcon)
    {
        hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));
    }
    return hIcon;
}

bool TrayManager::AddOrUpdateTrayIcon()
{
    if (!m_hwnd || !IsWindow(m_hwnd))
    {
        return false;
    }

    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_hwnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = m_hIcon;

    std::wstring tip = L"Windget 桌面小组件";
    wcsncpy_s(m_nid.szTip, tip.c_str(), _TRUNCATE);

    DWORD msg = m_iconAdded ? NIM_MODIFY : NIM_ADD;
    BOOL res = Shell_NotifyIconW(msg, &m_nid);
    if (!res && !m_iconAdded)
    {
        Sleep(50);
        res = Shell_NotifyIconW(NIM_ADD, &m_nid);
    }

    if (res)
    {
        m_iconAdded = true;
    }
    return (res != FALSE);
}

void TrayManager::RemoveTrayIcon()
{
    if (m_iconAdded && m_hwnd && IsWindow(m_hwnd))
    {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_iconAdded = false;
    }
}

void TrayManager::ShowBalloonNotification(const std::wstring& title, const std::wstring& message)
{
    if (!m_iconAdded || !m_hwnd || !IsWindow(m_hwnd))
    {
        return;
    }

    NOTIFYICONDATAW nid = m_nid;
    nid.uFlags |= NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO;
    wcsncpy_s(nid.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(nid.szInfo, message.c_str(), _TRUNCATE);
    nid.uTimeout = 3000;
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void TrayManager::ShowContextMenu()
{
    HMENU hMenu = CreatePopupMenu();
    HMENU hSubAdd = CreatePopupMenu();
    HMENU hSubTheme = CreatePopupMenu();

    AppendMenuW(hMenu, MF_STRING | MF_GRAYED, IDM_TRAY_TITLE, L"Windget 桌面组件");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    // 全局深浅毛玻璃外观主题
    AppThemeMode curMode = ThemeManager::GetInstance().GetThemeMode();
    AppendMenuW(hSubTheme, MF_STRING | (curMode == AppThemeMode::System ? MF_CHECKED : 0), IDM_TRAY_THEME_SYSTEM, L"🌓 跟随系统 (System Default)");
    AppendMenuW(hSubTheme, MF_STRING | (curMode == AppThemeMode::Dark ? MF_CHECKED : 0), IDM_TRAY_THEME_DARK, L"⬛ 深色毛玻璃 (Dark Frosted)");
    AppendMenuW(hSubTheme, MF_STRING | (curMode == AppThemeMode::Light ? MF_CHECKED : 0), IDM_TRAY_THEME_LIGHT, L"⬜ 浅色毛玻璃 (Light Frosted)");
    AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hSubTheme), L"🎨 外观主题 (Theme)");

    // ➕ 添加小组件子菜单
    AppendMenuW(hSubAdd, MF_STRING, IDM_TRAY_ADD_CLOCK, L"🕒 时钟组件 (Clock)");
    AppendMenuW(hSubAdd, MF_STRING, IDM_TRAY_ADD_CALENDAR, L"📅 日历组件 (Calendar)");
    AppendMenuW(hSubAdd, MF_STRING, IDM_TRAY_ADD_SYSTEM, L"⚡ 硬件监控 (System)");
    AppendMenuW(hSubAdd, MF_STRING, IDM_TRAY_ADD_MUSIC, L"🎵 音乐控制器 (Music)");
    AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hSubAdd), L"➕ 添加小组件");

    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_REFRESH_ALL, L"🔄 刷新全部组件");
    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_TOGGLE_SHOW, L"👁️ 显示 / 隐藏全部");
    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_RESET_POSITION, L"💻 恢复默认位置");

    // 桌面网格吸附与位置锁定
    bool isSnap = ConfigManager::GetInstance().IsSnapToGrid();
    bool isLocked = ConfigManager::GetInstance().IsLockPosition();
    AppendMenuW(hMenu, MF_STRING | (isSnap ? MF_CHECKED : 0), IDM_TRAY_SNAP_GRID, L"🧲 对齐到桌面网格 (Snap to Grid)");
    AppendMenuW(hMenu, MF_STRING | (isLocked ? MF_CHECKED : 0), IDM_TRAY_LOCK_POSITION, L"🔒 锁定小组件位置 (Lock Position)");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    // 开机自启动
    bool autoStart = AutoStartManager::GetInstance().IsAutoStartEnabled();
    UINT autoStartFlags = MF_STRING | (autoStart ? MF_CHECKED : MF_UNCHECKED);
    AppendMenuW(hMenu, autoStartFlags, IDM_TRAY_AUTO_START, L"🚀 开机自启动");

    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_OPEN_CONFIG, L"📂 打开配置文件目录");
    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_GITHUB, L"🌐 项目 GitHub 主页");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_EXIT, L"❌ 退出 Windget");

    POINT pt;
    GetCursorPos(&pt);

    SetForegroundWindow(m_hwnd);
    int cmd = TrackPopupMenu(
        hMenu,
        TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
        pt.x,
        pt.y,
        0,
        m_hwnd,
        nullptr
    );
    PostMessageW(m_hwnd, WM_NULL, 0, 0);

    DestroyMenu(hSubTheme);
    DestroyMenu(hSubAdd);
    DestroyMenu(hMenu);

    switch (cmd)
    {
    case IDM_TRAY_THEME_SYSTEM:
        ThemeManager::GetInstance().SetThemeMode(AppThemeMode::System);
        ConfigManager::GetInstance().SetThemeMode("system");
        ConfigManager::GetInstance().Save();
        WidgetManager::GetInstance().RefreshAll();
        break;

    case IDM_TRAY_THEME_DARK:
        ThemeManager::GetInstance().SetThemeMode(AppThemeMode::Dark);
        ConfigManager::GetInstance().SetThemeMode("dark");
        ConfigManager::GetInstance().Save();
        WidgetManager::GetInstance().RefreshAll();
        break;

    case IDM_TRAY_THEME_LIGHT:
        ThemeManager::GetInstance().SetThemeMode(AppThemeMode::Light);
        ConfigManager::GetInstance().SetThemeMode("light");
        ConfigManager::GetInstance().Save();
        WidgetManager::GetInstance().RefreshAll();
        break;
    case IDM_TRAY_ADD_CLOCK:
        WidgetManager::GetInstance().AddWidgetAtDefaultPosition("clock");
        break;

    case IDM_TRAY_ADD_CALENDAR:
        WidgetManager::GetInstance().AddWidgetAtDefaultPosition("calendar");
        break;

    case IDM_TRAY_ADD_SYSTEM:
        WidgetManager::GetInstance().AddWidgetAtDefaultPosition("system");
        break;

    case IDM_TRAY_ADD_MUSIC:
        WidgetManager::GetInstance().AddWidgetAtDefaultPosition("music");
        break;

    case IDM_TRAY_REFRESH_ALL:
        WidgetManager::GetInstance().RefreshAll();
        break;

    case IDM_TRAY_TOGGLE_SHOW:
        WidgetManager::GetInstance().ToggleVisibility();
        break;

    case IDM_TRAY_RESET_POSITION:
        WidgetManager::GetInstance().ResetPositions();
        break;

    case IDM_TRAY_SNAP_GRID:
    {
        bool snap = !ConfigManager::GetInstance().IsSnapToGrid();
        ConfigManager::GetInstance().SetSnapToGrid(snap);
        ConfigManager::GetInstance().Save();
        break;
    }

    case IDM_TRAY_LOCK_POSITION:
    {
        bool lock = !ConfigManager::GetInstance().IsLockPosition();
        ConfigManager::GetInstance().SetLockPosition(lock);
        ConfigManager::GetInstance().Save();
        break;
    }

    case IDM_TRAY_AUTO_START:
    {
        bool currentState = AutoStartManager::GetInstance().IsAutoStartEnabled();
        AutoStartManager::GetInstance().SetAutoStartEnabled(!currentState);
        break;
    }

    case IDM_TRAY_OPEN_CONFIG:
    {
        std::wstring configDir = ConfigManager::GetInstance().GetConfigDirectory().wstring();
        ShellExecuteW(nullptr, L"open", configDir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        break;
    }

    case IDM_TRAY_GITHUB:
        ShellExecuteW(nullptr, L"open", L"https://github.com/Jupiter0818/Winget.git", nullptr, nullptr, SW_SHOWNORMAL);
        break;

    case IDM_TRAY_EXIT:
        PostQuitMessage(0);
        break;

    default:
        break;
    }
}

LRESULT CALLBACK TrayManager::StaticTrayWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TrayManager* pThis = nullptr;
    if (uMsg == WM_NCCREATE)
    {
        auto* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<TrayManager*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        if (pThis)
        {
            pThis->m_hwnd = hwnd;
        }
    }
    else
    {
        pThis = reinterpret_cast<TrayManager*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        return pThis->HandleMessage(hwnd, uMsg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

LRESULT TrayManager::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (m_taskbarCreatedMsg != 0 && uMsg == m_taskbarCreatedMsg)
    {
        // 资源管理器 (explorer.exe) 重启后，自动恢复托盘图标
        AddOrUpdateTrayIcon();
        return 0;
    }

    switch (uMsg)
    {
    case WM_TRAYICON:
    {
        UINT evt = LOWORD(lParam);
        if (evt == WM_RBUTTONUP || evt == WM_CONTEXTMENU)
        {
            ShowContextMenu();
        }
        else if (evt == WM_LBUTTONDBLCLK)
        {
            WidgetManager::GetInstance().ToggleVisibility();
        }
        return 0;
    }

    case WM_DESTROY:
        RemoveTrayIcon();
        return 0;

    default:
        break;
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

} // namespace Windget
