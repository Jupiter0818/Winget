#pragma once

#include <windows.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <string>

namespace Windget {

using Microsoft::WRL::ComPtr;
class IWidget;

class D2DRenderer
{
public:
    D2DRenderer();
    ~D2DRenderer();

    // 初始化 Direct2D 和 DirectWrite 资源及位图缓冲区
    bool Initialize(HWND hwnd, float cardWidth, float cardHeight);

    // 释放全部图形资源
    void Cleanup();

    // 绘制并向 DWM 提交分层窗口 (包含阴影、卡片背景及小组件内容)
    void RenderWidget(HWND hwnd, IWidget* pWidget);

    // 更新 DPI 缩放并重新计算缓冲区
    void UpdateDpi(HWND hwnd, float dpi, float cardWidth, float cardHeight);

    // 获取当前物理像素宽高 (包含阴影区域)
    int GetPhysicalWidth() const { return m_pixelWidth; }
    int GetPhysicalHeight() const { return m_pixelHeight; }

    // 获取阴影外边距 (DIP)
    float GetPadding() const { return PADDING; }
    float GetDpiScale() const { return m_dpiScale; }

    // 获取共享 DirectWrite 工厂接口
    IDWriteFactory* GetDWriteFactory() const { return m_dwriteFactory.Get(); }

private:
    bool CreateDeviceIndependentResources();
    bool CreateDeviceResources();
    bool CreateBitmapBuffer(HWND hwnd, int width, int height);
    void DiscardDeviceResources();

private:
    static constexpr float PADDING = 2.0f;        // 剔除外扩散投影，仅保留 2px 边缘亚像素抗锯齿缓冲
    static constexpr float CORNER_RADIUS = 22.0f; // macOS 风格圆角半径

    // 工厂接口
    ComPtr<ID2D1Factory> m_d2dFactory;
    ComPtr<IDWriteFactory> m_dwriteFactory;

    // Direct2D 渲染目标与毛玻璃画刷
    ComPtr<ID2D1DCRenderTarget> m_dcRenderTarget;
    ComPtr<ID2D1LinearGradientBrush> m_frostedGlassBrush;
    ComPtr<ID2D1LinearGradientBrush> m_glassBorderBrush;
    ComPtr<ID2D1SolidColorBrush> m_glassHighlightBrush;

    // 内存位图与设备上下文 (用于 32-bit ARGB UpdateLayeredWindow)
    HDC m_memDC = nullptr;
    HBITMAP m_memBitmap = nullptr;
    HBITMAP m_oldBitmap = nullptr;
    void* m_bitmapBits = nullptr;

    float m_cardWidth = 320.0f;
    float m_cardHeight = 170.0f;

    int m_pixelWidth = 0;
    int m_pixelHeight = 0;
    float m_dpi = 96.0f;
    float m_dpiScale = 1.0f;
};

// 辅助函数：安全获取窗口当前 DPI
float GetWindowDpi(HWND hwnd);

} // namespace Windget
