#pragma once

#include <windows.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include "core/WidgetWindow.h"
#include "widgets/IWidget.h"

namespace Windget {

class WidgetManager
{
public:
    using WidgetCreator = std::function<std::shared_ptr<IWidget>(const std::string& id)>;

    static WidgetManager& GetInstance();

    // 初始化管理器并根据配置恢复所有小组件
    bool Initialize(HINSTANCE hInstance);

    // 注册 Widget 类型创建工厂
    void RegisterWidgetType(const std::string& type, WidgetCreator creator);

    // 根据类型创建并启动一个 Widget 桌面窗口实例
    WidgetWindow* CreateWidget(const std::string& type, int x, int y, const std::string& id = "");

    // 销毁指定 Widget 实例
    bool DestroyWidget(WidgetWindow* pWindow);

    // 刷新全部小组件
    void RefreshAll();

    // 切换所有小组件显示/隐藏状态
    void ToggleVisibility();

    // 将所有小组件恢复至屏幕右侧安全工作区
    void ResetPositions();

    // 在屏幕右侧推荐位置添加指定类型的小组件
    WidgetWindow* AddWidgetAtDefaultPosition(const std::string& type);

    // 销毁并关闭全部 Widget 实例 (同时保存最终状态)
    void Shutdown();

    // 获取所有活动的 Widget 窗口列表
    const std::vector<std::unique_ptr<WidgetWindow>>& GetWidgets() const { return m_windows; }

    // 获取当前活动的 Widget 数量
    size_t GetWidgetCount() const { return m_windows.size(); }

private:
    WidgetManager();
    ~WidgetManager();

    WidgetManager(const WidgetManager&) = delete;
    WidgetManager& operator=(const WidgetManager&) = delete;

private:
    HINSTANCE m_hInstance = nullptr;
    std::unordered_map<std::string, WidgetCreator> m_creators;
    std::vector<std::unique_ptr<WidgetWindow>> m_windows;
};

} // namespace Windget
