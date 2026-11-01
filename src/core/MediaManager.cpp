#include "core/MediaManager.h"
#include "core/ConfigManager.h"
#include "utils/JsonUtils.h"
#include <fstream>
#include <sstream>
#include <cstring>

namespace Windget {

MediaManager& MediaManager::GetInstance()
{
    static MediaManager instance;
    return instance;
}

MediaManager::~MediaManager()
{
    Shutdown();
}

bool MediaManager::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

    std::filesystem::path configDir = ConfigManager::GetInstance().GetConfigDirectory();
    m_daemonScriptPath = (configDir / "media_daemon.ps1").wstring();
    m_mediaJsonPath = (configDir / "media.json").wstring();
    m_cmdFilePath = (configDir / "media_cmd.txt").wstring();

    EnsureDaemonScriptExists();
    LaunchDaemonProcess();

    m_initialized = true;
    return true;
}

void MediaManager::Shutdown()
{
    if (m_hDaemonProcess)
    {
        TerminateProcess(m_hDaemonProcess, 0);
        CloseHandle(m_hDaemonProcess);
        m_hDaemonProcess = nullptr;
    }
    m_initialized = false;
}

void MediaManager::EnsureDaemonScriptExists()
{
    // Pure ASCII script to avoid PowerShell 5.1 character encoding parser issues
    static const char* DAEMON_SCRIPT = R"POWERSHELL(param([int]$ParentPid = 0)

$ErrorActionPreference = 'SilentlyContinue'

[Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager, Windows.Media.Control, ContentType = WindowsRuntime] | Out-Null
Add-Type -AssemblyName System.Runtime.WindowsRuntime

$asTaskGeneric = ([System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object { 
    $_.Name -eq 'AsTask' -and 
    $_.GetParameters().Count -eq 1 -and 
    $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1' 
})[0]

function Await($WinRtTask, $ResultType) {
    if (-not $WinRtTask) { return $null }
    try {
        $asTask = $asTaskGeneric.MakeGenericMethod($ResultType)
        $netTask = $asTask.Invoke($null, @($WinRtTask))
        $netTask.Wait(1200) | Out-Null
        return $netTask.Result
    } catch {
        return $null
    }
}

$appData = [Environment]::GetFolderPath('ApplicationData')
$outDir = Join-Path $appData "Windget"
if (-not (Test-Path $outDir)) {
    New-Item -ItemType Directory -Path $outDir -Force | Out-Null
}
$outFile = Join-Path $outDir "media.json"
$tmpFile = Join-Path $outDir "media.tmp"
$cmdFile = Join-Path $outDir "media_cmd.txt"
$coverFile = ""
$coverIndex = 0
$utf8NoBom = New-Object System.Text.UTF8Encoding $false

$sessionManager = $null
$lastTrackKey = ""

function Handle-Command($mgr, $file) {
    if (-not (Test-Path $file)) { return }
    $cmd = (Get-Content $file -Raw).Trim()
    Remove-Item $file -Force -ErrorAction SilentlyContinue
    if (-not $mgr) { return }

    $target = $null
    $sessions = $mgr.GetSessions()
    foreach ($s in $sessions) {
        $pb = $s.GetPlaybackInfo()
        if ($pb -and ($pb.PlaybackStatus -eq 4 -or $pb.PlaybackStatus -eq 'Playing')) {
            $target = $s
            break
        }
    }
    if (-not $target) {
        $target = $mgr.GetCurrentSession()
    }
    if (-not $target -and $sessions.Count -gt 0) {
        $target = $sessions[0]
    }

    if ($target) {
        if ($cmd -eq "PAUSE") {
            $null = $target.TryPauseAsync()
        } elseif ($cmd -eq "PLAY") {
            $null = $target.TryPlayAsync()
        } elseif ($cmd -eq "PLAY_PAUSE") {
            $null = $target.TryTogglePlayPauseAsync()
        } elseif ($cmd -eq "NEXT") {
            $null = $target.TrySkipNextAsync()
        } elseif ($cmd -eq "PREV") {
            $null = $target.TrySkipPreviousAsync()
        }
    }
}

while ($true) {
    if ($ParentPid -gt 0) {
        $p = Get-Process -Id $ParentPid -ErrorAction SilentlyContinue
        if (-not $p) {
            break
        }
    }

    try {
        if (-not $sessionManager) {
            $sessionManager = Await ([Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager]::RequestAsync()) ([Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager])
        }

        # Handle any pending commands immediately
        Handle-Command $sessionManager $cmdFile

        if ($sessionManager) {
            $sessions = $sessionManager.GetSessions()
            $targetSession = $null

            foreach ($s in $sessions) {
                $pb = $s.GetPlaybackInfo()
                if ($pb -and ($pb.PlaybackStatus -eq 4 -or $pb.PlaybackStatus -eq 'Playing')) {
                    $targetSession = $s
                    break
                }
            }

            if (-not $targetSession) {
                $targetSession = $sessionManager.GetCurrentSession()
            }
            if (-not $targetSession -and $sessions.Count -gt 0) {
                $targetSession = $sessions[0]
            }

            if ($targetSession) {
                $props = Await ($targetSession.TryGetMediaPropertiesAsync()) ([Windows.Media.Control.GlobalSystemMediaTransportControlsSessionMediaProperties])
                $pb = $targetSession.GetPlaybackInfo()
                $status = if ($pb -and ($pb.PlaybackStatus -eq 4 -or $pb.PlaybackStatus -eq 'Playing')) { "Playing" } else { "Paused" }

                $appId = $targetSession.SourceAppUserModelId
                $appName = "NOW PLAYING"
                if ($appId -like "*AppleMusic*") { $appName = "Apple Music" }
                elseif ($appId -like "*Spotify*") { $appName = "Spotify" }
                elseif ($appId -like "*CloudMusic*" -or $appId -like "*Netease*") { $appName = "NetEase Music" }
                elseif ($appId -like "*QQMusic*") { $appName = "QQ Music" }
                elseif ($appId -like "*Chrome*") { $appName = "Chrome" }
                elseif ($appId -like "*msedge*") { $appName = "Edge" }

                $title = if ($props -and $props.Title) { $props.Title } else { "" }
                $artist = if ($props -and $props.Artist) { $props.Artist } else { "" }
                $album = if ($props -and $props.AlbumTitle) { $props.AlbumTitle } else { "" }

                # Extract album artwork if track changed or missing
                $currentTrackKey = "$title - $artist - $album"
                if ($currentTrackKey -ne $lastTrackKey -or -not (Test-Path $coverFile)) {
                    $nextIndex = 1 - $coverIndex
                    $targetCover = Join-Path $outDir "cover_$nextIndex.jpg"
                    $tmpCover = Join-Path $outDir "cover_$nextIndex.tmp"
                    $gotCover = $false

                    if ($props -and $props.Thumbnail) {
                        try {
                            $stream = Await ($props.Thumbnail.OpenReadAsync()) ([Windows.Storage.Streams.IRandomAccessStreamWithContentType])
                            if ($stream) {
                                $asStreamMethod = [System.IO.WindowsRuntimeStreamExtensions].GetMethod('AsStreamForRead', [Type[]]@([Windows.Storage.Streams.IInputStream]))
                                $netStream = $asStreamMethod.Invoke($null, @($stream))
                                $fileStream = [System.IO.File]::Create($tmpCover)
                                $netStream.CopyTo($fileStream)
                                $fileStream.Close()
                                $netStream.Close()
                                Move-Item -LiteralPath $tmpCover -Destination $targetCover -Force -ErrorAction SilentlyContinue
                                if (Test-Path $targetCover) {
                                    $coverIndex = $nextIndex
                                    $coverFile = $targetCover
                                    $gotCover = $true
                                    $lastTrackKey = $currentTrackKey
                                }
                            }
                        } catch {}
                    }

                    if (-not $gotCover) {
                        if (-not ($props -and $props.Thumbnail)) {
                            $coverFile = ""
                            $lastTrackKey = $currentTrackKey
                        }
                    }
                }

                $cover = if (Test-Path $coverFile) { $coverFile } else { "" }

                $data = @{
                    title = $title
                    artist = $artist
                    app = $appName
                    status = $status
                    cover = $cover
                }
                $json = $data | ConvertTo-Json -Compress
                [System.IO.File]::WriteAllText($tmpFile, $json, $utf8NoBom)
                Move-Item -LiteralPath $tmpFile -Destination $outFile -Force -ErrorAction SilentlyContinue
            } else {
                $coverFile = ""
                $lastTrackKey = ""
                $data = @{
                    title = ""
                    artist = ""
                    app = "NOW PLAYING"
                    status = "Stopped"
                    cover = ""
                }
                $json = $data | ConvertTo-Json -Compress
                [System.IO.File]::WriteAllText($tmpFile, $json, $utf8NoBom)
                Move-Item -LiteralPath $tmpFile -Destination $outFile -Force -ErrorAction SilentlyContinue
            }
        }
    } catch {
        $sessionManager = $null
    }

    # Fast polling for command requests (5 x 100ms = 500ms total)
    for ($k = 0; $k -lt 5; $k++) {
        Start-Sleep -Milliseconds 100
        if (Test-Path $cmdFile) {
            Handle-Command $sessionManager $cmdFile
            break
        }
    }
}
)POWERSHELL";

    std::ofstream out(m_daemonScriptPath.c_str(), std::ios::binary);
    if (out.is_open())
    {
        out.write(DAEMON_SCRIPT, std::strlen(DAEMON_SCRIPT));
        out.close();
    }
}

void MediaManager::LaunchDaemonProcess()
{
    // 如果已有活跃句柄，检查其存活
    if (m_hDaemonProcess)
    {
        DWORD exitCode = 0;
        if (GetExitCodeProcess(m_hDaemonProcess, &exitCode) && exitCode == STILL_ACTIVE)
        {
            return;
        }
        CloseHandle(m_hDaemonProcess);
        m_hDaemonProcess = nullptr;
    }

    DWORD myPid = GetCurrentProcessId();
    std::wstring cmdLine = L"powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File \"" +
                           m_daemonScriptPath + L"\" -ParentPid " + std::to_wstring(myPid);

    STARTUPINFOW si{};
    si.cb = sizeof(STARTUPINFOW);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi{};
    BOOL success = CreateProcessW(
        nullptr,
        cmdLine.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (success)
    {
        m_hDaemonProcess = pi.hProcess;
        CloseHandle(pi.hThread);
    }
}

CurrentMediaInfo MediaManager::GetCurrentMedia()
{
    // 限频检查守护进程存活状态 (每 5 秒最多检查一次，杜绝主 UI 线程卡顿)
    ULONGLONG now = GetTickCount64();
    if (now - m_lastCheckTick > 5000)
    {
        m_lastCheckTick = now;
        DWORD exitCode = 0;
        if (!m_hDaemonProcess || (GetExitCodeProcess(m_hDaemonProcess, &exitCode) && exitCode != STILL_ACTIVE))
        {
            LaunchDaemonProcess();
        }
    }

    // 读取 media.json
    std::ifstream file(m_mediaJsonPath.c_str());
    if (!file.is_open())
    {
        return m_cachedInfo;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    file.close();

    std::string jsonStr = ss.str();
    if (jsonStr.empty())
    {
        return m_cachedInfo;
    }

    // 跳过 UTF-8 BOM (PowerShell WriteAllText 默认会附带 EF BB BF)
    if (jsonStr.size() >= 3 &&
        static_cast<unsigned char>(jsonStr[0]) == 0xEF &&
        static_cast<unsigned char>(jsonStr[1]) == 0xBB &&
        static_cast<unsigned char>(jsonStr[2]) == 0xBF)
    {
        jsonStr = jsonStr.substr(3);
    }

    std::string err;
    JsonValue root = JsonValue::Parse(jsonStr, &err);
    if (!root.IsObject())
    {
        return m_cachedInfo;
    }

    std::string titleUtf8 = root.Contains("title") ? root["title"].AsString() : "";
    std::string artistUtf8 = root.Contains("artist") ? root["artist"].AsString() : "";
    std::string appUtf8 = root.Contains("app") ? root["app"].AsString() : "";
    std::string status = root.Contains("status") ? root["status"].AsString() : "";
    std::string coverUtf8 = root.Contains("cover") ? root["cover"].AsString() : "";

    auto utf8ToWstring = [](const std::string& str) -> std::wstring {
        if (str.empty()) return L"";
        int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        if (size <= 1) return L"";
        std::wstring result(size - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size);
        return result;
    };

    std::wstring titleW = utf8ToWstring(titleUtf8);
    std::wstring artistW = utf8ToWstring(artistUtf8);
    std::wstring appW = utf8ToWstring(appUtf8);
    std::wstring coverW = utf8ToWstring(coverUtf8);

    CurrentMediaInfo info{};
    info.title = titleW;
    info.artist = artistW;
    info.appName = appW.empty() ? L"NOW PLAYING" : appW;
    info.coverPath = coverW;
    info.isPlaying = (status == "Playing");

    // 只要有非空标题就视为有活跃媒体 (暂停状态也应展示歌曲信息)
    info.hasActiveMedia = !titleW.empty();

    m_cachedInfo = info;
    return info;
}

void MediaManager::SendCommand(const std::string& cmd)
{
    // 1. 写入指令文件通知后台守护进程执行 WinRT GSMTC 精准控制
    std::ofstream out(m_cmdFilePath.c_str(), std::ios::trunc);
    if (out.is_open())
    {
        out << cmd;
        out.close();
    }

    // 2. 仅在守护进程未运行 (例如非 Win10/11 系统或未初始化) 时作为后备触发传统按键
    DWORD exitCode = 0;
    if (!m_hDaemonProcess || (GetExitCodeProcess(m_hDaemonProcess, &exitCode) && exitCode != STILL_ACTIVE))
    {
        if (cmd == "PLAY_PAUSE" || cmd == "PLAY" || cmd == "PAUSE")
        {
            keybd_event(VK_MEDIA_PLAY_PAUSE, 0, 0, 0);
            keybd_event(VK_MEDIA_PLAY_PAUSE, 0, KEYEVENTF_KEYUP, 0);
        }
        else if (cmd == "NEXT")
        {
            keybd_event(VK_MEDIA_NEXT_TRACK, 0, 0, 0);
            keybd_event(VK_MEDIA_NEXT_TRACK, 0, KEYEVENTF_KEYUP, 0);
        }
        else if (cmd == "PREV")
        {
            keybd_event(VK_MEDIA_PREV_TRACK, 0, 0, 0);
            keybd_event(VK_MEDIA_PREV_TRACK, 0, KEYEVENTF_KEYUP, 0);
        }
    }
}

} // namespace Windget
