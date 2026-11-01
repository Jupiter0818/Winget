# Windget - Windows 原生桌面小组件系统

<p align="center">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599C?style=flat-square&logo=c%2B%2B" alt="C++17">
  <img src="https://img.shields.io/badge/API-Win32%20%2F%20Direct2D-0078D6?style=flat-square&logo=windows" alt="Win32 / Direct2D">
  <img src="https://img.shields.io/badge/Platform-Windows%2010%20%2F%2011%20(64--bit)-blue?style=flat-square" alt="Windows 10/11">
  <img src="https://img.shields.io/badge/License-MIT-green?style=flat-square" alt="MIT License">
  <img src="https://img.shields.io/badge/Memory-~45MB-brightgreen?style=flat-square" alt="Low Memory">
  <img src="https://img.shields.io/badge/Idle%20CPU-0%25-brightgreen?style=flat-square" alt="Zero CPU">
</p>

**Windget** 是一个为 Windows 10/11 打造的**原生桌面小组件平台系统**。旨在提供类似 macOS Widget 的极致桌面体验——轻盈、优雅、与壁纸自然融为一体，且拥有极低系统资源占用。

最终目标不是做一个简单的“时钟程序”，而是一个**高扩展性、原生窗口行为的 Windows 桌面 Widget Engine**。

---

## 视觉预览

```text
╭────────────────────────────────────────╮
│                                        │
│                 22:38                  │
│                                        │
│          Saturday, October 31          │
│                                        │
╰────────────────────────────────────────╯
```

- **深色磨砂半透明质感**：22px 大圆角与 `rgba(28, 28, 33, 0.85)` 深色背景。
- **8 层柔和弥散阴影**：多层环境漫反射阴影，带有自然向下沉降感，完美融入各种壁纸。
- **现代化排版**：DirectWrite 灰度矢量抗锯齿渲染，大号 `Segoe UI Variable Display` 时间与次级对比度日期。

---

## 核心设计与特性

### 1. 真正的桌面层级融合（Desktop Layer）
- **绝不置顶**：通过绑定 Windows 桌面程序管理器（`Progman`）所有者与 `HWND_BOTTOM` 约束，小组件作为壁纸层的一部分存在。
- **普通窗口自然覆盖**：打开浏览器、VS Code 或记事本时，前台窗口自然覆盖小组件。
- **`Win + D` 驻留**：按下键盘快捷键 `Win + D`（显示桌面）时，普通应用被最小化，小组件稳固保留在桌面壁纸上，绝不会被隐藏或最小化。

### 2. 不抢占输入焦点（Non-activating）
- 处理 `WM_MOUSEACTIVATE` 并返回 `MA_NOACTIVATE`。
- 用户在桌面上任意拖拽或点击小组件时，当前正在打字的前台文本光标（如 IDE、聊天软件）不受任何干扰。

### 3. 极低资源占用（Zero Overhead）
- **0% 闲置 CPU**：没有无意义的高频渲染轮询与动画帧循环，时钟每秒仅精准重绘一次。
- **0% 拖拽 CPU**：使用 `WS_EX_LAYERED` 结合 DWM 硬件加速合成，拖拽时仅移动 DWM 表面，**拖动过程中 0 重绘、0 闪烁、0 CPU 飙升**。
- **几十 MB 运行内存**：彻底杜绝 Chromium、Electron、WebView、Qt 或 .NET 等重型运行时。

### 4. 统一小组件引擎平台（Widget Engine）
- **`IWidget` 接口规范**：抽离纯虚接口，小组件仅需实现数据更新与 Direct2D 内容绘制，无任何 Win32 样板代码负担。
- **`WidgetWindow` 宿主**：统一封装透明分层缓冲、DPI 动态缩放、桌面绑定、拖动管理与右键快捷交互。
- **`WidgetManager` 调度中心**：支持工厂注册与多组件并发运行。

### 5. 轻量配置持久化系统（ConfigManager）
- **纯原生 C++17 JSON 工具**：零第三方依赖，自研微型健壮 JSON 解析器与格式化器。
- **拖动松开即存**：鼠标拖拽松开后自动将最新 `(x, y)` 坐标写入 `%APPDATA%\Windget\config.json`。
- **下次启动恢复**：启动时自动恢复已放置的所有小组件与对应坐标。
- **便携模式与自愈机制**：支持可执行文件同级目录便携配置，配置文件损坏或不存在时自动使用合理默认值，杜绝崩溃。

---

## 配置文件结构

配置文件默认位于 `%APPDATA%\Windget\config.json`（亦可在同级目录放置以启用便携模式）：

```json
{
    "widgets": [
        {
            "id": "clock_default",
            "type": "clock",
            "x": 200,
            "y": 200,
            "width": 320,
            "height": 170,
            "theme": "dark",
            "opacity": 0.85,
            "fontSize": 56
        }
    ]
}
```

---

## 架构概览

```text
Windget
│
├── CMakeLists.txt
├── .gitignore
├── LICENSE
├── README.md
│
├── dist                         # 绿色独立发行目录
│   ├── Windget.exe              # 单文件独立可执行程序 (~1.3MB，全静态链接)
│   └── Windget-v1.0.0-windows-x64.zip
│
└── src
    ├── main.cpp                 # 引擎启动主入口
    │
    ├── core                     # 核心架构层
    │   ├── DesktopManager       # Windows 桌面层级锚定与 Explorer 自愈
    │   ├── WidgetWindow         # 通用原生桌面分层宿主窗口
    │   ├── WidgetManager        # 小组件全局生命周期与注册中心
    │   ├── ConfigManager        # 配置加载、实时存储与自愈
    │   ├── MonitorManager       # 多显示器跨屏与 Per-Monitor V2 DPI 适配
    │   ├── TrayManager          # Windows 系统托盘常驻与右键菜单调度
    │   └── AutoStartManager     # 注册表开机自启动管理
    │
    ├── graphics                 # 渲染管线
    │   └── D2DRenderer          # Direct2D/DirectWrite 硬件加速、阴影与分层输出
    │
    ├── widgets                  # 小组件业务实现
    │   ├── IWidget.h            # 小组件统一抽象基类
    │   ├── ClockWidget          # 独立时钟小组件
    │   ├── CalendarWidget       # macOS 风格精致月历小组件
    │   └── SystemMonitorWidget  # CPU/内存实时硬件监控小组件
    │
    └── utils                    # 辅助工具
        └── JsonUtils            # 零依赖轻量 C++17 JSON 解析与序列化器
```

---

## 编译与发行打包

### 开发环境要求
- **操作系统**：Windows 10 / Windows 11 (64-bit)
- **构建工具**：CMake 3.20 或更高版本
- **编译器**：支持 C++17 的编译器（MinGW-w64 GCC 10+ 或 MSVC 2019+）

### 一键编译生成绿色独立单文件程序

```powershell
# 1. 生成构建配置
cmake -S . -B build -G "MinGW Makefiles"

# 2. 编译 Release 发行版 (自动静态链接 MinGW 运行时并剥离调试符号)
cmake --build build --config Release
```

编译完成后，会在项目根目录下的 **`dist/`** 目录生成：
- **`dist/Windget.exe`**：完全独立、单文件绿色免安装可执行程序（无任何第三方 DLL 依赖，纯 Win32 原生依赖）。
- 支持直接双击启动，在任意 Windows 10/11 电脑上均可即开即用！

---

## 运行与交互

直接运行 `dist/Windget.exe`：

```powershell
Start-Process .\dist\Windget.exe
```

### 1. 桌面小组件交互
- **自由拖拽**：鼠标左键按住任意小组件卡片即可在桌面拖动，松开即刻自动持久化保存新坐标到 `%APPDATA%\Windget\config.json`。
- **右键组件菜单**：在桌面的任意小组件上点击鼠标右键：
  - `➕ 添加小组件`（随时增添时钟、日历或性能监控组件）
  - `🔄 刷新此组件`
  - `🗑️ 移除此组件`（自由删除桌面多余组件）
  - `💻 恢复默认位置`（一键重排至屏幕安全工作区）
  - `❌ 退出 Windget`

### 2. 系统任务栏托盘（右下角）
- **双击托盘图标**：一键快速显示 / 隐藏桌面上全部小组件（老板键）。
- **右键托盘图标**：
  - 增删小组件、刷新全部组件、恢复默认位置
  - `🚀 开机自启动`（自动同步 Windows 注册表 Run 键，勾选状态实时显示）
  - `📂 打开配置文件目录`（直接在资源管理器中定位并打开 `%APPDATA%\Windget`）
  - `🌐 项目 GitHub 主页`
  - `❌ 退出 Windget`

---

## 开发路线图（Roadmap）

- [x] **Phase 1: Clock Widget 原型验证**（基础 Win32 窗口、拖拽、定时刷新、不抢焦点）
- [x] **Phase 2: Direct2D / DirectWrite 现代化渲染系统**（真 32-bit Alpha、大圆角半透明卡片、柔和弥散阴影、矢量字体排版）
- [x] **Phase 3: 桌面层级深度集成**（Progman 所有者锚定、不压制普通窗口、`Win + D` 驻留不最小化、Explorer 重启自愈）
- [x] **Phase 4 & 5: 面向对象架构重构**（`IWidget` 抽象基类、`WidgetWindow` 宿主、`WidgetManager` 平台管理）
- [x] **Phase 6: 配置持久化系统**（自研轻量 JSON、`%APPDATA%` 存储、便携模式、拖动即存、启动恢复）
- [x] **Phase 7: 多显示器与 Per-Monitor V2 高 DPI 进阶适配**（负坐标多屏支持、跨屏动态 DPI 缩放、断开副屏越界自愈）
- [x] **Phase 8: 系统任务栏托盘常驻与右键菜单**（Shell_NotifyIcon、Explorer 崩溃重连、双击显隐、开机自启切换）
- [x] **Phase 9: 丰富小组件库**：
  - [x] 时钟组件（`ClockWidget`，大字号时间 + 星期日期）
  - [x] 日历组件（`CalendarWidget`，macOS 风格月历网格与今日高亮）
  - [x] 硬件监控组件（`SystemMonitorWidget`，CPU & 内存真实使用率与双进度条）
- [x] **Phase 10: 绿色单文件发行打包**（静态链接消除 DLL 依赖，体积仅 ~1.3MB）

---

## 许可证 (License)

本项目采用 [MIT License](LICENSE) 授权开源。
