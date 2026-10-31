#include "widgets/ClockWidget.h"
#include <ctime>

namespace Windget {

ClockWidget::ClockWidget(std::string id)
    : m_id(std::move(id))
{
    RefreshTimeAndDate();
}

ClockWidget::~ClockWidget()
{
    Shutdown();
}

void ClockWidget::RefreshTimeAndDate()
{
    std::time_t now = std::time(nullptr);
    std::tm localTime{};
    localtime_s(&localTime, &now);

    // 格式化时间 (例如 22:38)
    wchar_t timeBuf[32];
    std::wcsftime(timeBuf, sizeof(timeBuf) / sizeof(wchar_t), L"%H:%M", &localTime);
    m_timeString = timeBuf;

    // 格式化日期 (例如 Saturday, October 31)
    wchar_t dateBuf[64];
    std::wcsftime(dateBuf, sizeof(dateBuf) / sizeof(wchar_t), L"%A, %B %d", &localTime);
    m_dateString = dateBuf;
}

void ClockWidget::Initialize(IDWriteFactory* dwriteFactory)
{
    if (!dwriteFactory)
    {
        return;
    }

    // 1. 创建时间文本格式 (优先 Segoe UI Variable Display，回退 Segoe UI)
    HRESULT hr = dwriteFactory->CreateTextFormat(
        L"Segoe UI Variable Display",
        nullptr,
        DWRITE_FONT_WEIGHT_LIGHT,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        56.0f,
        L"",
        m_timeFormat.GetAddressOf()
    );

    if (FAILED(hr) || !m_timeFormat)
    {
        dwriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_LIGHT,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            56.0f,
            L"",
            m_timeFormat.GetAddressOf()
        );
    }

    if (m_timeFormat)
    {
        m_timeFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_timeFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    // 2. 创建日期文本格式 (优先 Segoe UI Variable Text，回退 Segoe UI)
    hr = dwriteFactory->CreateTextFormat(
        L"Segoe UI Variable Text",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        16.5f,
        L"",
        m_dateFormat.GetAddressOf()
    );

    if (FAILED(hr) || !m_dateFormat)
    {
        dwriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            16.5f,
            L"",
            m_dateFormat.GetAddressOf()
        );
    }

    if (m_dateFormat)
    {
        m_dateFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_dateFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    RefreshTimeAndDate();
}

void ClockWidget::Update()
{
    RefreshTimeAndDate();
}

void ClockWidget::Render(
    ID2D1DCRenderTarget* target,
    IDWriteFactory* /*dwriteFactory*/,
    const D2D1_RECT_F& cardBounds
)
{
    if (!target)
    {
        return;
    }

    // 确保文本画刷有效
    if (!m_timeBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.96f),
            m_timeBrush.GetAddressOf()
        );
    }

    if (!m_dateBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(0.92f, 0.92f, 0.96f, 0.62f),
            m_dateBrush.GetAddressOf()
        );
    }

    // 绘制时间文本
    if (m_timeFormat && m_timeBrush)
    {
        D2D1_RECT_F timeRect = D2D1::RectF(
            cardBounds.left,
            cardBounds.top + 20.0f,
            cardBounds.right,
            cardBounds.top + 98.0f
        );

        target->DrawText(
            m_timeString.c_str(),
            static_cast<UINT32>(m_timeString.length()),
            m_timeFormat.Get(),
            timeRect,
            m_timeBrush.Get()
        );
    }

    // 绘制日期文本
    if (m_dateFormat && m_dateBrush)
    {
        D2D1_RECT_F dateRect = D2D1::RectF(
            cardBounds.left,
            cardBounds.top + 104.0f,
            cardBounds.right,
            cardBounds.top + 142.0f
        );

        target->DrawText(
            m_dateString.c_str(),
            static_cast<UINT32>(m_dateString.length()),
            m_dateFormat.Get(),
            dateRect,
            m_dateBrush.Get()
        );
    }
}

void ClockWidget::Shutdown()
{
    m_timeBrush.Reset();
    m_dateBrush.Reset();
    m_timeFormat.Reset();
    m_dateFormat.Reset();
}

} // namespace Windget
