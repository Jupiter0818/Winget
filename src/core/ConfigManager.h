#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace Windget {

struct WidgetConfig
{
    std::string id = "clock_default";
    std::string type = "clock";
    int x = 200;
    int y = 200;
    int width = 320;
    int height = 170;
    std::string theme = "dark";
    float opacity = 0.85f;
    int fontSize = 56;
};

struct AppConfig
{
    std::string themeMode = "system"; // "system", "dark", "light"
    bool snapToGrid = true;
    bool lockPosition = false;
    std::vector<WidgetConfig> widgets;
};

class ConfigManager
{
public:
    static ConfigManager& GetInstance();

    // 加载配置文件 (如果不存在则自动创建默认配置)
    bool Load();

    // 保存当前配置到 %APPDATA%\Windget\config.json
    bool Save();

    // 更新小组件屏幕位置与外观主题
    void UpdateWidgetPosition(const std::string& id, int x, int y);
    void UpdateWidgetTheme(const std::string& id, const std::string& theme);

    // 全局深浅色外观模式 ("system", "dark", "light")
    const std::string& GetThemeMode() const { return m_config.themeMode; }
    void SetThemeMode(const std::string& mode) { m_config.themeMode = mode; }

    // 桌面网格吸附与位置锁定设置
    bool IsSnapToGrid() const { return m_config.snapToGrid; }
    void SetSnapToGrid(bool enabled) { m_config.snapToGrid = enabled; }
    bool IsLockPosition() const { return m_config.lockPosition; }
    void SetLockPosition(bool locked) { m_config.lockPosition = locked; }

    // 获取完整应用配置
    const AppConfig& GetConfig() const { return m_config; }
    AppConfig& GetConfig() { return m_config; }

    // 获取配置文件完整路径与目录
    std::filesystem::path GetConfigFilePath() const { return m_configFilePath; }
    std::filesystem::path GetConfigDirectory() const { return m_configFilePath.parent_path(); }

    // 重置并生成合理的默认配置
    void ResetToDefault();

private:
    ConfigManager();
    ~ConfigManager();

    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    std::filesystem::path ResolveConfigPath();

private:
    AppConfig m_config;
    std::filesystem::path m_configFilePath;
};

} // namespace Windget
