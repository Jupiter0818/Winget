#pragma once

#include "widgets/IWidget.h"
#include <wrl/client.h>

namespace Windget {

using Microsoft::WRL::ComPtr;

class ClockWidget : public IWidget
{
public:
    explicit ClockWidget(std::string id = "clock_default");
    ~ClockWidget() override;

    std::string GetId() const override { return m_id; }
    std::string GetType() const override { return "clock"; }

    float GetWidth() const override { return 320.0f; }
    float GetHeight() const override { return 170.0f; }
    uint32_t GetUpdateIntervalMs() const override { return 1000; }

    void Initialize(IDWriteFactory* dwriteFactory) override;
    void Update() override;
    void Render(
        ID2D1DCRenderTarget* target,
        IDWriteFactory* dwriteFactory,
        const D2D1_RECT_F& cardBounds
    ) override;
    void Shutdown() override;

private:
    void RefreshTimeAndDate();

private:
    std::string m_id;
    std::wstring m_timeString;
    std::wstring m_dateString;

    ComPtr<IDWriteTextFormat> m_timeFormat;
    ComPtr<IDWriteTextFormat> m_dateFormat;

    ComPtr<ID2D1SolidColorBrush> m_timeBrush;
    ComPtr<ID2D1SolidColorBrush> m_dateBrush;
};

} // namespace Windget
