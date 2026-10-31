#include "core/WidgetWindow.h"
#include "core/DesktopManager.h"
#include "core/ConfigManager.h"
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

    float initialDpi = GetWindowDpi(nullptr);
    float dpiScale = initialDpi / 96.0f;
    float padding = m_renderer.GetPadding();

    int initialWidth = static_cast<int>(std::ceil((m_widget->GetWidth() + padding * 2.0f) * dpiScale));
    int initialHeight = static_cast<int>(std::ceil((m_widget->GetHeight() + padding * 2.0f) * dpiScale));

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

        // 5. 按照 Widget 自身设定的更新周期开启定时器
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

    // 处理 DPI 变更
    case WM_DPICHANGED:
    {
        UINT newDpi = HIWORD(wParam);
        m_renderer.UpdateDpi(m_hwnd, static_cast<float>(newDpi), m_widget->GetWidth(), m_widget->GetHeight());

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

        m_renderer.RenderWidget(m_hwnd, m_widget.get());
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

    // 鼠标移动：移动小组件位置
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

    // 鼠标左键释放：结束拖动并保存最新位置
    case WM_LBUTTONUP:
    {
        if (m_isDragging)
        {
            m_isDragging = false;
            ReleaseCapture();

            // 拖动完成后将当前位置保存到配置系统
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
        std::wstring menuTitle = L"退出 Windget";
        InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, 1001, menuTitle.c_str());

        POINT pt;
        GetCursorPos(&pt);

        SetForegroundWindow(m_hwnd);
        int cmd = TrackPopupMenu(
            hMenu,
            TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
            pt.x,
            pt.y,
            0,
            m_hwnd,
            nullptr
        );
        DestroyMenu(hMenu);

        if (cmd == 1001)
        {
            PostQuitMessage(0);
        }
        return 0;
    }

    case WM_DESTROY:
    {
        KillTimer(m_hwnd, m_timerId);
        if (m_widget)
        {
            // 窗口销毁时同步最新坐标
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
