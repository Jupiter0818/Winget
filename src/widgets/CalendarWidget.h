#pragma once

#include "widgets/IWidget.h"
#include <wrl/client.h>
#include <vector>

namespace Windget {

using Microsoft::WRL::ComPtr;

class CalendarWidget : public IWidget
{
public:
    explicit CalendarWidget(std::string id = "calendar_default");
    ~CalendarWidget() override;

    std::string GetId() const override { return m_id; }
    std::string GetType() const override { return "calendar"; }

    float GetWidth() const override { return 320.0f; }
    float GetHeight() const override { return 170.0f; }
    uint32_t GetUpdateIntervalMs() const override { return 10000; } // 10秒检查一次日期

    void Initialize(IDWriteFactory* dwriteFactory) override;
    void Update() override;
    void Render(
        ID2D1DCRenderTarget* target,
        IDWriteFactory* dwriteFactory,
        const D2D1_RECT_F& cardBounds
    ) override;
    void Shutdown() override;

private:
    void RefreshCalendarData();

private:
    std::string m_id;

    // 当前日期状态
    int m_year = 2026;
    int m_month = 0;       // 0 - 11
    int m_mday = 1;        // 1 - 31
    int m_wday = 0;        // 0 - 6 (0=Sun)
    int m_firstDayWday = 0;// 本月1日星期几
    int m_daysInMonth = 31;// 本月总天数

    std::wstring m_monthName;
    std::wstring m_dayNumberStr;
    std::wstring m_weekdayName;

    // DirectWrite 文本格式
    ComPtr<IDWriteTextFormat> m_monthHeaderFormat;
    ComPtr<IDWriteTextFormat> m_bigDayFormat;
    ComPtr<IDWriteTextFormat> m_weekdayFormat;
    ComPtr<IDWriteTextFormat> m_gridHeaderFormat;
    ComPtr<IDWriteTextFormat> m_gridDayFormat;

    // Direct2D 颜色画刷
    ComPtr<ID2D1SolidColorBrush> m_accentBrush;      // 苹果红/橙色
    ComPtr<ID2D1SolidColorBrush> m_textPrimaryBrush; // 高亮主文字
    ComPtr<ID2D1SolidColorBrush> m_textSecondaryBrush;// 次要灰白文字
    ComPtr<ID2D1SolidColorBrush> m_dividerBrush;     // 分割线
    ComPtr<ID2D1SolidColorBrush> m_todayCircleBrush; // 今日高亮圆底
};

} // namespace Windget
