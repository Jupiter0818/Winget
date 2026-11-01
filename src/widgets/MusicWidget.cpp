#include "widgets/MusicWidget.h"
#include "core/MediaManager.h"
#include "core/ConfigManager.h"
#include "core/ThemeManager.h"
#include <windows.h>
#include <cmath>
#include <vector>
#include <fstream>
#include <algorithm>

namespace Windget {

struct PlayerMatchInfo
{
    std::wstring playerName;
    std::wstring songTitle;
    std::wstring artistName;
    bool isPlaying = false;
};

// 窗口扫描枚举回调 (Win32 经典媒体播放器兜底)
static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (!IsWindowVisible(hwnd))
    {
        return TRUE;
    }

    wchar_t title[512]{};
    int len = GetWindowTextW(hwnd, title, 512);
    if (len <= 0)
    {
        return TRUE;
    }

    std::wstring windowTitle(title);
    auto* pMatch = reinterpret_cast<PlayerMatchInfo*>(lParam);

    // Spotify
    if (windowTitle.find(L"Spotify") != std::wstring::npos)
    {
        if (windowTitle == L"Spotify" || windowTitle == L"Spotify Free" || windowTitle == L"Spotify Premium")
        {
            if (pMatch->songTitle.empty())
            {
                pMatch->playerName = L"SPOTIFY";
                pMatch->songTitle = L"Spotify Paused";
                pMatch->artistName = L"Press Play to Resume";
                pMatch->isPlaying = false;
            }
        }
        else
        {
            size_t sep = windowTitle.find(L" - ");
            if (sep != std::wstring::npos)
            {
                pMatch->playerName = L"SPOTIFY";
                pMatch->songTitle = windowTitle.substr(0, sep);
                pMatch->artistName = windowTitle.substr(sep + 3);
                pMatch->isPlaying = true;
                return FALSE;
            }
        }
    }

    // 网易云音乐 / QQ音乐
    size_t sep = windowTitle.find(L" - ");
    if (sep != std::wstring::npos && windowTitle.length() > 6)
    {
        wchar_t className[256]{};
        GetClassNameW(hwnd, className, 256);
        std::wstring cls(className);

        if (cls.find(L"Orion_Window") != std::wstring::npos ||
            cls.find(L"QQMusic") != std::wstring::npos ||
            windowTitle.find(L"网易云") != std::wstring::npos ||
            windowTitle.find(L"QQ音乐") != std::wstring::npos)
        {
            pMatch->playerName = L"NOW PLAYING";
            pMatch->songTitle = windowTitle.substr(0, sep);
            pMatch->artistName = windowTitle.substr(sep + 3);
            pMatch->isPlaying = true;
            return FALSE;
        }
    }

    return TRUE;
}

static bool PtInRectF(const D2D1_RECT_F& rc, float x, float y)
{
    return (x >= rc.left && x <= rc.right && y >= rc.top && y <= rc.bottom);
}

MusicWidget::MusicWidget(std::string id)
    : m_id(std::move(id))
{
    // 从持久化配置中恢复主题模式 (默认封面自适应取色)
    const auto& config = ConfigManager::GetInstance().GetConfig();
    for (const auto& w : config.widgets)
    {
        if (w.id == m_id || (w.type == "music" && config.widgets.size() == 1))
        {
            if (w.theme == "dark")
            {
                m_themeMode = MusicThemeMode::Dark;
            }
            else
            {
                m_themeMode = MusicThemeMode::Adaptive;
            }
            break;
        }
    }

    RefreshMediaStatus();
}

MusicWidget::~MusicWidget()
{
    Shutdown();
}

void MusicWidget::SetThemeMode(MusicThemeMode mode)
{
    m_themeMode = mode;
    ConfigManager::GetInstance().UpdateWidgetTheme(
        m_id,
        (mode == MusicThemeMode::Adaptive) ? "adaptive" : "dark"
    );
    ConfigManager::GetInstance().Save();
}

void MusicWidget::ToggleThemeMode()
{
    SetThemeMode((m_themeMode == MusicThemeMode::Adaptive) ? MusicThemeMode::Dark : MusicThemeMode::Adaptive);
}

void MusicWidget::RefreshMediaStatus()
{
    // 优先从系统级 GSMTC 媒体服务获取 (完美支持 Apple Music、Spotify 等 WinUI / Store 现代应用)
    CurrentMediaInfo gsmtc = MediaManager::GetInstance().GetCurrentMedia();
    if (gsmtc.hasActiveMedia)
    {
        m_playerName = gsmtc.appName;
        m_songTitle = gsmtc.title;
        m_artistName = gsmtc.artist;
        m_coverPath = gsmtc.coverPath;
        m_isPlaying = gsmtc.isPlaying;
    }
    else
    {
        // 兜底方案：回退到传统 Win32 窗口枚举 (foobar2000, AIMP, 网易云音乐等)
        m_coverPath.clear();
        PlayerMatchInfo match{};
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&match));

        if (!match.songTitle.empty())
        {
            m_playerName = match.playerName;
            m_songTitle = match.songTitle;
            m_artistName = match.artistName;
            m_isPlaying = match.isPlaying;
        }
        else
        {
            m_playerName = L"NOW PLAYING";
            m_songTitle = L"No Media Playing";
            m_artistName = L"Open Spotify, NetEase or Apple Music";
            m_isPlaying = false;
        }
    }
}

void MusicWidget::SendMediaKey(BYTE vk)
{
    keybd_event(vk, 0, 0, 0);
    keybd_event(vk, 0, KEYEVENTF_KEYUP, 0);
}

bool MusicWidget::OnMouseMove(float localX, float localY)
{
    MusicButtonId prevHover = m_hoveredButton;
    m_hoveredButton = MusicButtonId::None;

    if (PtInRectF(m_prevButtonRect, localX, localY))
    {
        m_hoveredButton = MusicButtonId::Prev;
    }
    else if (PtInRectF(m_playButtonRect, localX, localY))
    {
        m_hoveredButton = MusicButtonId::PlayPause;
    }
    else if (PtInRectF(m_nextButtonRect, localX, localY))
    {
        m_hoveredButton = MusicButtonId::Next;
    }
    else if (PtInRectF(m_themeToggleRect, localX, localY))
    {
        m_hoveredButton = MusicButtonId::ThemeToggle;
    }

    return (m_hoveredButton != prevHover);
}

void MusicWidget::OnMouseLeave()
{
    m_hoveredButton = MusicButtonId::None;
    m_pressedButton = MusicButtonId::None;
}

bool MusicWidget::OnMouseDown(float localX, float localY)
{
    if (PtInRectF(m_prevButtonRect, localX, localY))
    {
        m_pressedButton = MusicButtonId::Prev;
        return true;
    }
    if (PtInRectF(m_playButtonRect, localX, localY))
    {
        m_pressedButton = MusicButtonId::PlayPause;
        return true;
    }
    if (PtInRectF(m_nextButtonRect, localX, localY))
    {
        m_pressedButton = MusicButtonId::Next;
        return true;
    }
    if (PtInRectF(m_themeToggleRect, localX, localY))
    {
        m_pressedButton = MusicButtonId::ThemeToggle;
        return true;
    }
    return false;
}

bool MusicWidget::OnMouseUp(float /*localX*/, float /*localY*/)
{
    if (m_pressedButton != MusicButtonId::None)
    {
        m_pressedButton = MusicButtonId::None;
        return true;
    }
    return false;
}

bool MusicWidget::OnClick(float localX, float localY)
{
    // 1. 上一曲按钮
    if (PtInRectF(m_prevButtonRect, localX, localY))
    {
        MediaManager::GetInstance().SendCommand("PREV");
        return true;
    }

    // 2. 播放 / 暂停按钮 (精准幂等控制，杜绝二次翻转)
    if (PtInRectF(m_playButtonRect, localX, localY))
    {
        if (m_isPlaying)
        {
            m_isPlaying = false;
            MediaManager::GetInstance().SendCommand("PAUSE");
        }
        else
        {
            m_isPlaying = true;
            MediaManager::GetInstance().SendCommand("PLAY");
        }
        return true;
    }

    // 3. 下一曲按钮
    if (PtInRectF(m_nextButtonRect, localX, localY))
    {
        MediaManager::GetInstance().SendCommand("NEXT");
        return true;
    }

    // 4. 外观主题切换 (右上角胶囊徽标或来源标签)
    if (PtInRectF(m_themeToggleRect, localX, localY))
    {
        ToggleThemeMode();
        return true;
    }

    return false;
}

void MusicWidget::ExtractDominantColor(IWICFormatConverter* converter)
{
    if (!converter || !m_wicFactory)
    {
        return;
    }

    ComPtr<IWICBitmapScaler> scaler;
    HRESULT hr = m_wicFactory->CreateBitmapScaler(scaler.GetAddressOf());
    if (FAILED(hr))
    {
        return;
    }

    // 降采样至 32x32 像素快速提取特征色 (Fant 高保真下采样)
    hr = scaler->Initialize(converter, 32, 32, WICBitmapInterpolationModeFant);
    if (FAILED(hr))
    {
        return;
    }

    std::vector<BYTE> pixels(32 * 32 * 4);
    hr = scaler->CopyPixels(nullptr, 32 * 4, static_cast<UINT>(pixels.size()), pixels.data());
    if (FAILED(hr))
    {
        return;
    }

    float totalWeight = 0.0f;
    float sumR = 0.0f, sumG = 0.0f, sumB = 0.0f;

    for (size_t i = 0; i < pixels.size(); i += 4)
    {
        float b = static_cast<float>(pixels[i]);
        float g = static_cast<float>(pixels[i + 1]);
        float r = static_cast<float>(pixels[i + 2]);

        float maxV = std::max(r, std::max(g, b));
        float minV = std::min(r, std::min(g, b));
        float sat = (maxV > 0.0f) ? (maxV - minV) / maxV : 0.0f;
        float lum = 0.299f * r + 0.587f * g + 0.114f * b;

        // 剔除过曝纯白与极暗杂色
        if (lum < 20.0f || lum > 240.0f)
        {
            continue;
        }

        // 突出饱满鲜艳的主题色彩权重
        float weight = 1.0f + 4.0f * sat * sat;
        sumR += r * weight;
        sumG += g * weight;
        sumB += b * weight;
        totalWeight += weight;
    }

    float finalR = (totalWeight > 0.0f) ? (sumR / totalWeight) : 65.0f;
    float finalG = (totalWeight > 0.0f) ? (sumG / totalWeight) : 65.0f;
    float finalB = (totalWeight > 0.0f) ? (sumB / totalWeight) : 80.0f;

    m_dominantColor = D2D1::ColorF(finalR / 255.0f, finalG / 255.0f, finalB / 255.0f, 1.0f);
}

bool MusicWidget::LoadCoverBitmap(ID2D1DCRenderTarget* target, const std::wstring& path)
{
    if (!target || !m_wicFactory || path.empty())
    {
        return false;
    }

    // 1. 读取文件全部字节至内存流并立即关闭文件句柄，彻底消除任何磁盘文件占用/锁定
    std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        return false;
    }
    std::streamsize fileSize = file.tellg();
    if (fileSize <= 0)
    {
        return false;
    }
    file.seekg(0, std::ios::beg);
    std::vector<BYTE> buffer(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize))
    {
        return false;
    }
    file.close(); // 磁盘句柄立即释放，外部守护进程可畅快写入/轮换

    // 2. 使用 IWICStream 从内存缓冲构建 WIC 解码器
    ComPtr<IWICStream> wicStream;
    HRESULT hr = m_wicFactory->CreateStream(wicStream.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = wicStream->InitializeFromMemory(buffer.data(), static_cast<DWORD>(buffer.size()));
    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapDecoder> decoder;
    hr = m_wicFactory->CreateDecoderFromStream(
        wicStream.Get(),
        nullptr,
        WICDecodeMetadataCacheOnLoad,
        decoder.GetAddressOf()
    );
    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, frame.GetAddressOf());
    if (FAILED(hr)) return false;

    ComPtr<IWICFormatConverter> converter;
    hr = m_wicFactory->CreateFormatConverter(converter.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = converter->Initialize(
        frame.Get(),
        GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0f,
        WICBitmapPaletteTypeMedianCut
    );
    if (FAILED(hr)) return false;

    // 提取封面核心色相 (用于自适应 Apple Music 氛围渐变)
    ExtractDominantColor(converter.Get());

    m_coverBitmap.Reset();
    hr = target->CreateBitmapFromWicBitmap(converter.Get(), nullptr, m_coverBitmap.GetAddressOf());
    if (FAILED(hr) || !m_coverBitmap) return false;

    m_coverBrush.Reset();
    D2D1_BITMAP_BRUSH_PROPERTIES brushProps = D2D1::BitmapBrushProperties(
        D2D1_EXTEND_MODE_CLAMP,
        D2D1_EXTEND_MODE_CLAMP,
        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
    );
    hr = target->CreateBitmapBrush(m_coverBitmap.Get(), brushProps, m_coverBrush.GetAddressOf());
    return SUCCEEDED(hr);
}

void MusicWidget::CreateAdaptiveBrushes(ID2D1DCRenderTarget* target, const D2D1_RECT_F& cardBounds)
{
    if (!target) return;

    float r = m_dominantColor.r * 255.0f;
    float g = m_dominantColor.g * 255.0f;
    float b = m_dominantColor.b * 255.0f;

    // 仿 Apple Music 沉浸式高级氛围底色：
    // 提取封面核心色调，调节为通透且具表现力的深色调底色，左上微透微光，右下沉稳下沉
    float bgR1 = std::clamp(r * 0.46f + 14.0f, 24.0f, 125.0f) / 255.0f;
    float bgG1 = std::clamp(g * 0.46f + 14.0f, 24.0f, 125.0f) / 255.0f;
    float bgB1 = std::clamp(b * 0.46f + 14.0f, 28.0f, 140.0f) / 255.0f;

    float bgR2 = std::clamp(bgR1 * 255.0f * 0.68f, 16.0f, 85.0f) / 255.0f;
    float bgG2 = std::clamp(bgG1 * 255.0f * 0.68f, 16.0f, 85.0f) / 255.0f;
    float bgB2 = std::clamp(bgB1 * 255.0f * 0.68f, 18.0f, 95.0f) / 255.0f;

    D2D1_GRADIENT_STOP stops[2];
    stops[0].position = 0.0f;
    stops[0].color = D2D1::ColorF(bgR1, bgG1, bgB1, 0.95f);
    stops[1].position = 1.0f;
    stops[1].color = D2D1::ColorF(bgR2, bgG2, bgB2, 0.98f);

    ComPtr<ID2D1GradientStopCollection> stopCollection;
    HRESULT hr = target->CreateGradientStopCollection(
        stops,
        2,
        D2D1_GAMMA_2_2,
        D2D1_EXTEND_MODE_CLAMP,
        stopCollection.GetAddressOf()
    );
    if (SUCCEEDED(hr))
    {
        m_adaptiveBgBrush.Reset();
        target->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(cardBounds.left, cardBounds.top),
                D2D1::Point2F(cardBounds.right, cardBounds.bottom)
            ),
            stopCollection.Get(),
            m_adaptiveBgBrush.GetAddressOf()
        );
    }

    // 动态提取高亮强调色用于来源标签 (例如 Apple Music 品牌品红或封面专属高饱和色相)
    float maxV = std::max(r, std::max(g, b));
    float minV = std::min(r, std::min(g, b));
    float sat = (maxV > 0.0f) ? (maxV - minV) / maxV : 0.0f;

    D2D1_COLOR_F accColor;
    if (sat > 0.18f && maxV > 30.0f)
    {
        // 提纯饱和度并保证明度适中，富有色彩感但不抢夺白字歌名的视线焦点
        float scale = 220.0f / maxV;
        accColor = D2D1::ColorF(
            std::clamp(r * scale, 30.0f, 255.0f) / 255.0f,
            std::clamp(g * scale, 30.0f, 255.0f) / 255.0f,
            std::clamp(b * scale, 30.0f, 255.0f) / 255.0f,
            1.0f
        );
    }
    else
    {
        // 默认 Apple Music 标志性品红 (#FA243C)
        accColor = D2D1::ColorF(0.99f, 0.17f, 0.33f, 1.0f);
    }

    m_accentBrush.Reset();
    target->CreateSolidColorBrush(accColor, m_accentBrush.GetAddressOf());

    // 半透明高质感微边框
    m_adaptiveBorderBrush.Reset();
    target->CreateSolidColorBrush(
        D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.12f),
        m_adaptiveBorderBrush.GetAddressOf()
    );
}

void MusicWidget::Initialize(IDWriteFactory* dwriteFactory)
{
    // 初始化 WIC 图像工厂用于高清专辑解码与色彩采样
    CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(m_wicFactory.GetAddressOf())
    );

    if (!dwriteFactory)
    {
        return;
    }

    // 1. 来源标识 (小字号粗体大写)
    dwriteFactory->CreateTextFormat(
        L"Segoe UI Variable Display",
        nullptr,
        DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        10.5f,
        L"",
        m_sourceFormat.GetAddressOf()
    );
    if (!m_sourceFormat)
    {
        dwriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            10.5f,
            L"",
            m_sourceFormat.GetAddressOf()
        );
    }
    if (m_sourceFormat)
    {
        m_sourceFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        m_sourceFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    // 2. 歌名格式 (15.5pt 粗体，自适应省略号)
    dwriteFactory->CreateTextFormat(
        L"Segoe UI Variable Display",
        nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        15.5f,
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
            15.5f,
            L"",
            m_titleFormat.GetAddressOf()
        );
    }
    if (m_titleFormat)
    {
        m_titleFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        m_titleFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        m_titleFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        DWRITE_TRIMMING trimming = { DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
        ComPtr<IDWriteInlineObject> ellipsis;
        if (SUCCEEDED(dwriteFactory->CreateEllipsisTrimmingSign(m_titleFormat.Get(), ellipsis.GetAddressOf())))
        {
            m_titleFormat->SetTrimming(&trimming, ellipsis.Get());
        }
    }

    // 3. 歌手格式 (12.0pt，自适应省略号)
    dwriteFactory->CreateTextFormat(
        L"Segoe UI Variable Text",
        nullptr,
        DWRITE_FONT_WEIGHT_MEDIUM,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.0f,
        L"",
        m_artistFormat.GetAddressOf()
    );
    if (!m_artistFormat)
    {
        dwriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_MEDIUM,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            12.0f,
            L"",
            m_artistFormat.GetAddressOf()
        );
    }
    if (m_artistFormat)
    {
        m_artistFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        m_artistFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        m_artistFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        DWRITE_TRIMMING trimming = { DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
        ComPtr<IDWriteInlineObject> ellipsis;
        if (SUCCEEDED(dwriteFactory->CreateEllipsisTrimmingSign(m_artistFormat.Get(), ellipsis.GetAddressOf())))
        {
            m_artistFormat->SetTrimming(&trimming, ellipsis.Get());
        }
    }

    RefreshMediaStatus();
}

void MusicWidget::Update()
{
    RefreshMediaStatus();
}

void MusicWidget::Render(
    ID2D1DCRenderTarget* target,
    IDWriteFactory* /*dwriteFactory*/,
    const D2D1_RECT_F& cardBounds
)
{
    if (!target)
    {
        return;
    }

    // 初始化通用画刷
    if (!m_textPrimaryBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.96f),
            m_textPrimaryBrush.GetAddressOf()
        );
    }
    if (!m_textSecondaryBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.58f),
            m_textSecondaryBrush.GetAddressOf()
        );
    }
    if (!m_accentBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(0.99f, 0.17f, 0.33f, 1.0f), // 默认 Apple Music 潮流品红
            m_accentBrush.GetAddressOf()
        );
    }
    if (!m_controlBgBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.12f),
            m_controlBgBrush.GetAddressOf()
        );
    }
    if (!m_controlIconBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(0.11f, 0.11f, 0.13f, 1.0f),
            m_controlIconBrush.GetAddressOf()
        );
    }
    if (!m_borderBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.10f),
            m_borderBrush.GetAddressOf()
        );
    }
    if (!m_shadowBrush)
    {
        target->CreateSolidColorBrush(
            D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.28f),
            m_shadowBrush.GetAddressOf()
        );
    }

    float top = cardBounds.top;
    float left = cardBounds.left;

    // 确定当前卡片视觉底色是深色还是浅色 (严格根据实际卡片呈现的底色判断)
    bool isDarkSystem = ThemeManager::GetInstance().IsDarkMode();
    bool isCardDark = true;
    if (m_themeMode == MusicThemeMode::Adaptive)
    {
        // 封面自适应模式：当前背景为基于专辑色调优的深色沉浸氛围渐变，底色恒为深色
        isCardDark = true;
    }
    else
    {
        // 经典毛玻璃模式：严格跟随当前系统的深色/浅色模式或手动主题
        isCardDark = isDarkSystem;
    }

    // 全局动态更新文字与控制画刷颜色，确保永远呈现最高对比度与清晰可读性
    if (m_textPrimaryBrush)
    {
        // 歌名主标题与按钮图标：深色底为纯白高亮字，浅色底为深炭黑字
        m_textPrimaryBrush->SetColor(
            isCardDark
                ? D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.98f)
                : D2D1::ColorF(0.08f, 0.09f, 0.12f, 0.96f)
        );
    }
    if (m_textSecondaryBrush)
    {
        // 歌手/专辑副标题：深色底为清透白，浅色底为沉稳冷灰
        m_textSecondaryBrush->SetColor(
            isCardDark
                ? D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.68f)
                : D2D1::ColorF(0.32f, 0.35f, 0.42f, 0.85f)
        );
    }
    if (m_controlBgBrush)
    {
        m_controlBgBrush->SetColor(
            isCardDark
                ? D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.12f)
                : D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.06f)
        );
    }
    if (m_borderBrush)
    {
        m_borderBrush->SetColor(
            isCardDark
                ? D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.12f)
                : D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.10f)
        );
    }
    if (m_themeMode != MusicThemeMode::Adaptive)
    {
        if (m_accentBrush)
        {
            m_accentBrush->SetColor(
                isCardDark
                    ? D2D1::ColorF(0.99f, 0.22f, 0.38f, 1.0f)
                    : D2D1::ColorF(0.88f, 0.12f, 0.28f, 1.0f)
            );
        }
    }

    // 检查封面文件或曲目是否更新并加载位图
    bool needReload = false;
    std::wstring currentTrackKey = m_songTitle + L" - " + m_artistName;
    if (currentTrackKey != m_lastTrackKey)
    {
        m_lastTrackKey = currentTrackKey;
        needReload = true;
    }

    if (m_coverPath != m_loadedCoverPath)
    {
        needReload = true;
    }

    if (!m_coverPath.empty())
    {
        std::error_code ec;
        if (std::filesystem::exists(m_coverPath, ec))
        {
            auto writeTime = std::filesystem::last_write_time(m_coverPath, ec);
            if (!ec && writeTime != m_lastCoverWriteTime)
            {
                m_lastCoverWriteTime = writeTime;
                needReload = true;
            }
        }
    }

    if (m_coverPath.empty())
    {
        if (m_coverBitmap)
        {
            m_coverBitmap.Reset();
            m_coverBrush.Reset();
            m_loadedCoverPath.clear();
            CreateAdaptiveBrushes(target, cardBounds);
        }
    }
    else if (needReload || !m_coverBitmap)
    {
        if (LoadCoverBitmap(target, m_coverPath))
        {
            m_loadedCoverPath = m_coverPath;
            CreateAdaptiveBrushes(target, cardBounds);
        }
        else
        {
            m_coverBitmap.Reset();
            m_coverBrush.Reset();
            m_loadedCoverPath.clear();
        }
    }

    // ==========================================
    // 1. 卡片底色渲染 (经典黑曜 VS 封面自适应取色)
    // ==========================================
    if (m_themeMode == MusicThemeMode::Adaptive)
    {
        if (!m_adaptiveBgBrush)
        {
            CreateAdaptiveBrushes(target, cardBounds);
        }

        if (m_adaptiveBgBrush)
        {
            // 覆盖默认卡片背景，呈现 Apple Music 动态渐变沉浸底色
            D2D1_ROUNDED_RECT bgRect = D2D1::RoundedRect(cardBounds, 16.0f, 16.0f);
            target->FillRoundedRectangle(bgRect, m_adaptiveBgBrush.Get());
            if (m_adaptiveBorderBrush)
            {
                target->DrawRoundedRectangle(bgRect, m_adaptiveBorderBrush.Get(), 1.0f);
            }
        }
    }

    // ==========================================
    // 2. 左侧：高质感纯净专辑封面 (彻底去除黑胶，96x96 圆角正方形)
    // ==========================================
    float coverSize = 96.0f;
    float coverX = left + 18.0f;
    float coverY = top + (170.0f - coverSize) / 2.0f;
    D2D1_RECT_F coverRect = D2D1::RectF(coverX, coverY, coverX + coverSize, coverY + coverSize);
    D2D1_ROUNDED_RECT roundedCover = D2D1::RoundedRect(coverRect, 10.0f, 10.0f);

    if (m_coverBitmap && m_coverBrush)
    {
        // 缩放纹理使任意尺寸封面完美适配 96x96
        D2D1_SIZE_F bmpSize = m_coverBitmap->GetSize();
        if (bmpSize.width > 0 && bmpSize.height > 0)
        {
            D2D1_MATRIX_3X2_F scale = D2D1::Matrix3x2F::Scale(
                coverSize / bmpSize.width,
                coverSize / bmpSize.height,
                D2D1::Point2F(0, 0)
            );
            D2D1_MATRIX_3X2_F translation = D2D1::Matrix3x2F::Translation(coverX, coverY);
            m_coverBrush->SetTransform(scale * translation);
        }

        target->FillRoundedRectangle(roundedCover, m_coverBrush.Get());

        // 封面精致微边框
        if (m_borderBrush)
        {
            target->DrawRoundedRectangle(roundedCover, m_borderBrush.Get(), 1.0f);
        }
    }
    else
    {
        // 无封面时的极简高阶占位卡 (微质感纯色 + 音符)
        if (m_controlBgBrush)
        {
            target->FillRoundedRectangle(roundedCover, m_controlBgBrush.Get());
            if (m_borderBrush)
            {
                target->DrawRoundedRectangle(roundedCover, m_borderBrush.Get(), 1.0f);
            }
        }

        // 音符图标
        if (m_textSecondaryBrush)
        {
            float cx = coverX + coverSize / 2.0f;
            float cy = coverY + coverSize / 2.0f;
            target->DrawLine(D2D1::Point2F(cx - 6.0f, cy + 8.0f), D2D1::Point2F(cx - 6.0f, cy - 8.0f), m_textSecondaryBrush.Get(), 2.0f);
            target->DrawLine(D2D1::Point2F(cx + 6.0f, cy + 4.0f), D2D1::Point2F(cx + 6.0f, cy - 12.0f), m_textSecondaryBrush.Get(), 2.0f);
            target->DrawLine(D2D1::Point2F(cx - 6.0f, cy - 8.0f), D2D1::Point2F(cx + 6.0f, cy - 12.0f), m_textSecondaryBrush.Get(), 2.5f);
            target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx - 9.0f, cy + 8.0f), 4.5f, 3.5f), m_textSecondaryBrush.Get());
            target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx + 3.0f, cy + 4.0f), 4.5f, 3.5f), m_textSecondaryBrush.Get());
        }
    }

    // ==========================================
    // 3. 右侧：曲目元数据
    // ==========================================
    float textLeft = coverX + coverSize + 16.0f;
    float textRight = cardBounds.right - 18.0f;

    // 来源标识标签 (如 APPLE MUSIC)
    if (m_sourceFormat && m_accentBrush)
    {
        D2D1_RECT_F sourceRect = D2D1::RectF(textLeft, top + 22.0f, textRight - 32.0f, top + 38.0f);
        target->DrawText(
            m_playerName.c_str(),
            static_cast<UINT32>(m_playerName.length()),
            m_sourceFormat.Get(),
            sourceRect,
            m_accentBrush.Get()
        );
    }

    // 右上角外观主题切换胶囊 (点击切换 经典黑曜 / 自适应取色)
    float toggleX = textRight - 12.0f;
    float toggleY = top + 30.0f;
    m_themeToggleRect = D2D1::RectF(toggleX - 16.0f, toggleY - 12.0f, toggleX + 16.0f, toggleY + 12.0f);
    if (m_controlBgBrush)
    {
        bool isHovered = (m_hoveredButton == MusicButtonId::ThemeToggle);
        bool isPressed = (m_pressedButton == MusicButtonId::ThemeToggle);

        float pillAlpha = isPressed ? 0.35f : (isHovered ? 0.22f : 0.12f);
        ComPtr<ID2D1SolidColorBrush> pillBrush;
        target->CreateSolidColorBrush(
            isCardDark
                ? D2D1::ColorF(1.0f, 1.0f, 1.0f, pillAlpha)
                : D2D1::ColorF(0.0f, 0.0f, 0.0f, pillAlpha * 0.7f),
            pillBrush.GetAddressOf()
        );

        D2D1_ROUNDED_RECT pillRect = D2D1::RoundedRect(m_themeToggleRect, 6.0f, 6.0f);
        target->FillRoundedRectangle(pillRect, pillBrush.Get());

        // 绘制切换图标 (自适应模式绘制彩虹环点，黑曜模式绘制方形标)
        if (m_themeMode == MusicThemeMode::Adaptive)
        {
            if (m_accentBrush)
            {
                target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(toggleX, toggleY), 4.0f, 4.0f), m_accentBrush.Get());
            }
        }
        else
        {
            if (m_textSecondaryBrush)
            {
                target->FillRectangle(D2D1::RectF(toggleX - 3.5f, toggleY - 3.5f, toggleX + 3.5f, toggleY + 3.5f), m_textSecondaryBrush.Get());
            }
        }
    }

    // 歌名
    if (m_titleFormat && m_textPrimaryBrush)
    {
        D2D1_RECT_F titleRect = D2D1::RectF(textLeft, top + 42.0f, textRight, top + 66.0f);
        target->DrawText(
            m_songTitle.c_str(),
            static_cast<UINT32>(m_songTitle.length()),
            m_titleFormat.Get(),
            titleRect,
            m_textPrimaryBrush.Get()
        );
    }

    // 歌手
    if (m_artistFormat && m_textSecondaryBrush)
    {
        D2D1_RECT_F artistRect = D2D1::RectF(textLeft, top + 68.0f, textRight, top + 88.0f);
        target->DrawText(
            m_artistName.c_str(),
            static_cast<UINT32>(m_artistName.length()),
            m_artistFormat.Get(),
            artistRect,
            m_textSecondaryBrush.Get()
        );
    }

    // ==========================================
    // 4. 底部交互式控制按钮 (上一曲 | 播放/暂停 | 下一曲)
    // ==========================================
    float ctrlY = top + 122.0f;
    float btnSpacing = 48.0f;
    float playX = textLeft + (textRight - textLeft) / 2.0f;
    float prevX = playX - btnSpacing;
    float nextX = playX + btnSpacing;

    // --- 按钮 1: 上一曲 (|<<) ---
    m_prevButtonRect = D2D1::RectF(prevX - 18.0f, ctrlY - 18.0f, prevX + 18.0f, ctrlY + 18.0f);
    {
        bool isHovered = (m_hoveredButton == MusicButtonId::Prev);
        bool isPressed = (m_pressedButton == MusicButtonId::Prev);
        float radius = isPressed ? 12.5f : (isHovered ? 15.5f : 14.0f);
        float bgAlpha = isPressed ? 0.35f : (isHovered ? 0.25f : 0.12f);

        ComPtr<ID2D1SolidColorBrush> btnBg;
        target->CreateSolidColorBrush(
            isCardDark
                ? D2D1::ColorF(1.0f, 1.0f, 1.0f, bgAlpha)
                : D2D1::ColorF(0.0f, 0.0f, 0.0f, bgAlpha * 0.7f),
            btnBg.GetAddressOf()
        );
        target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(prevX, ctrlY), radius, radius), btnBg.Get());

        if (m_textPrimaryBrush)
        {
            // 竖线
            target->DrawLine(
                D2D1::Point2F(prevX - 4.5f, ctrlY - 5.5f),
                D2D1::Point2F(prevX - 4.5f, ctrlY + 5.5f),
                m_textPrimaryBrush.Get(),
                1.8f
            );
            // 三角形向左
            D2D1_POINT_2F p1 = D2D1::Point2F(prevX + 4.5f, ctrlY - 5.5f);
            D2D1_POINT_2F p2 = D2D1::Point2F(prevX - 3.5f, ctrlY);
            D2D1_POINT_2F p3 = D2D1::Point2F(prevX + 4.5f, ctrlY + 5.5f);
            target->DrawLine(p1, p2, m_textPrimaryBrush.Get(), 1.5f);
            target->DrawLine(p2, p3, m_textPrimaryBrush.Get(), 1.5f);
            target->DrawLine(p3, p1, m_textPrimaryBrush.Get(), 1.5f);
        }
    }

    // --- 按钮 2: 播放 / 暂停 (核心主控按钮，触感缩放反馈) ---
    m_playButtonRect = D2D1::RectF(playX - 22.0f, ctrlY - 22.0f, playX + 22.0f, ctrlY + 22.0f);
    {
        bool isHovered = (m_hoveredButton == MusicButtonId::PlayPause);
        bool isPressed = (m_pressedButton == MusicButtonId::PlayPause);
        float radius = isPressed ? 16.0f : (isHovered ? 19.5f : 18.0f);

        // 悬浮外发光光晕
        if (isHovered)
        {
            ComPtr<ID2D1SolidColorBrush> haloBrush;
            target->CreateSolidColorBrush(
                isCardDark
                    ? D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.20f)
                    : D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.10f),
                haloBrush.GetAddressOf()
            );
            target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(playX, ctrlY), radius + 4.0f, radius + 4.0f), haloBrush.Get());
        }

        float mainAlpha = isPressed ? 0.85f : 1.0f;
        ComPtr<ID2D1SolidColorBrush> mainPlayBg;
        target->CreateSolidColorBrush(
            isCardDark
                ? D2D1::ColorF(1.0f, 1.0f, 1.0f, mainAlpha)
                : D2D1::ColorF(0.10f, 0.11f, 0.14f, mainAlpha),
            mainPlayBg.GetAddressOf()
        );
        target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(playX, ctrlY), radius, radius), mainPlayBg.Get());

        if (m_controlIconBrush)
        {
            m_controlIconBrush->SetColor(
                isCardDark
                    ? D2D1::ColorF(0.11f, 0.11f, 0.13f, 1.0f)
                    : D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f)
            );

            if (m_isPlaying)
            {
                // 暂停两条竖杠 (||)
                target->DrawLine(
                    D2D1::Point2F(playX - 3.5f, ctrlY - 6.0f),
                    D2D1::Point2F(playX - 3.5f, ctrlY + 6.0f),
                    m_controlIconBrush.Get(),
                    2.8f
                );
                target->DrawLine(
                    D2D1::Point2F(playX + 3.5f, ctrlY - 6.0f),
                    D2D1::Point2F(playX + 3.5f, ctrlY + 6.0f),
                    m_controlIconBrush.Get(),
                    2.8f
                );
            }
            else
            {
                // 播放三角形向右 (▶)
                D2D1_POINT_2F p1 = D2D1::Point2F(playX - 3.0f, ctrlY - 6.5f);
                D2D1_POINT_2F p2 = D2D1::Point2F(playX + 6.0f, ctrlY);
                D2D1_POINT_2F p3 = D2D1::Point2F(playX - 3.0f, ctrlY + 6.5f);
                target->DrawLine(p1, p2, m_controlIconBrush.Get(), 1.8f);
                target->DrawLine(p2, p3, m_controlIconBrush.Get(), 1.8f);
                target->DrawLine(p3, p1, m_controlIconBrush.Get(), 1.8f);
            }
        }
    }

    // --- 按钮 3: 下一曲 (>>|) ---
    m_nextButtonRect = D2D1::RectF(nextX - 18.0f, ctrlY - 18.0f, nextX + 18.0f, ctrlY + 18.0f);
    {
        bool isHovered = (m_hoveredButton == MusicButtonId::Next);
        bool isPressed = (m_pressedButton == MusicButtonId::Next);
        float radius = isPressed ? 12.5f : (isHovered ? 15.5f : 14.0f);
        float bgAlpha = isPressed ? 0.35f : (isHovered ? 0.25f : 0.12f);

        ComPtr<ID2D1SolidColorBrush> btnBg;
        target->CreateSolidColorBrush(
            isCardDark
                ? D2D1::ColorF(1.0f, 1.0f, 1.0f, bgAlpha)
                : D2D1::ColorF(0.0f, 0.0f, 0.0f, bgAlpha * 0.7f),
            btnBg.GetAddressOf()
        );
        target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(nextX, ctrlY), radius, radius), btnBg.Get());

        if (m_textPrimaryBrush)
        {
            // 竖线
            target->DrawLine(
                D2D1::Point2F(nextX + 4.5f, ctrlY - 5.5f),
                D2D1::Point2F(nextX + 4.5f, ctrlY + 5.5f),
                m_textPrimaryBrush.Get(),
                1.8f
            );
            // 三角形向右
            D2D1_POINT_2F p1 = D2D1::Point2F(nextX - 4.5f, ctrlY - 5.5f);
            D2D1_POINT_2F p2 = D2D1::Point2F(nextX + 3.5f, ctrlY);
            D2D1_POINT_2F p3 = D2D1::Point2F(nextX - 4.5f, ctrlY + 5.5f);
            target->DrawLine(p1, p2, m_textPrimaryBrush.Get(), 1.5f);
            target->DrawLine(p2, p3, m_textPrimaryBrush.Get(), 1.5f);
            target->DrawLine(p3, p1, m_textPrimaryBrush.Get(), 1.5f);
        }
    }
}

void MusicWidget::Shutdown()
{
    m_coverBrush.Reset();
    m_coverBitmap.Reset();
    m_wicFactory.Reset();
    m_loadedCoverPath.clear();

    m_adaptiveBgBrush.Reset();
    m_adaptiveBorderBrush.Reset();

    m_textPrimaryBrush.Reset();
    m_textSecondaryBrush.Reset();
    m_accentBrush.Reset();
    m_controlBgBrush.Reset();
    m_controlIconBrush.Reset();
    m_borderBrush.Reset();
    m_shadowBrush.Reset();

    m_sourceFormat.Reset();
    m_titleFormat.Reset();
    m_artistFormat.Reset();
}

} // namespace Windget
