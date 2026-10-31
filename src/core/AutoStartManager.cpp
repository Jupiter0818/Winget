#include "core/AutoStartManager.h"
#include <vector>

namespace Windget {

AutoStartManager& AutoStartManager::GetInstance()
{
    static AutoStartManager instance;
    return instance;
}

std::wstring AutoStartManager::GetExecutablePath() const
{
    wchar_t buffer[MAX_PATH];
    DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length > 0 && length < MAX_PATH)
    {
        return std::wstring(buffer);
    }
    return L"";
}

bool AutoStartManager::IsAutoStartEnabled() const
{
    HKEY hKey = nullptr;
    LSTATUS status = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        REG_KEY_PATH,
        0,
        KEY_READ,
        &hKey
    );

    if (status != ERROR_SUCCESS)
    {
        return false;
    }

    wchar_t value[MAX_PATH * 2]{};
    DWORD bufferSize = sizeof(value);
    DWORD type = 0;

    status = RegQueryValueExW(
        hKey,
        APP_VALUE_NAME,
        nullptr,
        &type,
        reinterpret_cast<LPBYTE>(value),
        &bufferSize
    );

    RegCloseKey(hKey);

    return (status == ERROR_SUCCESS);
}

bool AutoStartManager::SetAutoStartEnabled(bool enable)
{
    HKEY hKey = nullptr;
    LSTATUS status = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        REG_KEY_PATH,
        0,
        KEY_SET_VALUE,
        &hKey
    );

    if (status != ERROR_SUCCESS)
    {
        return false;
    }

    bool success = false;
    if (enable)
    {
        std::wstring exePath = GetExecutablePath();
        if (!exePath.empty())
        {
            // 为防止路径中包含空格，必须用双引号包裹
            std::wstring quotedPath = L"\"" + exePath + L"\"";
            status = RegSetValueExW(
                hKey,
                APP_VALUE_NAME,
                0,
                REG_SZ,
                reinterpret_cast<const BYTE*>(quotedPath.c_str()),
                static_cast<DWORD>((quotedPath.length() + 1) * sizeof(wchar_t))
            );
            success = (status == ERROR_SUCCESS);
        }
    }
    else
    {
        status = RegDeleteValueW(hKey, APP_VALUE_NAME);
        // 如果值原本就不存在，也视为操作成功
        success = (status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND);
    }

    RegCloseKey(hKey);
    return success;
}

} // namespace Windget
