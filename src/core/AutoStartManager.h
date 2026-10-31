#pragma once

#include <windows.h>
#include <string>

namespace Windget {

class AutoStartManager
{
public:
    static AutoStartManager& GetInstance();

    // 检查当前是否已启用开机自启
    bool IsAutoStartEnabled() const;

    // 设置开机自启状态 (true: 写入注册表 Run 键，false: 删除该键)
    bool SetAutoStartEnabled(bool enable);

private:
    AutoStartManager() = default;
    ~AutoStartManager() = default;

    AutoStartManager(const AutoStartManager&) = delete;
    AutoStartManager& operator=(const AutoStartManager&) = delete;

    // 获取当前运行的可执行文件完整路径
    std::wstring GetExecutablePath() const;

private:
    const wchar_t* REG_KEY_PATH = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    const wchar_t* APP_VALUE_NAME = L"Windget";
};

} // namespace Windget
