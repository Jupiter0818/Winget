#include "core/ThemeManager.h"
#include "core/ConfigManager.h"

namespace Windget {

ThemeManager& ThemeManager::GetInstance()
{
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager()
{
    m_cachedWindowsIsDark = QueryWindowsSystemIsDark();
    m_lastQueryTick = GetTickCount64();
}

void ThemeManager::Initialize()
{
    const std::string& saved = ConfigManager::GetInstance().GetThemeMode();
    SetThemeModeString(saved);
    m_cachedWindowsIsDark = QueryWindowsSystemIsDark();
}

void ThemeManager::SetThemeMode(AppThemeMode mode)
{
    m_mode = mode;
    ConfigManager::GetInstance().SetThemeMode(GetThemeModeString());
    ConfigManager::GetInstance().Save();
}

void ThemeManager::SetThemeModeString(const std::string& modeStr)
{
    if (modeStr == "dark")
    {
        m_mode = AppThemeMode::Dark;
    }
    else if (modeStr == "light")
    {
        m_mode = AppThemeMode::Light;
    }
    else
    {
        m_mode = AppThemeMode::System;
    }
}

std::string ThemeManager::GetThemeModeString() const
{
    switch (m_mode)
    {
    case AppThemeMode::Dark:
        return "dark";
    case AppThemeMode::Light:
        return "light";
    case AppThemeMode::System:
    default:
        return "system";
    }
}

bool ThemeManager::QueryWindowsSystemIsDark() const
{
    // 读取注册表: HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize -> AppsUseLightTheme
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0,
        KEY_READ,
        &hKey
    ) == ERROR_SUCCESS)
    {
        DWORD value = 0;
        DWORD size = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return (value == 0); // 0 为深色模式，1 为浅色模式
        }
        RegCloseKey(hKey);
    }
    return false; // 默认回退
}

bool ThemeManager::CheckAndUpdateSystemTheme()
{
    bool newIsDark = QueryWindowsSystemIsDark();
    bool changed = (newIsDark != m_cachedWindowsIsDark);
    m_cachedWindowsIsDark = newIsDark;
    m_lastQueryTick = GetTickCount64();
    return changed;
}

bool ThemeManager::IsDarkMode() const
{
    if (m_mode == AppThemeMode::Dark)
    {
        return true;
    }
    if (m_mode == AppThemeMode::Light)
    {
        return false;
    }

    // 跟随系统模式：限频 500ms 检查系统深浅色偏好
    ULONGLONG now = GetTickCount64();
    if (now - m_lastQueryTick > 500)
    {
        m_lastQueryTick = now;
        m_cachedWindowsIsDark = QueryWindowsSystemIsDark();
    }
    return m_cachedWindowsIsDark;
}

} // namespace Windget
