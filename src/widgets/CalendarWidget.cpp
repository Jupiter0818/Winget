#include "widgets/CalendarWidget.h"
#include <ctime>
#include <string>

namespace Windget {

CalendarWidget::CalendarWidget(std::string id)
    : m_id(std::move(id))
{
    RefreshCalendarData();
}

CalendarWidget::~CalendarWidget()
{
    Shutdown();
}

void CalendarWidget::RefreshCalendarData()
{
    std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);

    m_year = local.tm_year + 1900;
    m_month = local.tm_mon;
    m_mday = local.tm_mday;
    m_wday = local.tm_wday;

    // 月份全称 (英文大写 macOS 风格)
    static const wchar_t* MONTH_NAMES[] = {
        L"JANUARY", L"FEBRUARY", L"MARCH", L"APRIL",
        L"MAY", L"JUNE", L"JULY", L"AUGUST",
        L"SEPTEMBER", L"OCTOBER", L"NOVEMBER", L"DECEMBER"
    };
    m_monthName = MONTH_NAMES[m_month];

    // 星期全称
    static const wchar_t* WEEKDAY_NAMES[] = {
        L"SUNDAY", L"MONDAY", L"TUESDAY", L"WEDNESDAY",
        L"THURSDAY", L"FRIDAY", L"SATURDAY"
    };
    m_weekdayName = WEEKDAY_NAMES[m_wday];

    m_dayNumberStr = std::to_wstring(m_mday);

    // 计算当月第一天星期几
    std::tm firstDay = local;
    firstDay.tm_mday = 1;
    firstDay.tm_hour = 0;
    firstDay.tm_min = 0;
    firstDay.tm_sec = 0;
    std::mktime(&firstDay);
    m_firstDayWday = firstDay.tm_wday;

    // 计算当月天数
    if (m_month == 3 || m_month == 5 || m_month == 8 || m_month == 10)
    {
        m_daysInMonth = 30;
    }
    else if (m_month == 1) // 2月
    {
        bool isLeap = (m_year % 4 == 0 && m_year % 100 != 0) || (m_year % 400 == 0);
        m_daysInMonth = isLeap ? 29 : 28;
    }
    else
    {
        m_daysInMonth = 31;
    }
}

void CalendarWidget::Initialize(IDWriteFactory* dwriteFactory)
{
    if (!dwriteFactory)
    {
        return;
    }

    // 1. 月份小标题 (大写红色粗体)
    dwriteFactory->CreateTextFormat(
        L"Segoe UI Variable Display",
        nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.5f,
        L"",
        m_monthHeaderFormat.GetAddressOf()
    );
    if (!m_monthHeaderFormat)
    {
        dwriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_SEMI_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            12.5f,
            L"",
            m_monthHeaderFormat.GetAddressOf()
        );
    }
    if (m_monthHeaderFormat)
    {
        m_monthHeaderFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_monthHeaderFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    // 2. 超大号今天日期数字 (44pt)
    dwriteFactory->CreateTextFormat(
        L"Segoe UI Variable Display",
        nullptr,
        DWRITE_FONT_WEIGHT_LIGHT,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        48.0f,
        L"",
        m_bigDayFormat.GetAddressOf()
    );
    if (!m_bigDayFormat)
    {
        dwriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_LIGHT,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            48.0f,
            L"",
            m_bigDayFormat.GetAddressOf()
        );
    }
    if (m_bigDayFormat)
    {
        m_bigDayFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_bigDayFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    // 3. 星期文本
    dwriteFactory->CreateTextFormat(
        L"Segoe UI Variable Text",
        nullptr,
        DWRITE_FONT_WEIGHT_MEDIUM,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.0f,
        L"",
        m_weekdayFormat.GetAddressOf()
    );
    if (m_weekdayFormat)
    {
        m_weekdayFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_weekdayFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    // 4. 网格头部星期 (S M T W T F S)
    dwriteFactory->CreateTextFormat(
        L"Segoe UI Variable Text",
        nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        10.5f,
        L"",
        m_gridHeaderFormat.GetAddressOf()
    );
    if (m_gridHeaderFormat)
    {
        m_gridHeaderFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_gridHeaderFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    // 5. 网格日期数字
    dwriteFactory->CreateTextFormat(
        L"Segoe UI Variable Text",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        11.5f,
        L"",
        m_gridDayFormat.GetAddressOf()
    );
    if (m_gridDayFormat)
    {
        m_gridDayFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_gridDayFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    RefreshCalendarData();
}

void CalendarWidget::Update()
{
    RefreshCalendarData();
}

void CalendarWidget::Render(
    ID2D1DCRenderTarget* target,
    IDWriteFactory* /*dwriteFactory*/,
    const D2D1_RECT_F& cardBounds
)
{
    if (!target)
    {
        return;
    }

    // 初始化画刷
    if (!m_accentBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(1.0f, 0.28f, 0.24f, 1.0f), // macOS 经典番茄红
            m_accentBrush.GetAddressOf()
        );
    }
    if (!m_textPrimaryBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.95f),
            m_textPrimaryBrush.GetAddressOf()
        );
    }
    if (!m_textSecondaryBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.45f),
            m_textSecondaryBrush.GetAddressOf()
        );
    }
    if (!m_dividerBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.12f),
            m_dividerBrush.GetAddressOf()
        );
    }
    if (!m_todayCircleBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(1.0f, 0.28f, 0.24f, 0.90f),
            m_todayCircleBrush.GetAddressOf()
        );
    }

    // ==========================================
    // 1. 左侧区域：大号日期展示区 (宽度 ~110px)
    // ==========================================
    float leftAreaWidth = 110.0f;
    float left = cardBounds.left;
    float top = cardBounds.top;

    // 月份标签 (例如 NOVEMBER)
    if (m_monthHeaderFormat && m_accentBrush)
    {
        D2D1_RECT_F monthRect = D2D1::RectF(
            left + 6.0f,
            top + 24.0f,
            left + leftAreaWidth,
            top + 44.0f
        );
        target->DrawText(
            m_monthName.c_str(),
            static_cast<UINT32>(m_monthName.length()),
            m_monthHeaderFormat.Get(),
            monthRect,
            m_accentBrush.Get()
        );
    }

    // 大号今天日期 (例如 1)
    if (m_bigDayFormat && m_textPrimaryBrush)
    {
        D2D1_RECT_F dayRect = D2D1::RectF(
            left + 6.0f,
            top + 46.0f,
            left + leftAreaWidth,
            top + 112.0f
        );
        target->DrawText(
            m_dayNumberStr.c_str(),
            static_cast<UINT32>(m_dayNumberStr.length()),
            m_bigDayFormat.Get(),
            dayRect,
            m_textPrimaryBrush.Get()
        );
    }

    // 星期全称 (例如 SUNDAY)
    if (m_weekdayFormat && m_textSecondaryBrush)
    {
        D2D1_RECT_F weekdayRect = D2D1::RectF(
            left + 6.0f,
            top + 114.0f,
            left + leftAreaWidth,
            top + 138.0f
        );
        target->DrawText(
            m_weekdayName.c_str(),
            static_cast<UINT32>(m_weekdayName.length()),
            m_weekdayFormat.Get(),
            weekdayRect,
            m_textSecondaryBrush.Get()
        );
    }

    // ==========================================
    // 2. 竖向半透明分割线
    // ==========================================
    float dividerX = left + 114.0f;
    if (m_dividerBrush)
    {
        target->DrawLine(
            D2D1::Point2F(dividerX, top + 22.0f),
            D2D1::Point2F(dividerX, cardBounds.bottom - 22.0f),
            m_dividerBrush.Get(),
            1.0f
        );
    }

    // ==========================================
    // 3. 右侧区域：整齐月历网格 (宽度 ~180px)
    // ==========================================
    float gridLeft = dividerX + 12.0f;
    float gridTop = top + 18.0f;
    float gridWidth = 176.0f;
    float colWidth = gridWidth / 7.0f;
    float rowHeight = 19.5f;

    // 绘制星期表头 (S M T W T F S)
    static const wchar_t* HEADERS[] = { L"S", L"M", L"T", L"W", L"T", L"F", L"S" };
    for (int col = 0; col < 7; ++col)
    {
        D2D1_RECT_F headerRect = D2D1::RectF(
            gridLeft + col * colWidth,
            gridTop,
            gridLeft + (col + 1) * colWidth,
            gridTop + rowHeight
        );

        target->DrawText(
            HEADERS[col],
            1,
            m_gridHeaderFormat.Get(),
            headerRect,
            m_textSecondaryBrush.Get()
        );
    }

    // 绘制当前月所有日期数字
    float daysBaseTop = gridTop + rowHeight + 2.0f;
    for (int day = 1; day <= m_daysInMonth; ++day)
    {
        int col = (m_firstDayWday + day - 1) % 7;
        int row = (m_firstDayWday + day - 1) / 7;

        float cellX = gridLeft + col * colWidth;
        float cellY = daysBaseTop + row * rowHeight;

        D2D1_RECT_F cellRect = D2D1::RectF(
            cellX,
            cellY,
            cellX + colWidth,
            cellY + rowHeight
        );

        // 如果是“今天”，绘制红色圆形底徽章
        if (day == m_mday && m_todayCircleBrush)
        {
            float centerX = cellX + colWidth / 2.0f;
            float centerY = cellY + rowHeight / 2.0f;
            D2D1_ELLIPSE ellipse = D2D1::Ellipse(
                D2D1::Point2F(centerX, centerY),
                8.8f,
                8.8f
            );
            target->FillEllipse(ellipse, m_todayCircleBrush.Get());
        }

        std::wstring dayStr = std::to_wstring(day);
        ID2D1SolidColorBrush* brush = (day == m_mday) ? m_textPrimaryBrush.Get() : m_textPrimaryBrush.Get();
        if (day != m_mday)
        {
            brush = (col == 0 || col == 6) ? m_textSecondaryBrush.Get() : m_textPrimaryBrush.Get();
        }

        target->DrawText(
            dayStr.c_str(),
            static_cast<UINT32>(dayStr.length()),
            m_gridDayFormat.Get(),
            cellRect,
            brush
        );
    }
}

void CalendarWidget::Shutdown()
{
    m_accentBrush.Reset();
    m_textPrimaryBrush.Reset();
    m_textSecondaryBrush.Reset();
    m_dividerBrush.Reset();
    m_todayCircleBrush.Reset();

    m_monthHeaderFormat.Reset();
    m_bigDayFormat.Reset();
    m_weekdayFormat.Reset();
    m_gridHeaderFormat.Reset();
    m_gridDayFormat.Reset();
}

} // namespace Windget
