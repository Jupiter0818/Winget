#include "core/WidgetWindow.h"
#include "core/DesktopManager.h"
#include "core/ConfigManager.h"
#include "core/ThemeManager.h"
#include "core/MonitorManager.h"
#include "core/WidgetManager.h"
#include "widgets/MusicWidget.h"
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
        DesktopManager::GetInstance().DetachFromDesktop(m_hwnd);
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

RECT WidgetWindow::GetVisualCardBounds() const
{
    float dpiScale = m_renderer.GetDpiScale();
    int padPx = static_cast<int>(std::round(m_renderer.GetPadding() * dpiScale));
    int cardW = static_cast<int>(std::round((m_widget ? m_widget->GetWidth() : 320.0f) * dpiScale));
    int cardH = static_cast<int>(std::round((m_widget ? m_widget->GetHeight() : 170.0f) * dpiScale));

    return RECT{
        m_x + padPx,
        m_y + padPx,
        m_x + padPx + cardW,
        m_y + padPx + cardH
    };
}

POINT WidgetWindow::SnapPosition(int rawX, int rawY, bool isDuringDrag) const
{
    float dpiScale = m_renderer.GetDpiScale();
    int padPx = static_cast<int>(std::round(m_renderer.GetPadding() * dpiScale));
    int cardW = static_cast<int>(std::round((m_widget ? m_widget->GetWidth() : 320.0f) * dpiScale));
    int cardH = static_cast<int>(std::round((m_widget ? m_widget->GetHeight() : 170.0f) * dpiScale));

    int cardLeft = rawX + padPx;
    int cardTop = rawY + padPx;
    int cardRight = cardLeft + cardW;
    int cardBottom = cardTop + cardH;

    // 获取当前卡片中心所在的显示器工作区
    POINT centerPt{ cardLeft + cardW / 2, cardTop + cardH / 2 };
    MonitorInfo monitor = MonitorManager::GetInstance().GetMonitorForPoint(centerPt);

    int workL = monitor.workArea.left;
    int workT = monitor.workArea.top;
    int workR = monitor.workArea.right;
    int workB = monitor.workArea.bottom;

    // 系统桌面图标网格间距 (SM_CXICONSPACING / SM_CYICONSPACING)
    int iconSpacingX = GetSystemMetrics(SM_CXICONSPACING);
    int iconSpacingY = GetSystemMetrics(SM_CYICONSPACING);
    if (iconSpacingX <= 0) iconSpacingX = static_cast<int>(std::round(75.0f * dpiScale));
    if (iconSpacingY <= 0) iconSpacingY = static_cast<int>(std::round(75.0f * dpiScale));

    int edgeMargin = static_cast<int>(std::round(16.0f * dpiScale));      // 屏幕边缘留白边距
    int interWidgetGap = static_cast<int>(std::round(16.0f * dpiScale));  // 组件并排/堆叠间隙
    int magneticDist = static_cast<int>(std::round(18.0f * dpiScale));    // 磁吸感应阈值距离

    int bestLeft = cardLeft;
    int bestTop = cardTop;
    bool snappedX = false;
    bool snappedY = false;

    // 1. 优先判定：与其他同屏小组件相互磁吸对齐 (左对齐、右对齐、顶对齐、底对齐、紧贴并排)
    const auto& allWindows = WidgetManager::GetInstance().GetWidgets();
    for (const auto& pOther : allWindows)
    {
        if (!pOther || pOther.get() == this || !pOther->IsVisible())
        {
            continue;
        }

        RECT o = pOther->GetVisualCardBounds();

        // 水平磁吸
        if (!snappedX)
        {
            // 左边线对齐
            if (std::abs(cardLeft - o.left) <= magneticDist)
            {
                bestLeft = o.left;
                snappedX = true;
            }
            // 右边线对齐
            else if (std::abs(cardRight - o.right) <= magneticDist)
            {
                bestLeft = o.right - cardW;
                snappedX = true;
            }
            // 紧贴邻居右侧并排
            else if (std::abs(cardLeft - (o.right + interWidgetGap)) <= magneticDist)
            {
                bestLeft = o.right + interWidgetGap;
                snappedX = true;
            }
            // 紧贴邻居左侧并排
            else if (std::abs(cardRight - (o.left - interWidgetGap)) <= magneticDist)
            {
                bestLeft = o.left - interWidgetGap - cardW;
                snappedX = true;
            }
        }

        // 垂直磁吸
        if (!snappedY)
        {
            // 顶边线对齐
            if (std::abs(cardTop - o.top) <= magneticDist)
            {
                bestTop = o.top;
                snappedY = true;
            }
            // 底边线对齐
            else if (std::abs(cardBottom - o.bottom) <= magneticDist)
            {
                bestTop = o.bottom - cardH;
                snappedY = true;
            }
            // 堆叠在邻居下方
            else if (std::abs(cardTop - (o.bottom + interWidgetGap)) <= magneticDist)
            {
                bestTop = o.bottom + interWidgetGap;
                snappedY = true;
            }
            // 堆叠在邻居上方
            else if (std::abs(cardBottom - (o.top - interWidgetGap)) <= magneticDist)
            {
                bestTop = o.top - interWidgetGap - cardH;
                snappedY = true;
            }
        }
    }

    // 2. 次级判定：吸附屏幕边缘留白
    if (!snappedX)
    {
        if (std::abs(cardLeft - (workL + edgeMargin)) <= magneticDist)
        {
            bestLeft = workL + edgeMargin;
            snappedX = true;
        }
        else if (std::abs(cardRight - (workR - edgeMargin)) <= magneticDist)
        {
            bestLeft = workR - edgeMargin - cardW;
            snappedX = true;
        }
    }

    if (!snappedY)
    {
        if (std::abs(cardTop - (workT + edgeMargin)) <= magneticDist)
        {
            bestTop = workT + edgeMargin;
            snappedY = true;
        }
        else if (std::abs(cardBottom - (workB - edgeMargin)) <= magneticDist)
        {
            bestTop = workB - edgeMargin - cardH;
            snappedY = true;
        }
    }

    // 3. 终级判定：对齐到 Windows 桌面图标网格
    int originX = workL + edgeMargin;
    int originY = workT + edgeMargin;

    if (!snappedX)
    {
        int col = static_cast<int>(std::round(static_cast<double>(cardLeft - originX) / iconSpacingX));
        int targetX = originX + col * iconSpacingX;
        if (!isDuringDrag || std::abs(cardLeft - targetX) <= magneticDist)
        {
            bestLeft = targetX;
            snappedX = true;
        }
    }

    if (!snappedY)
    {
        int row = static_cast<int>(std::round(static_cast<double>(cardTop - originY) / iconSpacingY));
        int targetY = originY + row * iconSpacingY;
        if (!isDuringDrag || std::abs(cardTop - targetY) <= magneticDist)
        {
            bestTop = targetY;
            snappedY = true;
        }
    }

    // 屏幕安全边界约束
    if (bestLeft < workL) bestLeft = workL;
    if (bestLeft + cardW > workR) bestLeft = workR - cardW;
    if (bestTop < workT) bestTop = workT;
    if (bestTop + cardH > workB) bestTop = workB - cardH;

    return POINT{ bestLeft - padPx, bestTop - padPx };
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

    // 系统深浅色外观或全局设置变化通知
    case WM_SETTINGCHANGE:
    {
        if (ThemeManager::GetInstance().GetThemeMode() == AppThemeMode::System)
        {
            ThemeManager::GetInstance().CheckAndUpdateSystemTheme();
            WidgetManager::GetInstance().RefreshAll();
        }
        return 0;
    }

    case WM_SETCURSOR:
    {
        if (LOWORD(lParam) == HTCLIENT && m_widget && m_widget->WantsHandCursor())
        {
            SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32649))); // IDC_HAND
            return TRUE;
        }
        return DefWindowProcW(m_hwnd, uMsg, wParam, lParam);
    }

    // 鼠标离开窗口：清理内部按键悬浮态
    case WM_MOUSELEAVE:
    {
        if (m_widget)
        {
            m_widget->OnMouseLeave();
            RequestRender();
        }
        return 0;
    }

    // 鼠标左键按下：记录坐标并捕获输入
    case WM_LBUTTONDOWN:
    {
        m_isMouseDown = true;
        m_hasMovedSignificantly = false;
        m_dragStart.x = GET_X_LPARAM(lParam);
        m_dragStart.y = GET_Y_LPARAM(lParam);
        GetCursorPos(&m_dragStartScreen);
        m_initialWindowPos.x = m_x;
        m_initialWindowPos.y = m_y;
        SetCapture(m_hwnd);

        if (m_widget)
        {
            float dpiScale = m_renderer.GetDpiScale();
            float localX = static_cast<float>(m_dragStart.x) / dpiScale;
            float localY = static_cast<float>(m_dragStart.y) / dpiScale;
            if (m_widget->OnMouseDown(localX, localY))
            {
                RequestRender();
            }
        }
        return 0;
    }

    // 鼠标移动：基于全局屏幕坐标与起始偏移移动窗口，杜绝抖动，100% 顺滑跟手
    case WM_MOUSEMOVE:
    {
        if (m_isMouseDown)
        {
            // 如果已开启位置锁定，禁止拖拽移动
            if (ConfigManager::GetInstance().IsLockPosition())
            {
                return 0;
            }

            POINT cursor;
            GetCursorPos(&cursor);

            int dx = cursor.x - m_dragStartScreen.x;
            int dy = cursor.y - m_dragStartScreen.y;
            if (!m_hasMovedSignificantly && (std::abs(dx) > 6 || std::abs(dy) > 6))
            {
                m_hasMovedSignificantly = true;
            }

            if (m_hasMovedSignificantly)
            {
                int rawX = m_initialWindowPos.x + dx;
                int rawY = m_initialWindowPos.y + dy;

                // 拖拽过程中的磁吸吸附 (按住 Alt 键可临时解除吸附自由拖拽)
                bool snapActive = ConfigManager::GetInstance().IsSnapToGrid() && !(GetKeyState(VK_MENU) & 0x8000);
                if (snapActive)
                {
                    POINT snapped = SnapPosition(rawX, rawY, true);
                    m_x = snapped.x;
                    m_y = snapped.y;
                }
                else
                {
                    m_x = rawX;
                    m_y = rawY;
                }

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
        }
        else
        {
            // 追踪鼠标离开事件
            TRACKMOUSEEVENT tme{};
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = m_hwnd;
            TrackMouseEvent(&tme);

            if (m_widget)
            {
                float dpiScale = m_renderer.GetDpiScale();
                float localX = static_cast<float>(GET_X_LPARAM(lParam)) / dpiScale;
                float localY = static_cast<float>(GET_Y_LPARAM(lParam)) / dpiScale;
                if (m_widget->OnMouseMove(localX, localY))
                {
                    RequestRender();
                }
            }
        }
        return 0;
    }

    // 鼠标左键释放：未显著移动则判定为点击事件，否则结束拖动并保存坐标
    case WM_LBUTTONUP:
    {
        bool wasMouseDown = m_isMouseDown;
        bool wasDrag = m_hasMovedSignificantly;
        POINT clickPos = m_dragStart;

        m_isMouseDown = false;
        m_hasMovedSignificantly = false;
        ReleaseCapture();

        if (m_widget)
        {
            float dpiScale = m_renderer.GetDpiScale();
            float localX = static_cast<float>(GET_X_LPARAM(lParam)) / dpiScale;
            float localY = static_cast<float>(GET_Y_LPARAM(lParam)) / dpiScale;
            if (m_widget->OnMouseUp(localX, localY))
            {
                RequestRender();
            }
        }

        if (wasMouseDown)
        {
            if (!wasDrag && m_widget)
            {
                // 将窗口物理像素坐标转换为 DIP 坐标 (包含 padding 偏移, 与 Render 的 cardBounds 坐标系一致)
                float dpiScale = m_renderer.GetDpiScale();
                float localX = static_cast<float>(clickPos.x) / dpiScale;
                float localY = static_cast<float>(clickPos.y) / dpiScale;

                bool handled = m_widget->OnClick(localX, localY);
                if (handled)
                {
                    RequestRender();
                    return 0;
                }
            }

            if (wasDrag && m_widget)
            {
                // 释放鼠标时完成精准网格吸附与边界对齐
                bool snapActive = ConfigManager::GetInstance().IsSnapToGrid() && !(GetKeyState(VK_MENU) & 0x8000);
                if (snapActive)
                {
                    POINT finalSnapped = SnapPosition(m_x, m_y, false);
                    m_x = finalSnapped.x;
                    m_y = finalSnapped.y;

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

                ConfigManager::GetInstance().UpdateWidgetPosition(m_widget->GetId(), m_x, m_y);
                ConfigManager::GetInstance().Save();
            }
        }
        return 0;
    }

    // 捕获丢失保护 (仅在非正常释放时清理状态)
    case WM_CAPTURECHANGED:
    {
        if (m_isMouseDown)
        {
            m_isMouseDown = false;
            m_hasMovedSignificantly = false;
        }
        return 0;
    }

    // 鼠标右键：快捷菜单
    case WM_RBUTTONUP:
    {
        HMENU hMenu = CreatePopupMenu();
        HMENU hSubAdd = CreatePopupMenu();
        HMENU hSubAppTheme = CreatePopupMenu();
        HMENU hSubTheme = nullptr;

        AppendMenuW(hMenu, MF_STRING | MF_GRAYED, 1000, L"Windget 小组件");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

        // 全局深浅毛玻璃外观主题
        AppThemeMode curMode = ThemeManager::GetInstance().GetThemeMode();
        AppendMenuW(hSubAppTheme, MF_STRING | (curMode == AppThemeMode::System ? MF_CHECKED : 0), 1030, L"🌓 跟随系统 (System Default)");
        AppendMenuW(hSubAppTheme, MF_STRING | (curMode == AppThemeMode::Dark ? MF_CHECKED : 0), 1031, L"⬛ 深色毛玻璃 (Dark Frosted)");
        AppendMenuW(hSubAppTheme, MF_STRING | (curMode == AppThemeMode::Light ? MF_CHECKED : 0), 1032, L"⬜ 浅色毛玻璃 (Light Frosted)");
        AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hSubAppTheme), L"🎨 外观主题 (Theme)");

        // 如果是音乐小组件，提供自适应取色与经典黑曜外观切换
        if (m_widget && m_widget->GetType() == "music")
        {
            auto* pMusic = dynamic_cast<MusicWidget*>(m_widget.get());
            hSubTheme = CreatePopupMenu();
            bool isAdaptive = pMusic && pMusic->GetThemeMode() == MusicThemeMode::Adaptive;
            AppendMenuW(hSubTheme, MF_STRING | (isAdaptive ? 0 : MF_CHECKED), 1010, L"⬛ 经典黑曜 (Classic Dark)");
            AppendMenuW(hSubTheme, MF_STRING | (isAdaptive ? MF_CHECKED : 0), 1011, L"🌈 封面自适应取色 (Album Color)");
            AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hSubTheme), L"🎵 播放器配色 (Player Style)");
        }

        AppendMenuW(hSubAdd, MF_STRING, 1002, L"🕒 时钟组件 (Clock)");
        AppendMenuW(hSubAdd, MF_STRING, 1006, L"📅 日历组件 (Calendar)");
        AppendMenuW(hSubAdd, MF_STRING, 1007, L"⚡ 硬件监控 (System)");
        AppendMenuW(hSubAdd, MF_STRING, 1008, L"🎵 音乐控制器 (Music)");
        AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hSubAdd), L"➕ 添加小组件");

        // 桌面网格吸附与位置锁定
        bool isSnap = ConfigManager::GetInstance().IsSnapToGrid();
        bool isLocked = ConfigManager::GetInstance().IsLockPosition();
        AppendMenuW(hMenu, MF_STRING | (isSnap ? MF_CHECKED : 0), 1020, L"🧲 对齐到桌面网格 (Snap to Grid)");
        AppendMenuW(hMenu, MF_STRING | (isLocked ? MF_CHECKED : 0), 1021, L"🔒 锁定小组件位置 (Lock Position)");

        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
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

        if (hSubTheme) DestroyMenu(hSubTheme);
        DestroyMenu(hSubAppTheme);
        DestroyMenu(hSubAdd);
        DestroyMenu(hMenu);

        if (cmd == 1001)
        {
            PostQuitMessage(0);
        }
        else if (cmd == 1020)
        {
            bool snap = !ConfigManager::GetInstance().IsSnapToGrid();
            ConfigManager::GetInstance().SetSnapToGrid(snap);
            ConfigManager::GetInstance().Save();
            if (snap)
            {
                POINT snapped = SnapPosition(m_x, m_y, false);
                m_x = snapped.x;
                m_y = snapped.y;
                SetWindowPos(m_hwnd, nullptr, m_x, m_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                ConfigManager::GetInstance().UpdateWidgetPosition(m_widget->GetId(), m_x, m_y);
                ConfigManager::GetInstance().Save();
            }
        }
        else if (cmd == 1021)
        {
            bool lock = !ConfigManager::GetInstance().IsLockPosition();
            ConfigManager::GetInstance().SetLockPosition(lock);
            ConfigManager::GetInstance().Save();
        }
        else if (cmd == 1030)
        {
            ThemeManager::GetInstance().SetThemeMode(AppThemeMode::System);
            ConfigManager::GetInstance().SetThemeMode("system");
            ConfigManager::GetInstance().Save();
            WidgetManager::GetInstance().RefreshAll();
        }
        else if (cmd == 1031)
        {
            ThemeManager::GetInstance().SetThemeMode(AppThemeMode::Dark);
            ConfigManager::GetInstance().SetThemeMode("dark");
            ConfigManager::GetInstance().Save();
            WidgetManager::GetInstance().RefreshAll();
        }
        else if (cmd == 1032)
        {
            ThemeManager::GetInstance().SetThemeMode(AppThemeMode::Light);
            ConfigManager::GetInstance().SetThemeMode("light");
            ConfigManager::GetInstance().Save();
            WidgetManager::GetInstance().RefreshAll();
        }
        else if (cmd == 1010)
        {
            auto* pMusic = dynamic_cast<MusicWidget*>(m_widget.get());
            if (pMusic)
            {
                pMusic->SetThemeMode(MusicThemeMode::Dark);
                RequestRender();
            }
        }
        else if (cmd == 1011)
        {
            auto* pMusic = dynamic_cast<MusicWidget*>(m_widget.get());
            if (pMusic)
            {
                pMusic->SetThemeMode(MusicThemeMode::Adaptive);
                RequestRender();
            }
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
        else if (cmd == 1008)
        {
            WidgetManager::GetInstance().AddWidgetAtDefaultPosition("music");
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
        DesktopManager::GetInstance().DetachFromDesktop(m_hwnd);
        return 0;
    }

    default:
        break;
    }

    return DefWindowProcW(m_hwnd, uMsg, wParam, lParam);
}

} // namespace Windget
