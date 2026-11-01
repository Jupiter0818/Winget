#pragma once

#include "widgets/IWidget.h"
#include <wrl/client.h>
#include <string>
#include <cstdint>

namespace Windget {

using Microsoft::WRL::ComPtr;

class SystemMonitorWidget : public IWidget
{
public:
    explicit SystemMonitorWidget(std::string id = "system_default");
    ~SystemMonitorWidget() override;

    std::string GetId() const override { return m_id; }
    std::string GetType() const override { return "system"; }

    float GetWidth() const override { return 320.0f; }
    float GetHeight() const override { return 170.0f; }
    uint32_t GetUpdateIntervalMs() const override { return 1000; } // 每秒平滑更新

    void Initialize(IDWriteFactory* dwriteFactory) override;
    void Update() override;
    void Render(
        ID2D1DCRenderTarget* target,
        IDWriteFactory* dwriteFactory,
        const D2D1_RECT_F& cardBounds
    ) override;
    void Shutdown() override;

private:
    void SamplePerformanceData();

private:
    std::string m_id;

    // 监控数值
    float m_cpuUsage = 0.0f;
    float m_memUsage = 0.0f;
    float m_memUsedGB = 0.0f;
    float m_memTotalGB = 0.0f;

    // CPU 时间差分历史记录
    uint64_t m_prevIdleTime = 0;
    uint64_t m_prevKernelTime = 0;
    uint64_t m_prevUserTime = 0;

    // DirectWrite 文本格式
    ComPtr<IDWriteTextFormat> m_titleFormat;
    ComPtr<IDWriteTextFormat> m_labelFormat;
    ComPtr<IDWriteTextFormat> m_valueFormat;

    // Direct2D 画刷
    ComPtr<ID2D1SolidColorBrush> m_statusDotBrush;
    ComPtr<ID2D1SolidColorBrush> m_titleBrush;
    ComPtr<ID2D1SolidColorBrush> m_labelBrush;
    ComPtr<ID2D1SolidColorBrush> m_trackBrush;
    ComPtr<ID2D1SolidColorBrush> m_cpuBarBrush;
    ComPtr<ID2D1SolidColorBrush> m_memBarBrush;
};

} // namespace Windget
