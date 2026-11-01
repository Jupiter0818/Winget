#pragma once

#include <windows.h>
#include <string>

namespace Windget {

enum class AppThemeMode
{
    System = 0, // 跟随 Windows 系统偏好 (默认)
    Dark = 1,   // 强制深色毛玻璃
    Light = 2   // 强制浅色毛玻璃
};

class ThemeManager
{
public:
    static ThemeManager& GetInstance();

    // 初始化主题管理器并加载配置
    void Initialize();

    // 模式切换与查询
    void SetThemeMode(AppThemeMode mode);
    void SetThemeModeString(const std::string& modeStr);
    AppThemeMode GetThemeMode() const { return m_mode; }
    std::string GetThemeModeString() const;

    // 当前生效的视觉外观判定 (true: 深色模式, false: 浅色模式)
    bool IsDarkMode() const;

    // 读取 Windows 系统注册表当前深浅色模式 (true: 深色, false: 浅色)
    bool QueryWindowsSystemIsDark() const;

    // 响应 Windows WM_SETTINGCHANGE 广播通知
    bool CheckAndUpdateSystemTheme();

private:
    ThemeManager();
    ~ThemeManager() = default;

    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

private:
    AppThemeMode m_mode = AppThemeMode::System;
    mutable bool m_cachedWindowsIsDark = false;
    mutable ULONGLONG m_lastQueryTick = 0;
};

} // namespace Windget
