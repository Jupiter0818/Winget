#include "widgets/SystemMonitorWidget.h"
#include "core/ThemeManager.h"
#include <windows.h>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace Windget {

static uint64_t FileTimeToUInt64(const FILETIME& ft)
{
    ULARGE_INTEGER li;
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    return li.QuadPart;
}

SystemMonitorWidget::SystemMonitorWidget(std::string id)
    : m_id(std::move(id))
{
    SamplePerformanceData();
}

SystemMonitorWidget::~SystemMonitorWidget()
{
    Shutdown();
}

void SystemMonitorWidget::SamplePerformanceData()
{
    // 1. 获取 CPU 使用率 (差分算法)
    FILETIME idleTime{}, kernelTime{}, userTime{};
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime))
    {
        uint64_t idle = FileTimeToUInt64(idleTime);
        uint64_t kernel = FileTimeToUInt64(kernelTime);
        uint64_t user = FileTimeToUInt64(userTime);

        if (m_prevKernelTime != 0 || m_prevUserTime != 0)
        {
            uint64_t kernelDiff = kernel - m_prevKernelTime;
            uint64_t userDiff = user - m_prevUserTime;
            uint64_t idleDiff = idle - m_prevIdleTime;
            uint64_t totalDiff = kernelDiff + userDiff;

            if (totalDiff > 0)
            {
                int64_t busy = static_cast<int64_t>(totalDiff) - static_cast<int64_t>(idleDiff);
                if (busy < 0) busy = 0;
                m_cpuUsage = static_cast<float>(busy * 100.0 / totalDiff);
                m_cpuUsage = std::clamp(m_cpuUsage, 0.0f, 100.0f);
            }
        }

        m_prevIdleTime = idle;
        m_prevKernelTime = kernel;
        m_prevUserTime = user;
    }

    // 2. 获取内存信息 (已用 / 总量)
    MEMORYSTATUSEX memStatus{};
    memStatus.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memStatus))
    {
        m_memUsage = static_cast<float>(memStatus.dwMemoryLoad);
        double totalBytes = static_cast<double>(memStatus.ullTotalPhys);
        double availBytes = static_cast<double>(memStatus.ullAvailPhys);
        double usedBytes = totalBytes - availBytes;

        m_memTotalGB = static_cast<float>(totalBytes / (1024.0 * 1024.0 * 1024.0));
        m_memUsedGB = static_cast<float>(usedBytes / (1024.0 * 1024.0 * 1024.0));
    }
}

void SystemMonitorWidget::Initialize(IDWriteFactory* dwriteFactory)
{
    if (!dwriteFactory)
    {
        return;
    }

    // 1. 顶部小标题
    dwriteFactory->CreateTextFormat(
        L"Segoe UI Variable Display",
        nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        11.0f,
        L"",
        m_titleFormat.GetAddressOf()
    );
    if (!m_titleFormat)
    {
        dwriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_SEMI_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            11.0f,
            L"",
            m_titleFormat.GetAddressOf()
        );
    }
    if (m_titleFormat)
    {
        m_titleFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        m_titleFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    // 2. 左侧标签格式 (CPU / Memory)
    dwriteFactory->CreateTextFormat(
        L"Segoe UI Variable Text",
        nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        13.5f,
        L"",
        m_labelFormat.GetAddressOf()
    );
    if (!m_labelFormat)
    {
        dwriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_SEMI_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            13.5f,
            L"",
            m_labelFormat.GetAddressOf()
        );
    }
    if (m_labelFormat)
    {
        m_labelFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        m_labelFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    // 3. 右侧数值格式 (居右对齐)
    dwriteFactory->CreateTextFormat(
        L"Segoe UI Variable Text",
        nullptr,
        DWRITE_FONT_WEIGHT_MEDIUM,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.5f,
        L"",
        m_valueFormat.GetAddressOf()
    );
    if (!m_valueFormat)
    {
        dwriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_MEDIUM,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            12.5f,
            L"",
            m_valueFormat.GetAddressOf()
        );
    }
    if (m_valueFormat)
    {
        m_valueFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        m_valueFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    SamplePerformanceData();
}

void SystemMonitorWidget::Update()
{
    SamplePerformanceData();
}

void SystemMonitorWidget::Render(
    ID2D1DCRenderTarget* target,
    IDWriteFactory* /*dwriteFactory*/,
    const D2D1_RECT_F& cardBounds
)
{
    if (!target)
    {
        return;
    }

    // 确保画刷可用
    if (!m_statusDotBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(0.19f, 0.82f, 0.35f, 0.95f), // 极客荧光绿
            m_statusDotBrush.GetAddressOf()
        );
    }
    if (!m_titleBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.55f),
            m_titleBrush.GetAddressOf()
        );
    }
    if (!m_labelBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.95f),
            m_labelBrush.GetAddressOf()
        );
    }
    if (!m_trackBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.10f),
            m_trackBrush.GetAddressOf()
        );
    }
    if (!m_cpuBarBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(0.04f, 0.52f, 1.0f, 0.95f), // macOS 经典科技蓝
            m_cpuBarBrush.GetAddressOf()
        );
    }
    if (!m_memBarBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(0.75f, 0.35f, 0.95f, 0.95f), // 优雅高雅紫
            m_memBarBrush.GetAddressOf()
        );
    }

    // 根据深浅主题动态调整标题、文字与底槽颜色
    bool isDark = ThemeManager::GetInstance().IsDarkMode();
    if (m_titleBrush)
    {
        m_titleBrush->SetColor(isDark ? D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.55f) : D2D1::ColorF(0.35f, 0.38f, 0.44f, 0.85f));
    }
    if (m_labelBrush)
    {
        m_labelBrush->SetColor(isDark ? D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.95f) : D2D1::ColorF(0.08f, 0.09f, 0.12f, 0.95f));
    }
    if (m_trackBrush)
    {
        m_trackBrush->SetColor(isDark ? D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.10f) : D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.08f));
    }

    float left = cardBounds.left + 24.0f;
    float right = cardBounds.right - 24.0f;
    float width = right - left;
    float top = cardBounds.top;

    // ==========================================
    // 1. 顶部标题栏 + 状态小绿点
    // ==========================================
    float headerY = top + 18.0f;
    if (m_statusDotBrush)
    {
        D2D1_ELLIPSE dot = D2D1::Ellipse(
            D2D1::Point2F(left + 4.0f, headerY + 8.0f),
            3.5f,
            3.5f
        );
        target->FillEllipse(dot, m_statusDotBrush.Get());
    }

    if (m_titleFormat && m_titleBrush)
    {
        D2D1_RECT_F titleRect = D2D1::RectF(
            left + 14.0f,
            headerY,
            right,
            headerY + 16.0f
        );
        std::wstring title = L"SYSTEM PERFORMANCE";
        target->DrawText(
            title.c_str(),
            static_cast<UINT32>(title.length()),
            m_titleFormat.Get(),
            titleRect,
            m_titleBrush.Get()
        );
    }

    // ==========================================
    // 2. CPU 监控行
    // ==========================================
    float cpuLabelY = top + 42.0f;
    if (m_labelFormat && m_labelBrush)
    {
        D2D1_RECT_F cpuTextRect = D2D1::RectF(left, cpuLabelY, left + 80.0f, cpuLabelY + 20.0f);
        std::wstring cpuLabel = L"CPU";
        target->DrawText(
            cpuLabel.c_str(),
            static_cast<UINT32>(cpuLabel.length()),
            m_labelFormat.Get(),
            cpuTextRect,
            m_labelBrush.Get()
        );
    }

    if (m_valueFormat && m_cpuBarBrush)
    {
        D2D1_RECT_F cpuValRect = D2D1::RectF(right - 100.0f, cpuLabelY, right, cpuLabelY + 20.0f);
        std::wstring cpuValStr = std::to_wstring(static_cast<int>(m_cpuUsage + 0.5f)) + L"%";
        target->DrawText(
            cpuValStr.c_str(),
            static_cast<UINT32>(cpuValStr.length()),
            m_valueFormat.Get(),
            cpuValRect,
            m_cpuBarBrush.Get()
        );
    }

    // CPU 胶囊进度条
    float cpuBarY = top + 66.0f;
    float barHeight = 7.0f;
    if (m_trackBrush && m_cpuBarBrush)
    {
        D2D1_ROUNDED_RECT trackRect = D2D1::RoundedRect(
            D2D1::RectF(left, cpuBarY, right, cpuBarY + barHeight),
            3.5f, 3.5f
        );
        target->FillRoundedRectangle(trackRect, m_trackBrush.Get());

        float fillWidth = std::max(barHeight, width * (m_cpuUsage / 100.0f));
        fillWidth = std::min(fillWidth, width);
        D2D1_ROUNDED_RECT fillRect = D2D1::RoundedRect(
            D2D1::RectF(left, cpuBarY, left + fillWidth, cpuBarY + barHeight),
            3.5f, 3.5f
        );
        target->FillRoundedRectangle(fillRect, m_cpuBarBrush.Get());
    }

    // ==========================================
    // 3. 内存监控行
    // ==========================================
    float memLabelY = top + 92.0f;
    if (m_labelFormat && m_labelBrush)
    {
        D2D1_RECT_F memTextRect = D2D1::RectF(left, memLabelY, left + 80.0f, memLabelY + 20.0f);
        std::wstring memLabel = L"Memory";
        target->DrawText(
            memLabel.c_str(),
            static_cast<UINT32>(memLabel.length()),
            m_labelFormat.Get(),
            memTextRect,
            m_labelBrush.Get()
        );
    }

    if (m_valueFormat && m_memBarBrush)
    {
        D2D1_RECT_F memValRect = D2D1::RectF(right - 180.0f, memLabelY, right, memLabelY + 20.0f);
        std::wstringstream ss;
        ss << std::fixed << std::setprecision(1) << m_memUsedGB << L" / " << m_memTotalGB << L" GB (" << static_cast<int>(m_memUsage + 0.5f) << L"%)";
        std::wstring memValStr = ss.str();
        target->DrawText(
            memValStr.c_str(),
            static_cast<UINT32>(memValStr.length()),
            m_valueFormat.Get(),
            memValRect,
            m_memBarBrush.Get()
        );
    }

    // 内存胶囊进度条
    float memBarY = top + 116.0f;
    if (m_trackBrush && m_memBarBrush)
    {
        D2D1_ROUNDED_RECT trackRect = D2D1::RoundedRect(
            D2D1::RectF(left, memBarY, right, memBarY + barHeight),
            3.5f, 3.5f
        );
        target->FillRoundedRectangle(trackRect, m_trackBrush.Get());

        float fillWidth = std::max(barHeight, width * (m_memUsage / 100.0f));
        fillWidth = std::min(fillWidth, width);
        D2D1_ROUNDED_RECT fillRect = D2D1::RoundedRect(
            D2D1::RectF(left, memBarY, left + fillWidth, memBarY + barHeight),
            3.5f, 3.5f
        );
        target->FillRoundedRectangle(fillRect, m_memBarBrush.Get());
    }
}

void SystemMonitorWidget::Shutdown()
{
    m_statusDotBrush.Reset();
    m_titleBrush.Reset();
    m_labelBrush.Reset();
    m_trackBrush.Reset();
    m_cpuBarBrush.Reset();
    m_memBarBrush.Reset();

    m_titleFormat.Reset();
    m_labelFormat.Reset();
    m_valueFormat.Reset();
}

} // namespace Windget
