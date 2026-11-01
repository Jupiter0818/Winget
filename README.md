# Windget 🪟

<p align="center">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599C?style=for-the-badge&logo=c%2B%2B" alt="C++17">
  <img src="https://img.shields.io/badge/Graphics-Direct2D%20%2F%20DirectWrite-0078D6?style=for-the-badge&logo=windows" alt="Direct2D">
  <img src="https://img.shields.io/badge/Platform-Windows%2010%20%2F%2011%20(64--bit)-0078D4?style=for-the-badge&logo=windows11" alt="Windows 10/11">
  <img src="https://img.shields.io/badge/Package-Standalone%20~1.4MB-success?style=for-the-badge" alt="Standalone">
  <img src="https://img.shields.io/badge/Idle%20CPU-0%25-brightgreen?style=for-the-badge" alt="Zero CPU">
  <img src="https://img.shields.io/badge/License-MIT-orange?style=for-the-badge" alt="MIT License">
</p>

<p align="center">
  <b>Windget</b> 是一个专为 Windows 10 / 11 打造的<b>超轻量、纯原生桌面小组件平台（Desktop Widget Engine）</b>。<br>
  融合 macOS Widget 的优雅交互与 Windows 11 Fluent 毛玻璃美学 —— 剔透无暇的深浅毛玻璃质感、对齐桌面图标网格的磁力吸附、实时跟随系统主题切换、矢量级排版，与桌面壁纸深度融合，拥有近乎 <b>0% 闲置 CPU</b> 和 <b>极小内存开销</b>。
</p>

<p align="center">
  <b>简体中文</b> | <a href="README_EN.md">English</a>
</p>

---

## 📸 内置小组件展示

Windget 内置了 4 款兼具实用性与高颜值的桌面小组件，开箱即用，支持多实例并发排布：

### 1. 🕒 时钟小组件 (Clock Widget)
```text
╭────────────────────────────────────────╮
│                                        │
│                 22:38                  │
│                                        │
│          Saturday, October 31          │
│                                        │
╰────────────────────────────────────────╯
```
- **极简大字号时间**：基于微软现代字体引擎 `Segoe UI Variable Display` 矢量级抗锯齿排版，清晰纯净。
- **动态星期与日期**：深浅主题自适应，深色背景为柔和高亮白，浅色背景为低调沉稳黑。

---

### 2. 📅 精致月历小组件 (Calendar Widget)
```text
╭────────────────────────────────────────╮
│  NOVEMBER │   S   M   T   W   T   F   S    │
│           │               1   2   3   4    │
│    01     │   5   6   7  (8)  9  10  11    │
│           │  12  13  14  15  16  17  18    │
│   SUNDAY  │  19  20  21  22  23  24  25    │
│           │  26  27  28  29  30  31        │
│           │  30  31                        │
╰────────────────────────────────────────╯
```
- **左侧核心徽章**：macOS 经典树莓红大写月份标题、48pt 优雅大号日期数字与星期全称。
- **右侧智能月历网格**：自动计算公历闰年与当月首日星期偏移，当天日期以高亮圆形底标醒目呈现，周末智能标红。

---

### 3. ⚡ 硬件性能监控组件 (System Monitor Widget)
```text
╭────────────────────────────────────────╮
│ ● SYSTEM PERFORMANCE                   │
│ CPU                                18% │
│ [█████████───────────────────────────] │
│ Memory             6.8 / 16.0 GB (42%) │
│ [██████████████████──────────────────] │
╰────────────────────────────────────────╯
```
- **实时监控指标**：通过 Win32 毫秒级差分算法统计 CPU 实际使用率，精准监测物理内存已用/总量与占比。
- **极客双色胶囊进度条**：科技微光绿运行状态点 + 经典科技蓝（CPU）与优雅高贵紫（Memory）平滑进度条。
- **极致无感开销**：基于 Win32 `GetSystemTimes` 与 `GlobalMemoryStatusEx`，1 秒/次轻量轮询，CPU 损耗趋近于 0。

---

### 4. 🎵 音乐媒体控制组件 (Music Widget)
```text
╭────────────────────────────────────────╮
│    ┌─────┐   APPLE MUSIC          (●)  │
│    │     │   The Fate of Ophelia       │
│    │ ALBUM│  Taylor Swift              │
│    └─────┘   [|◀]   ( ▶ )   [▶|]       │
╰────────────────────────────────────────╯
```
- **96×96 纯净高清专辑大封面**：告别繁琐黑胶，采用平滑圆角正方形高清大封面，自带 1px 精致折射微边框。
- **零锁流式图片解码（Zero Disk Lock）**：WIC 图像解码采用完全从内存流加载机制，杜绝任何磁盘句柄锁定，切歌与封面换图丝滑顺畅。
- **全生态媒体感知**：原生无缝支持 Apple Music、Spotify、网易云音乐、QQ 音乐、Foobar2000、Chrome/Edge/Firefox 等任意播放源。
- **灵动触感反馈与手型光标**：控件配备悬浮外发光光晕（Halo Glow）、按压物理微缩微动反馈与指针手型光标（`IDC_HAND`）。
- **双外观显示模式**：
  - **⬛ 经典毛玻璃模式**：微透深色黑曜石或纯净浅色牛奶白磨砂玻璃底色，严格跟随系统主题。
  - **🌈 封面自适应取色模式**：对齐 Apple Music mini-player 沉浸取色逻辑，根据当前专辑核心色相提取通透深色调氛围渐变底色，搭配高对比度纯白主歌名与副标题，右上角胶囊一键极速切换！

---

## 🌟 核心特性与工程设计

### 1. 🌓 实时跟随 Windows 深浅色外观主题
- **原生注册表与事件监听**：程序通过 `ThemeManager` 实时监听系统的 `WM_SETTINGCHANGE` 广播与 `AppsUseLightTheme` 注册表项，深浅色切换毫秒级响应，**全量组件无需重启、无缝变色**。
- **自由外观选项**：在任何小组件右键或系统托盘菜单中均支持自由指定：
  - `🌓 跟随系统 (System Default)`（默认自适应）
  - `⬛ 深色毛玻璃 (Dark Frosted)`
  - `⬜ 浅色毛玻璃 (Light Frosted)`

### 2. 🪟 剔透无暇的毛玻璃（Frosted Glass）与无阴影设计
- **真正去外阴影（Shadow-Free）**：彻底清除外围模糊阴影晕染，窗口尺寸与卡片视觉边缘严格 1:1 贴合，吸附与摆放利落干脆。
- **精细物理镜面光边（Specular Glass Edge）**：
  - **浅色毛玻璃**：纯净通透的牛奶白微透渐变（`rgba(255,255,255,0.84)` ~ `rgba(238,242,250,0.76)`），搭配顶部 1px 镜面折射光边与高光线，契合 Windows 11 Fluent 现代光感。
  - **深色毛玻璃**：剔透黑曜石微透渐变（`rgba(30,33,43,0.78)` ~ `rgba(18,20,26,0.88)`），通透深邃。

### 3. 🧲 像桌面图标一样支持网格磁吸（Grid Snapping）
- **对齐 Windows 桌面图标网格**：读取系统 `SM_CXICONSPACING` 与 `SM_CYICONSPACING` 图标间距标准，组件可精准对齐桌面图标列与行，排布规整严谨。
- **组件间智能互吸**：当拖拽一个小组件靠近另一个小组件时，自动在顶边、底边、左边、右边或并排间距处产生磁力吸附，像乐高积木一样自动紧凑嵌合。
- **屏幕工作区边缘磁吸**：拖拽至屏幕四周边缘时自动保留标准呼吸边距并平滑吸附。
- **自由拖拽与位置锁定**：
  - 拖拽时按住 `Alt` 键可临时解除吸附以任意像素自由摆放；
  - 菜单提供 `🔒 锁定小组件位置 (Lock Position)` 选项，防止日常操作发生误拖拽。

### 4. 📌 深度融入 Windows 桌面（Desktop Layer Pinning）
- **绝不遮挡前台工作**：底层绑定 Windows 外壳管理器（`Progman`）所有者，窗口层级严格锚定在 `HWND_BOTTOM`。
- **普通窗口自然覆盖**：打开浏览器、IDE、微信等任何应用程序，窗口将自然盖过小组件。
- **`Win + D` 桌面驻留**：按下快捷键 `Win + D`（显示桌面）时，普通应用被最小化，小组件像壁纸一部分一样稳固保留在桌面上，绝不闪烁、绝不被最小化。
- **Explorer 重启自愈**：监听 `TaskbarCreated` 系统广播，当资源管理器重启或崩溃恢复后，自动重新绑定桌面层级并恢复托盘图标。

### 5. 🎯 不抢占前台输入焦点（Non-Activating Interaction）
- 拦截并处理 `WM_MOUSEACTIVATE` 返回 `MA_NOACTIVATE`。
- 用户在桌面上任意点击、拖拽小组件或操作音乐播放器时，当前正在打字的前台文本光标（如代码编辑器、聊天软件）不受任何干扰。

### 6. ⚡ 0% 拖拽 CPU 与纯硬件加速（Direct2D Pipeline）
- **真正的 32-bit Premultiplied ARGB**：Direct2D 离屏位图渲染 + `UpdateLayeredWindow` 逐像素 Alpha 混合。
- **0% 拖动 CPU**：拖动时由 Windows DWM（桌面窗口管理器）直接移动表面缓存，**移动过程中 0 重绘、0 抖动、0 CPU 飙升**。
- **超轻量独立绿色单文件**：彻底杜绝 Chromium、Electron、WebView2、Qt 或 .NET 等重型引擎，单文件仅 **1.4MB**，内存占用极低。

### 7. 🖥️ 多显示器与 Per-Monitor V2 高 DPI 适配
- **支持跨屏多分辨率**：副屏 100%、主屏 150%、高分屏 200% 动态无缝缩放，文字矢量渲染永不发虚。
- **负坐标虚拟屏幕支持**：完整支持多显示器位于主屏左侧或上方时的负坐标定位。
- **屏幕断开自愈保护**：拔除外接副屏时，智能检测并自动将脱离视野的小组件迁回当前活动屏幕工作区。

### 8. 🔔 系统托盘常驻与快捷控制
- **任务栏右下角常驻托盘**：专属科技微光表盘图标，常驻通知区域。
- **双击老板键**：双击托盘图标一键快速隐藏/显示桌面全部小组件。
- **右键菜单功能全开**：
  - `🎨 外观主题`（跟随系统 / 深色毛玻璃 / 浅色毛玻璃）
  - `➕ 添加小组件`（随时增添时钟、日历、硬件监控、音乐控制器）
  - `🧲 对齐到桌面网格` / `🔒 锁定小组件位置`
  - `🔄 刷新全部组件` / `👁️ 显示 / 隐藏全部` / `💻 恢复默认位置`
  - `🚀 开机自启动`（自动同步注册表 `Run` 项）
  - `📂 打开配置文件目录`（直接在资源管理器中定位 `%APPDATA%\Windget`）
  - `❌ 退出 Windget`

### 9. 💾 零依赖配置持久化（Config Persistence）
- 自研零依赖微型 C++17 JSON 引擎，鼠标拖拽松开即刻持久化写入 `%APPDATA%\Windget\config.json`。
- 启动时自动恢复已放置的所有小组件实例、各自类型与专属坐标，兼容同级目录便携模式（Portable Mode）。

---

## 🚀 快速开始

### 方式一：下载即用（推荐）
从 [Releases](../../releases) 页面下载最新预编译的绿色发行包：
1. 下载 **`Windget-v1.0.0-windows-x64.zip`** 并解压。
2. 双击运行 **`Windget.exe`** 即可！
*(无任何安装步骤，不写多余垃圾注册表项，纯绿色单文件)*

### 方式二：从源码编译构建

#### 环境要求
- Windows 10 / 11 (64-bit)
- CMake 3.20 或更高版本
- MinGW-w64 (GCC 10+) 或 Visual Studio (MSVC 2019+)

#### 编译命令
```powershell
# 1. 克隆代码仓库
git clone https://github.com/Jupiter0818/Winget.git
cd Winget

# 2. 生成构建文件
cmake -S . -B build -G "MinGW Makefiles"

# 3. 编译静态单文件 Release 版本
cmake --build build --config Release
```
编译成功后，完全独立的单文件程序将输出在 **`dist/Windget.exe`**。

---

## 📁 目录与架构概览

```text
Windget
│
├── CMakeLists.txt               # CMake 构建与静态链接配置 (-static -O3 -s)
├── .gitignore                   # Git 忽略配置
├── LICENSE                      # MIT 开源许可证
├── README.md                    # 中文说明文档
├── README_EN.md                 # 英文说明文档
│
├── .github/workflows            # GitHub Actions 自动化 CI/CD
│   └── build.yml                # 自动构建并打包 Release
│
├── dist                         # 绿色独立可执行程序输出目录
│   └── Windget.exe              # 单文件独立程序 (~1.4MB)
│
└── src
    ├── main.cpp                 # 引擎主入口、DPI 初始化与消息循环
    │
    ├── core                     # 核心架构层
    │   ├── DesktopManager       # Windows 桌面层级锚定与 Explorer 崩溃监听
    │   ├── WidgetWindow         # Win32 分层透明宿主窗口、网格吸附与消息路由
    │   ├── WidgetManager        # 组件工厂与多组件生命周期调度中心
    │   ├── ThemeManager         # 系统深浅主题实时感知与外观切换管理
    │   ├── MediaManager         # 全局媒体信息感知与交互通信支持
    │   ├── ConfigManager        # 配置加载、实时存储与自愈
    │   ├── MonitorManager       # 多显示器探测与 Per-Monitor V2 DPI 适配
    │   ├── TrayManager          # Windows 系统托盘常驻与右键菜单
    │   └── AutoStartManager     # 注册表开机自启管理
    │
    ├── graphics                 # 渲染引擎
    │   └── D2DRenderer          # Direct2D/DirectWrite 硬件加速与毛玻璃微光管线
    │
    ├── widgets                  # 小组件业务实现
    │   ├── IWidget.h            # 小组件统一抽象基类
    │   ├── ClockWidget          # 时钟小组件
    │   ├── CalendarWidget       # 月历小组件
    │   ├── SystemMonitorWidget  # 硬件监控小组件
    │   └── MusicWidget          # 音乐控制器小组件 (Apple Music/自适应取色)
    │
    └── utils                    # 辅助工具库
        └── JsonUtils            # 零依赖高性能 C++17 JSON 解析/序列化器
```

---

## 🧩 如何开发你自己的小组件？

Windget 采用了极致解耦的架构，添加一个全新小组件只需实现 `IWidget` 接口，无需处理任何繁杂的 Win32 窗口消息和分层 API：

```cpp
#include "widgets/IWidget.h"

class MyCustomWidget : public Windget::IWidget
{
public:
    std::string GetId() const override { return "my_widget_1"; }
    std::string GetType() const override { return "my_widget"; }
    float GetWidth() const override { return 320.0f; }
    float GetHeight() const override { return 170.0f; }
    uint32_t GetUpdateIntervalMs() const override { return 1000; }

    void Initialize(IDWriteFactory* dwriteFactory) override {
        // 创建 DirectWrite 文本格式等资源
    }

    void Update() override {
        // 更新业务数据 (如网络请求、数据计算)
    }

    void Render(ID2D1DCRenderTarget* target, IDWriteFactory* dwriteFactory, const D2D1_RECT_F& cardBounds) override {
        // 使用 Direct2D 原生绘制卡片内容 (背景由引擎自动统一渲染毛玻璃质感)
    }

    void Shutdown() override {
        // 释放图形资源
    }
};
```

随后在 `WidgetManager::Initialize` 中注册工厂即可：
```cpp
RegisterWidgetType("my_widget", [](const std::string& id) {
    return std::make_shared<MyCustomWidget>(id);
});
```

---

## 🗺️ 开发路线图 (Roadmap)

- [x] **Phase 1: Clock Widget 原型验证**（Win32 分层窗口、拖拽、定时刷新、不抢焦点）
- [x] **Phase 2: Direct2D 现代化渲染管线**（真 32 位 Alpha、磨砂圆角卡片、DirectWrite 字体排版）
- [x] **Phase 3: 桌面层级深度锚定**（Progman 所有者锚定、`HWND_BOTTOM` 约束、`Win + D` 驻留不被最小化）
- [x] **Phase 4 & 5: 面向对象架构重构**（`IWidget` 接口解耦、`WidgetWindow` 通用宿主、`WidgetManager` 多组件管理器）
- [x] **Phase 6: 配置持久化系统**（自研零依赖 JSON、`%APPDATA%` 存储、便携模式、拖动即存、重启自动恢复）
- [x] **Phase 7: 多显示器与 Per-Monitor V2 DPI 进阶适配**（负坐标多屏支持、跨屏动态 DPI 缩放、断开副屏越界自愈）
- [x] **Phase 8: 系统任务栏托盘常驻与右键菜单**（Shell_NotifyIcon、Explorer 崩溃重连、双击显隐、开机自启切换）
- [x] **Phase 9: 丰富小组件库**：
  - [x] 🕒 时钟组件（`ClockWidget`，大字号时间 + 星期日期）
  - [x] 📅 日历组件（`CalendarWidget`，macOS 风格月历网格与今日高亮）
  - [x] ⚡ 硬件监控组件（`SystemMonitorWidget`，CPU & 内存真实使用率与双进度条）
- [x] **Phase 10: 绿色单文件发行打包与 CI/CD**（静态链接消除 DLL 依赖，体积仅 ~1.4MB，集成 GitHub Actions）
- [x] **Phase 11: 音乐媒体控制组件与全生态交互系统**：
  - [x] 🎵 音乐控制器（`MusicWidget`，96×96 纯净大封面、内存流零锁解码、全生态媒体感知）
  - [x] 🖱️ 卡片交互控件系统（上一曲 / 播放暂停 / 下一曲桌面直控、悬浮光晕与触感缩放微动）
  - [x] 🌈 双外观显示模式（经典毛玻璃 VS 封面自适应取色，动态文本对比度与主次层次）
- [x] **Phase 12: 桌面图标网格对齐与磁性吸附**：
  - [x] 🧲 对齐 Windows 桌面图标网格（读取 `SM_CXICONSPACING` / `SM_CYICONSPACING`）
  - [x] 🧲 组件间相互吸附与屏幕边缘贴靠
  - [x] ⌨️ 按住 `Alt` 键临时自由脱离吸附，提供位置锁定选项
- [x] **Phase 13: Windows 系统深浅色外观同步与极致毛玻璃**：
  - [x] 🌓 实时监听系统深浅主题切换（`AppsUseLightTheme` + `WM_SETTINGCHANGE`）
  - [x] 🪟 Fluent 浅色毛玻璃与剔透深色黑曜毛玻璃设计，1px 镜面折射微边框
  - [x] 🚫 彻底去除外围模糊阴影，卡片贴合严丝合缝
- [ ] **Phase 14: 更多小组件规划**：
  - [ ] ⛅ 实时天气小组件（Weather Widget，全球气象与天气图标）
  - [ ] 📝 便签与待办打卡（Quick Notes / Todo List）

---

## 📄 开源许可证

本项目采用 **[MIT License](LICENSE)** 授权开源。欢迎 Star 🌟 与 Fork 贡献代码！
