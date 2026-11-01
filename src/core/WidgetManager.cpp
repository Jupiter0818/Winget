#include "core/WidgetManager.h"
#include "core/ConfigManager.h"
#include "core/MonitorManager.h"
#include "widgets/ClockWidget.h"
#include "widgets/CalendarWidget.h"
#include "widgets/SystemMonitorWidget.h"
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

    // 1. 注册内置 Widget 工厂
    RegisterWidgetType("clock", [](const std::string& id) -> std::shared_ptr<IWidget> {
        return std::make_shared<ClockWidget>(id.empty() ? "clock_default" : id);
    });

    RegisterWidgetType("calendar", [](const std::string& id) -> std::shared_ptr<IWidget> {
        return std::make_shared<CalendarWidget>(id.empty() ? "calendar_default" : id);
    });

    RegisterWidgetType("system", [](const std::string& id) -> std::shared_ptr<IWidget> {
        return std::make_shared<SystemMonitorWidget>(id.empty() ? "system_default" : id);
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

void WidgetManager::RefreshAll()
{
    for (auto& w : m_windows)
    {
        if (w)
        {
            if (w->GetWidget())
            {
                w->GetWidget()->Update();
            }
            w->RequestRender();
        }
    }
}

void WidgetManager::ToggleVisibility()
{
    if (m_windows.empty())
    {
        return;
    }

    bool anyVisible = false;
    for (const auto& w : m_windows)
    {
        if (w && w->IsVisible())
        {
            anyVisible = true;
            break;
        }
    }

    for (auto& w : m_windows)
    {
        if (w)
        {
            if (anyVisible)
            {
                w->Hide();
            }
            else
            {
                w->Show();
                w->RequestRender();
            }
        }
    }
}

void WidgetManager::ResetPositions()
{
    auto primary = MonitorManager::GetInstance().GetPrimaryMonitor();
    int startX = primary.workArea.right - 360;
    int startY = primary.workArea.top + 60;

    for (size_t i = 0; i < m_windows.size(); ++i)
    {
        auto& window = m_windows[i];
        if (window && window->GetHwnd())
        {
            int x = startX;
            int y = startY + static_cast<int>(i) * 200;

            if (y + 180 > primary.workArea.bottom)
            {
                x -= 340;
                y = primary.workArea.top + 60;
            }

            SetWindowPos(
                window->GetHwnd(),
                nullptr,
                x,
                y,
                0,
                0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
            );

            window->SetPosition(x, y);

            if (window->GetWidget())
            {
                ConfigManager::GetInstance().UpdateWidgetPosition(
                    window->GetWidget()->GetId(),
                    x,
                    y
                );
            }
            window->RequestRender();
        }
    }

    ConfigManager::GetInstance().Save();
}

WidgetWindow* WidgetManager::AddWidgetAtDefaultPosition(const std::string& type)
{
    auto primary = MonitorManager::GetInstance().GetPrimaryMonitor();
    int defaultX = primary.workArea.right - 360;
    int defaultY = primary.workArea.top + 60 + static_cast<int>(m_windows.size()) * 200;

    if (defaultY + 180 > primary.workArea.bottom)
    {
        defaultX -= 340;
        defaultY = primary.workArea.top + 60;
    }

    return CreateWidget(type, defaultX, defaultY);
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
