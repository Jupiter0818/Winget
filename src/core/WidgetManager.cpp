#include "core/WidgetManager.h"
#include "core/ConfigManager.h"
#include "widgets/ClockWidget.h"
#include <algorithm>

namespace Windget {

WidgetManager& WidgetManager::GetInstance()
{
    static WidgetManager instance;
    return instance;
}

WidgetManager::WidgetManager() = default;

WidgetManager::~WidgetManager()
{
    Shutdown();
}

bool WidgetManager::Initialize(HINSTANCE hInstance)
{
    m_hInstance = hInstance;

    // 1. 注册内置 Clock Widget 工厂
    RegisterWidgetType("clock", [](const std::string& id) -> std::shared_ptr<IWidget> {
        return std::make_shared<ClockWidget>(id.empty() ? "clock_default" : id);
    });

    // 2. 从 %APPDATA%\Windget\config.json 加载配置
    ConfigManager::GetInstance().Load();
    const auto& config = ConfigManager::GetInstance().GetConfig();

    // 3. 根据配置文件自动恢复历史小组件与各自记录的桌面位置
    for (const auto& widgetCfg : config.widgets)
    {
        CreateWidget(widgetCfg.type, widgetCfg.x, widgetCfg.y, widgetCfg.id);
    }

    // 容错：若未恢复出任何组件，自动唤起默认 Clock
    if (m_windows.empty())
    {
        CreateWidget("clock", 200, 200, "clock_default");
    }

    return true;
}

void WidgetManager::RegisterWidgetType(const std::string& type, WidgetCreator creator)
{
    m_creators[type] = std::move(creator);
}

WidgetWindow* WidgetManager::CreateWidget(const std::string& type, int x, int y, const std::string& id)
{
    auto it = m_creators.find(type);
    if (it == m_creators.end() || !it->second)
    {
        return nullptr;
    }

    std::string widgetId = id.empty() ? (type + "_" + std::to_string(m_windows.size() + 1)) : id;
    std::shared_ptr<IWidget> widget = it->second(widgetId);
    if (!widget)
    {
        return nullptr;
    }

    auto window = std::make_unique<WidgetWindow>(widget);
    if (!window->Create(m_hInstance, x, y))
    {
        return nullptr;
    }

    // 确保该小组件在 ConfigManager 中存在对应记录
    auto& configs = ConfigManager::GetInstance().GetConfig().widgets;
    bool foundInConfig = false;
    for (const auto& c : configs)
    {
        if (c.id == widgetId)
        {
            foundInConfig = true;
            break;
        }
    }
    if (!foundInConfig)
    {
        WidgetConfig newCfg;
        newCfg.id = widgetId;
        newCfg.type = type;
        newCfg.x = x;
        newCfg.y = y;
        newCfg.width = static_cast<int>(widget->GetWidth());
        newCfg.height = static_cast<int>(widget->GetHeight());
        configs.push_back(newCfg);
        ConfigManager::GetInstance().Save();
    }

    WidgetWindow* pWindow = window.get();
    m_windows.push_back(std::move(window));
    return pWindow;
}

bool WidgetManager::DestroyWidget(WidgetWindow* pWindow)
{
    if (!pWindow)
    {
        return false;
    }

    auto it = std::find_if(
        m_windows.begin(),
        m_windows.end(),
        [pWindow](const std::unique_ptr<WidgetWindow>& ptr) {
            return ptr.get() == pWindow;
        }
    );

    if (it != m_windows.end())
    {
        std::string removedId = (*it)->GetWidget() ? (*it)->GetWidget()->GetId() : "";
        (*it)->Close();
        m_windows.erase(it);

        if (!removedId.empty())
        {
            auto& cfgs = ConfigManager::GetInstance().GetConfig().widgets;
            cfgs.erase(
                std::remove_if(cfgs.begin(), cfgs.end(), [&](const WidgetConfig& c) {
                    return c.id == removedId;
                }),
                cfgs.end()
            );
            ConfigManager::GetInstance().Save();
        }
        return true;
    }

    return false;
}

void WidgetManager::Shutdown()
{
    // 同步所有小组件当前最新坐标至配置系统
    for (const auto& window : m_windows)
    {
        if (window && window->GetWidget())
        {
            ConfigManager::GetInstance().UpdateWidgetPosition(
                window->GetWidget()->GetId(),
                window->GetX(),
                window->GetY()
            );
        }
    }

    // 持久化保存到磁盘
    ConfigManager::GetInstance().Save();

    for (auto& window : m_windows)
    {
        if (window)
        {
            window->Close();
        }
    }
    m_windows.clear();
}

} // namespace Windget
