#include "graphics/D2DRenderer.h"
#include "widgets/IWidget.h"
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

    // 创建通用卡片背景与阴影画刷
    m_dcRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(0.11f, 0.11f, 0.13f, 0.85f),
        m_cardBrush.GetAddressOf()
    );

    m_dcRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.10f),
        m_borderBrush.GetAddressOf()
    );

    m_dcRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.05f),
        m_shadowBrush.GetAddressOf()
    );

    return true;
}

void D2DRenderer::DiscardDeviceResources()
{
    m_cardBrush.Reset();
    m_borderBrush.Reset();
    m_shadowBrush.Reset();
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

    // 2. 柔和环境投影 (多层叠加产生自然的模糊散色与下沉感)
    if (m_shadowBrush)
    {
        for (int i = 8; i >= 1; --i)
        {
            float spread = static_cast<float>(i) * 1.5f;
            float offsetY = static_cast<float>(i) * 0.75f + 1.5f;
            float alpha = 0.015f * static_cast<float>(9 - i);
            m_shadowBrush->SetColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, alpha));

            D2D1_ROUNDED_RECT shadowRect = D2D1::RoundedRect(
                D2D1::RectF(
                    cardLeft - spread,
                    cardTop - spread + offsetY,
                    cardRight + spread,
                    cardBottom + spread + offsetY
                ),
                CORNER_RADIUS + spread * 0.5f,
                CORNER_RADIUS + spread * 0.5f
            );
            m_dcRenderTarget->FillRoundedRectangle(shadowRect, m_shadowBrush.Get());
        }
    }

    // 3. 绘制卡片背景 (macOS 风格深色半透明圆角矩形)
    D2D1_ROUNDED_RECT cardRect = D2D1::RoundedRect(
        cardBounds,
        CORNER_RADIUS,
        CORNER_RADIUS
    );

    if (m_cardBrush)
    {
        m_cardBrush->SetColor(D2D1::ColorF(0.11f, 0.11f, 0.13f, 0.85f));
        m_dcRenderTarget->FillRoundedRectangle(cardRect, m_cardBrush.Get());
    }

    // 4. 精致玻璃微光边框 (1px 白色半透明)
    if (m_borderBrush)
    {
        m_borderBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.10f));
        m_dcRenderTarget->DrawRoundedRectangle(cardRect, m_borderBrush.Get(), 1.0f);
    }

    // 5. 调用具体 Widget 自身的渲染逻辑
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
