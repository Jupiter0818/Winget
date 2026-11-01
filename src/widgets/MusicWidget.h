#pragma once

#include "widgets/IWidget.h"
#include <wrl/client.h>
#include <string>
#include <vector>
#include <filesystem>
#include <wincodec.h>

namespace Windget {

using Microsoft::WRL::ComPtr;

enum class MusicThemeMode
{
    Dark,       // 经典黑曜
    Adaptive    // 封面自适应取色 (Apple Music 风格)
};

enum class MusicButtonId
{
    None = 0,
    Prev,
    PlayPause,
    Next,
    ThemeToggle
};

class MusicWidget : public IWidget
{
public:
    explicit MusicWidget(std::string id = "music_default");
    ~MusicWidget() override;

    std::string GetId() const override { return m_id; }
    std::string GetType() const override { return "music"; }

    float GetWidth() const override { return 320.0f; }
    float GetHeight() const override { return 170.0f; }
    uint32_t GetUpdateIntervalMs() const override { return 1000; } // 1秒更新一次播放状态

    void Initialize(IDWriteFactory* dwriteFactory) override;
    void Update() override;
    void Render(
        ID2D1DCRenderTarget* target,
        IDWriteFactory* dwriteFactory,
        const D2D1_RECT_F& cardBounds
    ) override;
    void Shutdown() override;

    // 交互式点击响应 (播放/暂停/上一曲/下一曲/主题切换)
    bool OnClick(float localX, float localY) override;

    // 悬浮与按压反馈
    bool OnMouseMove(float localX, float localY) override;
    void OnMouseLeave() override;
    bool OnMouseDown(float localX, float localY) override;
    bool OnMouseUp(float localX, float localY) override;
    bool WantsHandCursor() const override { return m_hoveredButton != MusicButtonId::None; }

    // 主题切换接口
    void SetThemeMode(MusicThemeMode mode);
    MusicThemeMode GetThemeMode() const { return m_themeMode; }
    void ToggleThemeMode();

private:
    void RefreshMediaStatus();
    void SendMediaKey(BYTE vk);
    bool LoadCoverBitmap(ID2D1DCRenderTarget* target, const std::wstring& path);
    void ExtractDominantColor(IWICFormatConverter* converter);
    void CreateAdaptiveBrushes(ID2D1DCRenderTarget* target, const D2D1_RECT_F& cardBounds);

private:
    std::string m_id;
    MusicThemeMode m_themeMode = MusicThemeMode::Adaptive;

    // 交互状态
    MusicButtonId m_hoveredButton = MusicButtonId::None;
    MusicButtonId m_pressedButton = MusicButtonId::None;

    // 媒体元数据
    std::wstring m_songTitle;
    std::wstring m_artistName;
    std::wstring m_playerName;
    std::wstring m_coverPath;
    std::wstring m_loadedCoverPath;
    std::filesystem::file_time_type m_lastCoverWriteTime{};
    std::wstring m_lastTrackKey;
    bool m_isPlaying = false;

    // 按钮交互感应热区 (DIP)
    D2D1_RECT_F m_prevButtonRect{};
    D2D1_RECT_F m_playButtonRect{};
    D2D1_RECT_F m_nextButtonRect{};
    D2D1_RECT_F m_themeToggleRect{};

    // 自适应取色提取出的核心色相
    D2D1_COLOR_F m_dominantColor = D2D1::ColorF(0.20f, 0.20f, 0.26f, 1.0f);

    // WIC 图像解码与 Direct2D 纹理画刷
    ComPtr<IWICImagingFactory> m_wicFactory;
    ComPtr<ID2D1Bitmap> m_coverBitmap;
    ComPtr<ID2D1BitmapBrush> m_coverBrush;

    // DirectWrite 文本格式
    ComPtr<IDWriteTextFormat> m_sourceFormat;
    ComPtr<IDWriteTextFormat> m_titleFormat;
    ComPtr<IDWriteTextFormat> m_artistFormat;

    // Direct2D 矢量画刷
    ComPtr<ID2D1SolidColorBrush> m_textPrimaryBrush;
    ComPtr<ID2D1SolidColorBrush> m_textSecondaryBrush;
    ComPtr<ID2D1SolidColorBrush> m_accentBrush;
    ComPtr<ID2D1SolidColorBrush> m_controlBgBrush;
    ComPtr<ID2D1SolidColorBrush> m_controlIconBrush;
    ComPtr<ID2D1SolidColorBrush> m_borderBrush;
    ComPtr<ID2D1SolidColorBrush> m_shadowBrush;

    // 自适应背景渐变画刷
    ComPtr<ID2D1LinearGradientBrush> m_adaptiveBgBrush;
    ComPtr<ID2D1SolidColorBrush> m_adaptiveBorderBrush;
};

} // namespace Windget
