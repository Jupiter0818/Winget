#include "core/WidgetWindow.h"
#include "core/DesktopManager.h"
#include "core/ConfigManager.h"
#include "core/MonitorManager.h"
#include "core/WidgetManager.h"
#include <windowsx.h>
#include <cmath>

namespace Windget {

bool WidgetWindow::s_classRegistered = false;

WidgetWindow::WidgetWindow(std::shared_ptr<IWidget> widget)
    : m_widget(std::move(widget))
{
}

WidgetWindow::~WidgetWindow()
{
    Close();
}

bool WidgetWindow::RegisterWindowClass(HINSTANCE hInstance)
{
    if (s_classRegistered)
    {
        return true;
    }

    WNDCLASSW wc{};
    wc.lpfnWndProc = StaticWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;

    ATOM atom = RegisterClassW(&wc);
    s_classRegistered = (atom != 0);
    return s_classRegistered;
}

bool WidgetWindow::Create(HINSTANCE hInstance, int x, int y)
{
    if (!m_widget)
    {
        return false;
    }

    if (!RegisterWindowClass(hInstance))
    {
        return false;
    }

    m_x = x;
    m_y = y;

    // 根据坐标点精确定位所在显示器的物理 DPI (支持副屏多分辨率)
    POINT initialPoint{ m_x, m_y };
    MonitorInfo monitor = MonitorManager::GetInstance().GetMonitorForPoint(initialPoint);
    float initialDpi = static_cast<float>(monitor.dpiX);
    float dpiScale = initialDpi / 96.0f;
    float padding = m_renderer.GetPadding();

    int initialWidth = static_cast<int>(std::ceil((m_widget->GetWidth() + padding * 2.0f) * dpiScale));
    int initialHeight = static_cast<int>(std::ceil((m_widget->GetHeight() + padding * 2.0f) * dpiScale));

    // 屏幕越界自愈检查 (若外接显示器已拔掉，自动移回可见区域)
    MonitorManager::GetInstance().EnsureVisibleOnScreen(m_x, m_y, initialWidth, initialHeight);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED,
        WINDOW_CLASS_NAME,
        L"Windget Host",
        WS_POPUP,
        m_x,
        m_y,
        initialWidth,
        initialHeight,
        nullptr,
        nullptr,
        hInstance,
        this
    );

    if (!hwnd)
    {
        return false;
    }

    Show();
    RequestRender();
    return true;
}

void WidgetWindow::Show()
{
    if (m_hwnd && IsWindow(m_hwnd))
    {
        ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
        UpdateWindow(m_hwnd);
    }
}

void WidgetWindow::Hide()
{
    if (m_hwnd && IsWindow(m_hwnd))
    {
        ShowWindow(m_hwnd, SW_HIDE);
    }
}

void WidgetWindow::Close()
{
    if (m_hwnd && IsWindow(m_hwnd))
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

void WidgetWindow::RequestRender()
{
    if (m_hwnd && IsWindow(m_hwnd) && m_widget)
    {
        m_renderer.RenderWidget(m_hwnd, m_widget.get());
    }
}

LRESULT CALLBACK WidgetWindow::StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WidgetWindow* pThis = nullptr;

    if (uMsg == WM_NCCREATE)
    {
        auto* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<WidgetWindow*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        if (pThis)
        {
            pThis->m_hwnd = hwnd;
        }
    }
    else
    {
        pThis = reinterpret_cast<WidgetWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

LRESULT WidgetWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // 监听 Explorer 重启事件 (TaskbarCreated)，重新锚定至桌面
    if (DesktopManager::GetInstance().HandleExplorerRestart(m_hwnd, uMsg))
    {
        RequestRender();
        return 0;
    }

    switch (uMsg)
    {
    case WM_CREATE:
    {
        // 1. 初始化图形渲染器并传入 Widget 逻辑尺寸
        m_renderer.Initialize(m_hwnd, m_widget->GetWidth(), m_widget->GetHeight());

        // 2. 将共享 DirectWrite 工厂传给 Widget 进行字体初始化
        m_widget->Initialize(m_renderer.GetDWriteFactory());

        // 3. 将小组件锚定至桌面层 (Progman Owner + HWND_BOTTOM)
        DesktopManager::GetInstance().AttachToDesktop(m_hwnd);

        // 4. 初次绘制
        m_renderer.RenderWidget(m_hwnd, m_widget.get());

        // 5. 开启定时器
        uint32_t interval = m_widget->GetUpdateIntervalMs();
        if (interval > 0)
        {
            SetTimer(m_hwnd, m_timerId, interval, nullptr);
        }
        return 0;
    }

    case WM_TIMER:
    {
        if (m_widget)
        {
            m_widget->Update();
            m_renderer.RenderWidget(m_hwnd, m_widget.get());
        }
        return 0;
    }

    // 维持桌面层 Z-order，确保普通窗口自然覆盖在小组件上方
    case WM_WINDOWPOSCHANGING:
    {
        auto* pos = reinterpret_cast<WINDOWPOS*>(lParam);
        DesktopManager::GetInstance().EnforceBottomZOrder(pos);
        break;
    }

    // 处理跨屏 DPI 动态变更
    case WM_DPICHANGED:
    {
        UINT newDpi = HIWORD(wParam);
        m_renderer.UpdateDpi(
            m_hwnd,
            static_cast<float>(newDpi),
            m_widget->GetWidth(),
            m_widget->GetHeight()
        );

        auto* prcNewWindow = reinterpret_cast<RECT*>(lParam);
        m_x = prcNewWindow->left;
        m_y = prcNewWindow->top;

        SetWindowPos(
            m_hwnd,
            nullptr,
            m_x,
            m_y,
            m_renderer.GetPhysicalWidth(),
            m_renderer.GetPhysicalHeight(),
            SWP_NOZORDER | SWP_NOACTIVATE
        );

        // 同步更新坐标至配置
        if (m_widget)
        {
            ConfigManager::GetInstance().UpdateWidgetPosition(m_widget->GetId(), m_x, m_y);
            ConfigManager::GetInstance().Save();
        }

        m_renderer.RenderWidget(m_hwnd, m_widget.get());
        return 0;
    }

    // 显示器热插拔或分辨率变更事件
    case WM_DISPLAYCHANGE:
    {
        MonitorManager::GetInstance().RefreshMonitors();
        MonitorManager::GetInstance().EnsureVisibleOnScreen(
            m_x,
            m_y,
            m_renderer.GetPhysicalWidth(),
            m_renderer.GetPhysicalHeight()
        );

        SetWindowPos(
            m_hwnd,
            nullptr,
            m_x,
            m_y,
            0,
            0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
        );

        RequestRender();
        return 0;
    }

    // 点击小组件不抢占当前前台应用输入焦点
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;

    // 鼠标左键按下：开始拖动
    case WM_LBUTTONDOWN:
    {
        m_isDragging = true;
        m_dragStart.x = GET_X_LPARAM(lParam);
        m_dragStart.y = GET_Y_LPARAM(lParam);
        SetCapture(m_hwnd);
        return 0;
    }

    // 鼠标移动：移动小组件位置 (天然支持多屏负坐标)
    case WM_MOUSEMOVE:
    {
        if (m_isDragging)
        {
            POINT cursor;
            GetCursorPos(&cursor);

            m_x = cursor.x - m_dragStart.x;
            m_y = cursor.y - m_dragStart.y;

            SetWindowPos(
                m_hwnd,
                nullptr,
                m_x,
                m_y,
                0,
                0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
            );
        }
        return 0;
    }

    // 鼠标左键释放：结束拖动并持久化新位置
    case WM_LBUTTONUP:
    {
        if (m_isDragging)
        {
            m_isDragging = false;
            ReleaseCapture();

            if (m_widget)
            {
                ConfigManager::GetInstance().UpdateWidgetPosition(m_widget->GetId(), m_x, m_y);
                ConfigManager::GetInstance().Save();
            }
        }
        return 0;
    }

    // 捕获丢失保护
    case WM_CAPTURECHANGED:
    {
        m_isDragging = false;
        return 0;
    }

    // 鼠标右键：快捷菜单
    case WM_RBUTTONUP:
    {
        HMENU hMenu = CreatePopupMenu();
        HMENU hSubAdd = CreatePopupMenu();

        AppendMenuW(hMenu, MF_STRING | MF_GRAYED, 1000, L"Windget 小组件");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

        AppendMenuW(hSubAdd, MF_STRING, 1002, L"🕒 时钟组件 (Clock)");
        AppendMenuW(hSubAdd, MF_STRING, 1006, L"📅 日历组件 (Calendar)");
        AppendMenuW(hSubAdd, MF_STRING, 1007, L"⚡ 硬件监控 (System)");
        AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hSubAdd), L"➕ 添加小组件");

        AppendMenuW(hMenu, MF_STRING, 1003, L"🔄 刷新此组件");
        AppendMenuW(hMenu, MF_STRING, 1004, L"🗑️ 移除此组件");
        AppendMenuW(hMenu, MF_STRING, 1005, L"💻 恢复默认位置");

        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, 1001, L"❌ 退出 Windget");

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

        DestroyMenu(hSubAdd);
        DestroyMenu(hMenu);

        if (cmd == 1001)
        {
            PostQuitMessage(0);
        }
        else if (cmd == 1002)
        {
            WidgetManager::GetInstance().AddWidgetAtDefaultPosition("clock");
        }
        else if (cmd == 1006)
        {
            WidgetManager::GetInstance().AddWidgetAtDefaultPosition("calendar");
        }
        else if (cmd == 1007)
        {
            WidgetManager::GetInstance().AddWidgetAtDefaultPosition("system");
        }
        else if (cmd == 1003)
        {
            if (m_widget)
            {
                m_widget->Update();
            }
            RequestRender();
        }
        else if (cmd == 1004)
        {
            WidgetManager::GetInstance().DestroyWidget(this);
        }
        else if (cmd == 1005)
        {
            WidgetManager::GetInstance().ResetPositions();
        }
        return 0;
    }

    case WM_DESTROY:
    {
        KillTimer(m_hwnd, m_timerId);
        if (m_widget)
        {
            ConfigManager::GetInstance().UpdateWidgetPosition(m_widget->GetId(), m_x, m_y);
            m_widget->Shutdown();
        }
        m_renderer.Cleanup();
        DesktopManager::GetInstance().DetachFromDesktop();
        return 0;
    }

    default:
        break;
    }

    return DefWindowProcW(m_hwnd, uMsg, wParam, lParam);
}

} // namespace Windget
