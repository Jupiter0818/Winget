#include "graphics/D2DRenderer.h"
#include "widgets/IWidget.h"
#include "core/ThemeManager.h"
#include <cmath>

namespace Windget {

float GetWindowDpi(HWND hwnd)
{
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32)
    {
        typedef UINT(WINAPI *GetDpiForWindowProc)(HWND);
        auto getDpi = (GetDpiForWindowProc)GetProcAddress(user32, "GetDpiForWindow");
        if (getDpi && hwnd)
        {
            UINT dpi = getDpi(hwnd);
            if (dpi > 0)
            {
                return static_cast<float>(dpi);
            }
        }
    }

    HDC screen = GetDC(nullptr);
    float dpi = 96.0f;
    if (screen)
    {
        dpi = static_cast<float>(GetDeviceCaps(screen, LOGPIXELSY));
        ReleaseDC(nullptr, screen);
    }
    return dpi > 0.0f ? dpi : 96.0f;
}

D2DRenderer::D2DRenderer() = default;

D2DRenderer::~D2DRenderer()
{
    Cleanup();
}

bool D2DRenderer::Initialize(HWND hwnd, float cardWidth, float cardHeight)
{
    m_cardWidth = cardWidth;
    m_cardHeight = cardHeight;

    if (!CreateDeviceIndependentResources())
    {
        return false;
    }

    float initialDpi = GetWindowDpi(hwnd);
    UpdateDpi(hwnd, initialDpi, cardWidth, cardHeight);

    return CreateDeviceResources();
}

void D2DRenderer::Cleanup()
{
    DiscardDeviceResources();

    m_dwriteFactory.Reset();
    m_d2dFactory.Reset();

    if (m_memDC)
    {
        if (m_oldBitmap)
        {
            SelectObject(m_memDC, m_oldBitmap);
            m_oldBitmap = nullptr;
        }
        if (m_memBitmap)
        {
            DeleteObject(m_memBitmap);
            m_memBitmap = nullptr;
        }
        DeleteDC(m_memDC);
        m_memDC = nullptr;
    }
    m_bitmapBits = nullptr;
}

bool D2DRenderer::CreateDeviceIndependentResources()
{
    if (m_d2dFactory && m_dwriteFactory)
    {
        return true;
    }

    // 1. 创建 Direct2D 工厂
    HRESULT hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        m_d2dFactory.GetAddressOf()
    );
    if (FAILED(hr))
    {
        return false;
    }

    // 2. 创建 DirectWrite 工厂
    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf())
    );
    return SUCCEEDED(hr);
}

bool D2DRenderer::CreateBitmapBuffer(HWND hwnd, int width, int height)
{
    if (m_memDC)
    {
        if (m_oldBitmap)
        {
            SelectObject(m_memDC, m_oldBitmap);
            m_oldBitmap = nullptr;
        }
        if (m_memBitmap)
        {
            DeleteObject(m_memBitmap);
            m_memBitmap = nullptr;
        }
        DeleteDC(m_memDC);
        m_memDC = nullptr;
    }

    HDC screenDC = GetDC(hwnd);
    m_memDC = CreateCompatibleDC(screenDC);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // 顶部向下的 DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    m_memBitmap = CreateDIBSection(
        m_memDC,
        &bmi,
        DIB_RGB_COLORS,
        &m_bitmapBits,
        nullptr,
        0
    );

    m_oldBitmap = static_cast<HBITMAP>(SelectObject(m_memDC, m_memBitmap));
    ReleaseDC(hwnd, screenDC);

    return m_memBitmap != nullptr;
}

bool D2DRenderer::CreateDeviceResources()
{
    if (m_dcRenderTarget)
    {
        return true;
    }

    if (!m_d2dFactory)
    {
        return false;
    }

    // 创建透明预乘 Alpha 支持的 DC 渲染目标
    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(
            DXGI_FORMAT_B8G8R8A8_UNORM,
            D2D1_ALPHA_MODE_PREMULTIPLIED
        ),
        0.0f,
        0.0f,
        D2D1_RENDER_TARGET_USAGE_NONE,
        D2D1_FEATURE_LEVEL_DEFAULT
    );

    HRESULT hr = m_d2dFactory->CreateDCRenderTarget(
        &props,
        m_dcRenderTarget.GetAddressOf()
    );
    if (FAILED(hr))
    {
        return false;
    }

    // 创建通用毛玻璃微光画刷
    m_dcRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.20f),
        m_glassHighlightBrush.GetAddressOf()
    );

    return true;
}

void D2DRenderer::DiscardDeviceResources()
{
    m_frostedGlassBrush.Reset();
    m_glassBorderBrush.Reset();
    m_glassHighlightBrush.Reset();
    m_dcRenderTarget.Reset();
}

void D2DRenderer::UpdateDpi(HWND hwnd, float dpi, float cardWidth, float cardHeight)
{
    m_cardWidth = cardWidth;
    m_cardHeight = cardHeight;
    m_dpi = (dpi > 0.0f) ? dpi : 96.0f;
    m_dpiScale = m_dpi / 96.0f;

    const float totalWidth = m_cardWidth + PADDING * 2.0f;
    const float totalHeight = m_cardHeight + PADDING * 2.0f;

    int newPixelWidth = static_cast<int>(std::ceil(totalWidth * m_dpiScale));
    int newPixelHeight = static_cast<int>(std::ceil(totalHeight * m_dpiScale));

    if (newPixelWidth != m_pixelWidth || newPixelHeight != m_pixelHeight || !m_memBitmap)
    {
        m_pixelWidth = newPixelWidth;
        m_pixelHeight = newPixelHeight;
        CreateBitmapBuffer(hwnd, m_pixelWidth, m_pixelHeight);
        DiscardDeviceResources();
    }
}

void D2DRenderer::RenderWidget(HWND hwnd, IWidget* pWidget)
{
    if (!CreateDeviceResources())
    {
        return;
    }

    if (!m_memDC || !m_dcRenderTarget)
    {
        return;
    }

    // 绑定内存 DC 到 Direct2D RenderTarget
    RECT rc{ 0, 0, m_pixelWidth, m_pixelHeight };
    HRESULT hr = m_dcRenderTarget->BindDC(m_memDC, &rc);
    if (FAILED(hr))
    {
        return;
    }

    m_dcRenderTarget->SetDpi(m_dpi, m_dpi);
    m_dcRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

    m_dcRenderTarget->BeginDraw();

    // 1. 全透明清屏
    m_dcRenderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

    const float cardLeft = PADDING;
    const float cardTop = PADDING;
    const float cardRight = cardLeft + m_cardWidth;
    const float cardBottom = cardTop + m_cardHeight;
    const D2D1_RECT_F cardBounds = D2D1::RectF(cardLeft, cardTop, cardRight, cardBottom);

    // 2. 彻底去除外扩散阴影，直接渲染纯净现代的毛玻璃质感底色
    bool isDark = ThemeManager::GetInstance().IsDarkMode();

    // 构建动态毛玻璃背景画刷
    D2D1_GRADIENT_STOP stops[2];
    if (isDark)
    {
        // 深色毛玻璃：剔透微透黑曜石，左上柔光微亮，右下沉稳深邃
        stops[0].position = 0.0f;
        stops[0].color = D2D1::ColorF(0.12f, 0.13f, 0.17f, 0.78f);
        stops[1].position = 1.0f;
        stops[1].color = D2D1::ColorF(0.07f, 0.08f, 0.10f, 0.88f);
    }
    else
    {
        // 浅色毛玻璃：纯净剔透牛奶白，完美契合 Windows 11 Fluent 现代光感
        stops[0].position = 0.0f;
        stops[0].color = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.84f);
        stops[1].position = 1.0f;
        stops[1].color = D2D1::ColorF(0.93f, 0.95f, 0.98f, 0.76f);
    }

    ComPtr<ID2D1GradientStopCollection> stopCollection;
    m_dcRenderTarget->CreateGradientStopCollection(
        stops,
        2,
        D2D1_GAMMA_2_2,
        D2D1_EXTEND_MODE_CLAMP,
        stopCollection.GetAddressOf()
    );

    m_frostedGlassBrush.Reset();
    m_dcRenderTarget->CreateLinearGradientBrush(
        D2D1::LinearGradientBrushProperties(
            D2D1::Point2F(cardBounds.left, cardBounds.top),
            D2D1::Point2F(cardBounds.right, cardBounds.bottom)
        ),
        stopCollection.Get(),
        m_frostedGlassBrush.GetAddressOf()
    );

    // 3. 构建高光折射玻璃微边框画刷 (Specular Glass Bevel)
    D2D1_GRADIENT_STOP borderStops[2];
    if (isDark)
    {
        borderStops[0].position = 0.0f;
        borderStops[0].color = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.28f);
        borderStops[1].position = 1.0f;
        borderStops[1].color = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.08f);
    }
    else
    {
        borderStops[0].position = 0.0f;
        borderStops[0].color = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.95f);
        borderStops[1].position = 1.0f;
        borderStops[1].color = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.12f);
    }

    ComPtr<ID2D1GradientStopCollection> borderStopCollection;
    m_dcRenderTarget->CreateGradientStopCollection(
        borderStops,
        2,
        D2D1_GAMMA_2_2,
        D2D1_EXTEND_MODE_CLAMP,
        borderStopCollection.GetAddressOf()
    );

    m_glassBorderBrush.Reset();
    m_dcRenderTarget->CreateLinearGradientBrush(
        D2D1::LinearGradientBrushProperties(
            D2D1::Point2F(cardBounds.left, cardBounds.top),
            D2D1::Point2F(cardBounds.right, cardBounds.bottom)
        ),
        borderStopCollection.Get(),
        m_glassBorderBrush.GetAddressOf()
    );

    // 4. 绘制毛玻璃圆角卡片背景
    D2D1_ROUNDED_RECT cardRect = D2D1::RoundedRect(
        cardBounds,
        CORNER_RADIUS,
        CORNER_RADIUS
    );

    if (m_frostedGlassBrush)
    {
        m_dcRenderTarget->FillRoundedRectangle(cardRect, m_frostedGlassBrush.Get());
    }

    // 5. 绘制玻璃边框
    if (m_glassBorderBrush)
    {
        m_dcRenderTarget->DrawRoundedRectangle(cardRect, m_glassBorderBrush.Get(), 1.0f);
    }

    // 6. 绘制玻璃顶层微妙折射微光线 (模拟真实玻璃厚度内部微折射)
    if (m_glassHighlightBrush)
    {
        m_glassHighlightBrush->SetColor(isDark ? D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.12f) : D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.65f));
        D2D1_RECT_F innerHighlight = D2D1::RectF(
            cardBounds.left + CORNER_RADIUS * 0.8f,
            cardBounds.top + 1.0f,
            cardBounds.right - CORNER_RADIUS * 0.8f,
            cardBounds.top + 2.0f
        );
        m_dcRenderTarget->FillRectangle(innerHighlight, m_glassHighlightBrush.Get());
    }

    // 7. 调用具体 Widget 自身的渲染逻辑
    if (pWidget)
    {
        pWidget->Render(m_dcRenderTarget.Get(), m_dwriteFactory.Get(), cardBounds);
    }

    hr = m_dcRenderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
    {
        DiscardDeviceResources();
        return;
    }

    // 6. 将 Direct2D 渲染结果通过 UpdateLayeredWindow 输出到桌面，实现真 32-bit Alpha 混合
    RECT windowRect{};
    GetWindowRect(hwnd, &windowRect);

    POINT ptDst = { windowRect.left, windowRect.top };
    HWND hParent = GetParent(hwnd);
    if (hParent)
    {
        ScreenToClient(hParent, &ptDst);
    }

    SIZE sizeWnd = { m_pixelWidth, m_pixelHeight };
    POINT ptSrc = { 0, 0 };

    BLENDFUNCTION blend{};
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    HDC screenDC = GetDC(nullptr);
    UpdateLayeredWindow(
        hwnd,
        screenDC,
        &ptDst,
        &sizeWnd,
        m_memDC,
        &ptSrc,
        0,
        &blend,
        ULW_ALPHA
    );
    ReleaseDC(nullptr, screenDC);
}

} // namespace Windget
