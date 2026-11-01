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
  <b>Windget</b> is an ultra-lightweight, native <b>Desktop Widget Engine</b> built exclusively for Windows 10 & 11 in modern C++17.<br>
  Merging the graceful interaction of macOS Widgets with the modern Fluent frosted glass aesthetics of Windows 11 — translucent dark/light acrylic glass, magnetic desktop icon grid snapping, real-time Windows dark/light theme synchronization, and vector typography seamlessly integrated into your desktop wallpaper with <b>near 0% idle CPU</b> and <b>minimal RAM usage</b>.
</p>

<p align="center">
  <a href="README.md">简体中文</a> | <b>English</b>
</p>

---

## 📸 Built-in Widgets Showcase

Windget comes equipped with 4 elegant, practical desktop widgets out of the box, supporting multi-instance concurrent placement:

### 1. 🕒 Clock Widget
```text
╭────────────────────────────────────────╮
│                                        │
│                 22:38                  │
│                                        │
│          Saturday, October 31          │
│                                        │
╰────────────────────────────────────────╯
```
- **Minimalist Large Typography**: Rendered with Microsoft's modern `Segoe UI Variable Display` engine for vector anti-aliasing.
- **Dynamic Weekday & Date**: Adapts to light/dark themes, displaying crisp high-contrast white on dark backgrounds and deep charcoal on light backgrounds.

---

### 2. 📅 Calendar Widget
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
- **Left Badge**: Prominent crimson uppercase month title, 48pt date number, and full weekday.
- **Smart Calendar Grid**: Automatically computes leap years and monthly start day offsets, with today's date highlighted in a vibrant circular badge and weekends smartly tinted.

---

### 3. ⚡ System Monitor Widget
```text
╭────────────────────────────────────────╮
│ ● SYSTEM PERFORMANCE                   │
│ CPU                                18% │
│ [█████████───────────────────────────] │
│ Memory             6.8 / 16.0 GB (42%) │
│ [██████████████████──────────────────] │
╰────────────────────────────────────────╯
```
- **Real-time Metrics**: Accurate CPU load and RAM usage (Used / Total GB & %) via zero-overhead Win32 kernel calls.
- **Dual Capsule Gauges**: Glowing status dot + macOS electric blue (CPU) and elegant purple (RAM) progress bars.
- **Negligible Overhead**: Uses `GetSystemTimes` and `GlobalMemoryStatusEx`, consuming virtually 0% CPU.

---

### 4. 🎵 Music Controller Widget
```text
╭────────────────────────────────────────╮
│    ┌─────┐   APPLE MUSIC          (●)  │
│    │     │   The Fate of Ophelia       │
│    │ ALBUM│  Taylor Swift              │
│    └─────┘   [|◀]   ( ▶ )   [▶|]       │
╰────────────────────────────────────────╯
```
- **Pure 96×96 High-Res Album Artwork**: Upgraded from vinyl to a prominent, clean square album card with rounded corners and a 1px specular reflection bevel.
- **Zero Disk Lock Streaming**: WIC image decoding is loaded entirely from memory streams, eliminating disk handle locks and allowing seamless track/cover rotation.
- **Universal Media Detection**: Native support for Apple Music, Spotify, NetEase, QQ Music, Foobar2000, and browsers (Chrome, Edge, Firefox).
- **Tactile Feedback & Hand Cursor**: Interactive buttons feature hover halo glow, micro-scale press compression, and `IDC_HAND` pointer cursor.
- **Dual Appearance Modes**:
  - **⬛ Classic Frosted Mode**: Translucent dark obsidian or pure milky white frosted glass following the system theme.
  - **🌈 Album Adaptive Mode**: Apple Music mini-player inspired ambient gradient extracted dynamically from the current album cover with high-contrast typography. Quick-toggle via the top-right capsule button!

---

## 🌟 Key Engineering Features

### 1. 🌓 Real-Time Windows Dark / Light Theme Sync
- **Registry & Event Driven**: Listens to Windows `WM_SETTINGCHANGE` broadcasts and `AppsUseLightTheme` registry key. Switches seamlessly across all widgets without restarting.
- **Flexible Theme Options**: Available via right-click and system tray menus:
  - `🌓 System Default` (Follows Windows)
  - `⬛ Dark Frosted Glass`
  - `⬜ Light Frosted Glass`

### 2. 🪟 Frosted Glass & Shadow-Free Design
- **Zero Outer Shadows**: Eliminates blurry outer shadow halos; window borders match visual card edges 1:1 for crisp, precise alignment.
- **Specular Glass Edge Reflection**:
  - **Light Frosted**: Milky translucent acrylic gradient (`rgba(255,255,255,0.84)` ~ `rgba(238,242,250,0.76)`) with a top specular bevel reflection and refraction highlight.
  - **Dark Frosted**: Smoked obsidian gradient (`rgba(30,33,43,0.78)` ~ `rgba(18,20,26,0.88)`).

### 3. 🧲 Desktop Icon Grid & Magnetic Snapping
- **Desktop Icon Grid Alignment**: Reads system `SM_CXICONSPACING` and `SM_CYICONSPACING` metrics to snap widgets neatly along desktop icon rows and columns.
- **Inter-Widget Magnetic Snapping**: Moving a widget close to another automatically snaps adjacent edges or centers.
- **Screen Work Area Edge Snapping**: Keeps comfortable breathing margins around display boundaries.
- **Free Placement & Position Lock**:
  - Hold `Alt` while dragging to bypass snapping for arbitrary pixel placement.
  - Enable `🔒 Lock Position` to prevent accidental drag during normal interactions.

### 4. 📌 True Desktop Layer Pinning
- **Never Obstructs Active Windows**: Owned by Windows Shell (`Progman`), strictly pinned to `HWND_BOTTOM`.
- **Natural Window Stacking**: Normal applications (browser, IDE, chat) naturally overlay the widgets.
- **`Win + D` Wallpaper Stay**: Pressing `Win + D` (Show Desktop) minimizes normal apps while leaving widgets intact and visible on your wallpaper.
- **Explorer Crash Self-Healing**: Automatically re-anchors to the desktop and restores tray icons upon `TaskbarCreated` broadcast.

### 5. 🎯 Non-Activating Focus-Free Interactions
- Implements `WM_MOUSEACTIVATE -> MA_NOACTIVATE`. Clicking, dragging widgets, or toggling playback controls never steals typing focus from active foreground windows.

### 6. ⚡ 0% Drag CPU & Hardware Acceleration
- **True 32-bit Premultiplied ARGB**: Direct2D offscreen bitmap rendering with `UpdateLayeredWindow` per-pixel alpha blending.
- **0% Drag CPU**: The Windows Desktop Window Manager (DWM) moves the hardware surface directly — 0 repaints, 0 jitter, 0 CPU spike during dragging.
- **Standalone Green Executable**: Rejects heavyweight runtimes (Electron, Chromium, Qt, .NET). The complete standalone executable is only **~1.4MB** with negligible RAM footprint.

### 7. 🖥️ Multi-Monitor & Per-Monitor V2 High DPI
- **Dynamic Cross-Screen Scaling**: Crisp vector anti-aliased rendering across mixed-DPI displays (100%, 125%, 150%, 200%).
- **Negative Coordinates Support**: Seamlessly accommodates secondary displays located to the left or above the primary monitor.
- **Display Disconnect Recovery**: Intelligently migrates off-screen widgets back to active display boundaries when an external monitor is unplugged.

### 8. 🔔 System Tray & Context Menus
- **Persistent Notification Area Icon**: Sleek modern dial icon.
- **Boss Key**: Double-click the tray icon to instantly toggle visibility of all widgets.
- **Comprehensive Context Menu**:
  - `🎨 Theme` (System Default / Dark Frosted / Light Frosted)
  - `➕ Add Widget` (Clock, Calendar, System Monitor, Music Controller)
  - `🧲 Snap to Grid` / `🔒 Lock Position`
  - `🔄 Refresh All` / `👁️ Show/Hide All` / `💻 Reset Positions`
  - `🚀 Launch on Startup` (Syncs Windows Run registry key)
  - `📂 Open Config Directory` (`%APPDATA%\Windget`)
  - `❌ Exit Windget`

### 9. 💾 Zero-Dependency Config Persistence
- Built-in lightweight C++17 JSON engine; widget positions are saved upon mouse release to `%APPDATA%\Windget\config.json`.
- Restores all widget instances and coordinates automatically on startup, with full portable mode support.

---

## 🚀 Quick Start

### Option 1: Download & Run (Recommended)
Download the latest standalone release from [Releases](../../releases):
1. Download **`Windget-v1.0.0-windows-x64.zip`** and extract it.
2. Double-click **`Windget.exe`** to run immediately!
*(Zero installation required, leaves no orphaned registry entries, completely green & portable)*

### Option 2: Build From Source

#### Requirements
- Windows 10 / 11 (64-bit)
- CMake 3.20 or newer
- MinGW-w64 (GCC 10+) or Visual Studio (MSVC 2019+)

#### Build Commands
```powershell
# 1. Clone repository
git clone https://github.com/Jupiter0818/Winget.git
cd Winget

# 2. Configure build files
cmake -S . -B build -G "MinGW Makefiles"

# 3. Compile static Release binary
cmake --build build --config Release
```
Upon completion, the fully self-contained binary will be generated at **`dist/Windget.exe`**.

---

## 📁 Project Architecture

```text
Windget
│
├── CMakeLists.txt               # CMake static build configuration (-static -O3 -s)
├── .gitignore                   # Git ignore rules
├── LICENSE                      # MIT License
├── README.md                    # Chinese documentation
├── README_EN.md                 # English documentation
│
├── .github/workflows            # GitHub Actions CI/CD
│   └── build.yml                # Automatic build & package releases
│
├── dist                         # Standalone executable output
│   └── Windget.exe              # Single executable binary (~1.4MB)
│
└── src
    ├── main.cpp                 # Main entry, DPI initialization & message loop
    │
    ├── core                     # Core architecture
    │   ├── DesktopManager       # Desktop layer pinning & Explorer crash recovery
    │   ├── WidgetWindow         # Win32 transparent layered window, grid snapping
    │   ├── WidgetManager        # Widget factory & lifecycle management
    │   ├── ThemeManager         # System theme detection & dynamic theme switching
    │   ├── MediaManager         # Global media detection & control bridge
    │   ├── ConfigManager        # Configuration loading, saving & portable support
    │   ├── MonitorManager       # Multi-monitor detection & Per-Monitor V2 DPI
    │   ├── TrayManager          # System tray icon & context menus
    │   └── AutoStartManager     # Startup registry management
    │
    ├── graphics                 # Rendering engine
    │   └── D2DRenderer          # Direct2D/DirectWrite pipeline & frosted glass
    │
    ├── widgets                  # Widget implementations
    │   ├── IWidget.h            # Widget abstract base interface
    │   ├── ClockWidget          # Clock widget
    │   ├── CalendarWidget       # Calendar widget
    │   ├── SystemMonitorWidget  # Hardware performance monitor
    │   └── MusicWidget          # Music controller widget (Apple Music/adaptive color)
    │
    └── utils                    # Utilities
        └── JsonUtils            # Zero-dependency C++17 JSON parser/serializer
```

---

## 🧩 Building Your Own Widget

Windget's decoupled architecture makes creating custom widgets straightforward. Simply inherit from `IWidget` without dealing with complex Win32 window management:

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
        // Create DirectWrite text formats and resources
    }

    void Update() override {
        // Update business data (network requests, system queries, etc.)
    }

    void Render(ID2D1DCRenderTarget* target, IDWriteFactory* dwriteFactory, const D2D1_RECT_F& cardBounds) override {
        // Draw widget content using Direct2D (Frosted glass background is handled by the engine)
    }

    void Shutdown() override {
        // Release resources
    }
};
```

Register your widget in `WidgetManager::Initialize`:
```cpp
RegisterWidgetType("my_widget", [](const std::string& id) {
    return std::make_shared<MyCustomWidget>(id);
});
```

---

## 🗺️ Roadmap

- [x] **Phase 1: Clock Widget Prototype** (Layered window, drag-to-move, timer refresh, non-activating)
- [x] **Phase 2: Direct2D Modern Rendering Pipeline** (32-bit Premultiplied Alpha, DirectWrite typography)
- [x] **Phase 3: Desktop Layer Pinning** (Progman anchor, `HWND_BOTTOM`, `Win + D` desktop stay)
- [x] **Phase 4 & 5: OOP Architecture Refactoring** (`IWidget` interface, `WidgetWindow`, `WidgetManager`)
- [x] **Phase 6: Config Persistence** (Zero-dependency JSON, `%APPDATA%`, portable mode, auto-save)
- [x] **Phase 7: Multi-Monitor & Per-Monitor V2 DPI Scaling** (Negative coordinate support, display self-healing)
- [x] **Phase 8: System Tray & Context Menu** (Shell_NotifyIcon, double-click boss key, auto-start toggle)
- [x] **Phase 9: Built-in Widget Suite**:
  - [x] 🕒 Clock Widget (`ClockWidget`, large vector time + date)
  - [x] 📅 Calendar Widget (`CalendarWidget`, macOS style monthly grid + today badge)
  - [x] ⚡ System Monitor Widget (`SystemMonitorWidget`, CPU/RAM dual gauges)
- [x] **Phase 10: Standalone Green Distribution & CI/CD** (Statically linked ~1.4MB binary, GitHub Actions)
- [x] **Phase 11: Music Controller Widget & Media Controls**:
  - [x] 🎵 Music Controller (`MusicWidget`, 96×96 clean artwork, zero disk lock, cross-platform media)
  - [x] 🖱️ Interactive Controls (Prev/Play/Next desktop controls, halo glow, tactile click feedback)
  - [x] 🌈 Dual Display Modes (Classic frosted glass vs. album color adaptive with contrast hierarchy)
- [x] **Phase 12: Desktop Icon Grid Snapping**:
  - [x] 🧲 Desktop icon grid alignment (`SM_CXICONSPACING` / `SM_CYICONSPACING`)
  - [x] 🧲 Inter-widget magnetic snapping & screen edge alignment
  - [x] ⌨️ Hold `Alt` to bypass snapping, position locking option
- [x] **Phase 13: Windows Theme Sync & Frosted Glass**:
  - [x] 🌓 Real-time Windows light/dark theme tracking (`AppsUseLightTheme` + `WM_SETTINGCHANGE`)
  - [x] 🪟 Windows 11 Fluent light/dark frosted glass with 1px specular bevel reflection
  - [x] 🚫 Zero outer shadow halos for 1:1 card-to-window snapping
- [ ] **Phase 14: Future Widgets**:
  - [ ] ⛅ Live Weather Widget (Global weather data & animated icons)
  - [ ] 📝 Quick Notes & Todo List

---

## 📄 License

This project is licensed under the **[MIT License](LICENSE)**. Star 🌟 and Fork contributions are always welcome!
