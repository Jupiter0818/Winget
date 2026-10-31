#pragma once

#include <windows.h>
#include <memory>
#include "widgets/IWidget.h"
#include "graphics/D2DRenderer.h"

namespace Windget {

class WidgetWindow
{
public:
    explicit WidgetWindow(std::shared_ptr<IWidget> widget);
    ~WidgetWindow();

    // 创建原生分层桌面窗口并关联 Widget 实例
    bool Create(HINSTANCE hInstance, int x, int y);

    // 显示与隐藏
    void Show();
    void Hide();
    void Close();

    // 重新渲染当前帧
    void RequestRender();

    // 获取内部属性
    HWND GetHwnd() const { return m_hwnd; }
    IWidget* GetWidget() const { return m_widget.get(); }
    int GetX() const { return m_x; }
    int GetY() const { return m_y; }

    // 注册全局窗口类 (单次初始化)
    static bool RegisterWindowClass(HINSTANCE hInstance);

private:
    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    static constexpr wchar_t WINDOW_CLASS_NAME[] = L"WindgetWidgetHostWindow";
    static bool s_classRegistered;

    HWND m_hwnd = nullptr;
    std::shared_ptr<IWidget> m_widget;
    D2DRenderer m_renderer;

    int m_x = 200;
    int m_y = 200;

    bool m_isDragging = false;
    POINT m_dragStart{};
    UINT_PTR m_timerId = 1;
};

} // namespace Windget
