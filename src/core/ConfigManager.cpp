#include "core/ConfigManager.h"
#include "utils/JsonUtils.h"
#include <windows.h>

namespace Windget {

ConfigManager& ConfigManager::GetInstance()
{
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager()
{
    m_configFilePath = ResolveConfigPath();
}

ConfigManager::~ConfigManager() = default;

std::filesystem::path ConfigManager::ResolveConfigPath()
{
    // 1. 优先检查可执行文件同级目录 (便携模式)
    wchar_t exePathBuffer[MAX_PATH];
    DWORD exeLen = GetModuleFileNameW(nullptr, exePathBuffer, MAX_PATH);
    if (exeLen > 0 && exeLen < MAX_PATH)
    {
        std::filesystem::path exeDir = std::filesystem::path(exePathBuffer).parent_path();
        std::filesystem::path localConfig = exeDir / L"config.json";
        if (std::filesystem::exists(localConfig))
        {
            return localConfig;
        }
    }

    // 2. 默认推荐标准路径：%APPDATA%\Windget\config.json
    wchar_t appDataBuffer[MAX_PATH];
    DWORD len = GetEnvironmentVariableW(L"APPDATA", appDataBuffer, MAX_PATH);
    if (len > 0 && len < MAX_PATH)
    {
        std::filesystem::path configDir = std::filesystem::path(appDataBuffer) / L"Windget";
        if (CreateDirectoryW(configDir.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS)
        {
            return configDir / L"config.json";
        }
    }

    // 3. 在沙盒或权限受限时，安全回退至当前工作目录
    return std::filesystem::path(L"config.json");
}

void ConfigManager::ResetToDefault()
{
    m_config.themeMode = "system";
    m_config.snapToGrid = true;
    m_config.lockPosition = false;
    m_config.widgets.clear();

    WidgetConfig defaultClock;
    defaultClock.id = "clock_default";
    defaultClock.type = "clock";
    defaultClock.x = 200;
    defaultClock.y = 200;
    defaultClock.width = 320;
    defaultClock.height = 170;
    defaultClock.theme = "dark";
    defaultClock.opacity = 0.85f;
    defaultClock.fontSize = 56;

    m_config.widgets.push_back(defaultClock);
}

bool ConfigManager::Load()
{
    HANDLE hFile = CreateFileW(
        m_configFilePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        // 配置文件不存在，自动生成默认配置并保存
        ResetToDefault();
        Save();
        return true;
    }

    DWORD fileSize = GetFileSize(hFile, nullptr);
    if (fileSize == INVALID_FILE_SIZE || fileSize == 0)
    {
        CloseHandle(hFile);
        ResetToDefault();
        return false;
    }

    std::string jsonContent(fileSize, '\0');
    DWORD bytesRead = 0;
    BOOL readOk = ReadFile(hFile, jsonContent.data(), fileSize, &bytesRead, nullptr);
    CloseHandle(hFile);

    if (!readOk || bytesRead == 0)
    {
        ResetToDefault();
        return false;
    }

    jsonContent.resize(bytesRead);

    std::string parseError;
    JsonValue root = JsonValue::Parse(jsonContent, &parseError);

    // 容错处理：若 JSON 损坏或格式错误，安全回退到默认配置，杜绝程序崩溃
    if (!root.IsObject() || !root.Contains("widgets"))
    {
        ResetToDefault();
        return false;
    }

    m_config.widgets.clear();
    m_config.themeMode = root.Contains("themeMode") ? root["themeMode"].AsString("system") : "system";
    m_config.snapToGrid = root.Contains("snapToGrid") ? root["snapToGrid"].AsBool(true) : true;
    m_config.lockPosition = root.Contains("lockPosition") ? root["lockPosition"].AsBool(false) : false;
    const auto& widgetsArr = root["widgets"].AsArray();

    for (const auto& item : widgetsArr)
    {
        if (!item.IsObject()) continue;

        WidgetConfig cfg;
        cfg.type = item["type"].AsString("clock");
        cfg.id = item["id"].AsString(cfg.type + "_main");
        cfg.x = item["x"].AsInt(200);
        cfg.y = item["y"].AsInt(200);
        cfg.width = item["width"].AsInt(320);
        cfg.height = item["height"].AsInt(170);
        cfg.theme = item["theme"].AsString("dark");
        cfg.opacity = static_cast<float>(item["opacity"].AsDouble(0.85));
        cfg.fontSize = item["fontSize"].AsInt(56);

        m_config.widgets.push_back(cfg);
    }

    if (m_config.widgets.empty())
    {
        ResetToDefault();
    }

    return true;
}

bool ConfigManager::Save()
{
    JsonValue root;
    JsonValue arr(std::vector<JsonValue>{});

    for (const auto& cfg : m_config.widgets)
    {
        JsonValue obj;
        obj.Set("id", JsonValue(cfg.id));
        obj.Set("type", JsonValue(cfg.type));
        obj.Set("x", JsonValue(cfg.x));
        obj.Set("y", JsonValue(cfg.y));
        obj.Set("width", JsonValue(cfg.width));
        obj.Set("height", JsonValue(cfg.height));
        obj.Set("theme", JsonValue(cfg.theme));
        obj.Set("opacity", JsonValue(static_cast<double>(cfg.opacity)));
        obj.Set("fontSize", JsonValue(cfg.fontSize));
        arr.PushBack(std::move(obj));
    }

    root.Set("themeMode", JsonValue(m_config.themeMode));
    root.Set("snapToGrid", JsonValue(m_config.snapToGrid));
    root.Set("lockPosition", JsonValue(m_config.lockPosition));
    root.Set("widgets", std::move(arr));

    std::string jsonStr = root.Serialize(4);

    if (m_configFilePath.has_parent_path())
    {
        CreateDirectoryW(m_configFilePath.parent_path().c_str(), nullptr);
    }

    // 原生 Win32 写入文件，完美规避多字节编码及路径空格问题
    HANDLE hFile = CreateFileW(
        m_configFilePath.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    DWORD bytesWritten = 0;
    BOOL writeOk = WriteFile(
        hFile,
        jsonStr.data(),
        static_cast<DWORD>(jsonStr.size()),
        &bytesWritten,
        nullptr
    );

    CloseHandle(hFile);
    return writeOk && (bytesWritten == jsonStr.size());
}

void ConfigManager::UpdateWidgetPosition(const std::string& id, int x, int y)
{
    for (auto& cfg : m_config.widgets)
    {
        if (cfg.id == id)
        {
            cfg.x = x;
            cfg.y = y;
            return;
        }
    }

    // 若未通过 ID 匹配且只有一个 Widget，则更新该 Widget
    if (m_config.widgets.size() == 1)
    {
        m_config.widgets[0].x = x;
        m_config.widgets[0].y = y;
    }
}

void ConfigManager::UpdateWidgetTheme(const std::string& id, const std::string& theme)
{
    for (auto& cfg : m_config.widgets)
    {
        if (cfg.id == id)
        {
            cfg.theme = theme;
            return;
        }
    }

    if (m_config.widgets.size() == 1)
    {
        m_config.widgets[0].theme = theme;
    }
}

} // namespace Windget
