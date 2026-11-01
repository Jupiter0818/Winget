#pragma once

#include <windows.h>
#include <string>

namespace Windget {

struct CurrentMediaInfo
{
    std::wstring title;
    std::wstring artist;
    std::wstring appName;
    std::wstring coverPath;
    bool isPlaying = false;
    bool hasActiveMedia = false;
};

class MediaManager
{
public:
    static MediaManager& GetInstance();

    // 启动系统级 GSMTC 媒体监听服务
    bool Initialize();

    // 停止并清理后台服务
    void Shutdown();

    // 获取当前正在播放的媒体信息 (支持 Apple Music、Spotify 等)
    CurrentMediaInfo GetCurrentMedia();

    // 发送媒体控制指令 (PLAY_PAUSE / NEXT / PREV)
    void SendCommand(const std::string& cmd);

private:
    MediaManager() = default;
    ~MediaManager();

    MediaManager(const MediaManager&) = delete;
    MediaManager& operator=(const MediaManager&) = delete;

    void EnsureDaemonScriptExists();
    void LaunchDaemonProcess();

private:
    HANDLE m_hDaemonProcess = nullptr;
    std::wstring m_daemonScriptPath;
    std::wstring m_mediaJsonPath;
    std::wstring m_cmdFilePath;
    bool m_initialized = false;
    ULONGLONG m_lastCheckTick = 0;
    CurrentMediaInfo m_cachedInfo{};
};

} // namespace Windget
