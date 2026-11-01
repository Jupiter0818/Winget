# Windget 🪟

<p align="center">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599C?style=for-the-badge&logo=c%2B%2B" alt="C++17">
  <img src="https://img.shields.io/badge/Graphics-Direct2D%20%2F%20DirectWrite-0078D6?style=for-the-badge&logo=windows" alt="Direct2D">
  <img src="https://img.shields.io/badge/Platform-Windows%2010%20%2F%2011%20(64--bit)-0078D4?style=for-the-badge&logo=windows11" alt="Windows 10/11">
  <img src="https://img.shields.io/badge/Package-Standalone%20~1.3MB-success?style=for-the-badge" alt="Standalone">
  <img src="https://img.shields.io/badge/Idle%20CPU-0%25-brightgreen?style=for-the-badge" alt="Zero CPU">
  <img src="https://img.shields.io/badge/License-MIT-orange?style=for-the-badge" alt="MIT License">
</p>

<p align="center">
  <b>Windget</b> is an ultra-lightweight, native <b>Desktop Widget Engine</b> built exclusively for Windows 10 & 11 in modern C++17.<br>
  Delivering macOS-level desktop aesthetics — dark frosted translucent cards, soft diffuse drop shadows, and crisp DirectWrite typography — seamlessly integrated into your wallpaper with <b>near 0% idle CPU</b> and <b>minimal RAM usage</b>.
</p>

<p align="center">
  <a href="README.md">简体中文</a> | <b>English</b>
</p>

---

## 📸 Built-in Widgets Showcase

Windget comes equipped with 3 classic desktop widgets out of the box:

### 1. 🕒 Clock Widget
- **Minimalist Large Typography**: Rendered via `Segoe UI Variable Display` for crisp vector anti-aliasing.
- **Dynamic Weekday & Date**: Secondary contrast date text tailored to dark backgrounds.

### 2. 📅 Calendar Widget
- **Left Badge**: Prominent crimson uppercase month title, 48pt date number, and full weekday.
- **Smart Calendar Grid**: Automatically computes leap years and monthly start day offsets, with today's date highlighted in a vibrant circular badge.

### 3. ⚡ System Monitor Widget
- **Real-time Metrics**: Accurate CPU load and RAM usage (Used / Total GB) via zero-overhead Win32 kernel calls.
- **Dual Capsule Gauges**: macOS electric blue (CPU) and elegant purple (RAM) progress bars.
- **Negligible Overhead**: Uses `GetSystemTimes` and `GlobalMemoryStatusEx`, consuming virtually 0% CPU.

---

## 🌟 Key Engineering Features

- **True Desktop Layer Pinning**: Anchored to Windows `Progman` owner at `HWND_BOTTOM`. Normal windows naturally cover the widgets; pressing `Win + D` keeps widgets steadfast on your desktop wallpaper. Auto-recovers on Explorer restarts via `TaskbarCreated`.
- **Non-Activating Interactions**: Implements `WM_MOUSEACTIVATE -> MA_NOACTIVATE`. Clicking or dragging widgets never steals input focus from active applications (e.g. typing in IDE or chat apps).
- **0% Drag CPU & Hardware Acceleration**: 32-bit premultiplied ARGB with Direct2D offscreen rendering + `UpdateLayeredWindow`. Dragging moves the DWM hardware surface with 0 repaints and 0 CPU spikes.
- **Multi-Monitor & High DPI Aware**: Seamless dynamic scaling across monitors with different DPIs (100%, 125%, 150%, 200%). Supports negative coordinates on multi-screen setups, and automatically heals position if a display is disconnected.
- **System Tray & Context Menus**: Custom dark clock tray icon with a double-click boss-key toggle (show/hide all widgets), auto-start on boot toggle, and widget management.
- **Config Persistence**: Drag-and-save position persistence via an in-house zero-dependency C++17 JSON engine, saved in `%APPDATA%\Windget\config.json`.
- **Standalone Portable Executable**: Fully statically linked into a single `1.32 MB` executable with zero DLL dependencies.

---

## 🚀 Quick Start

### Option 1: Download & Run (Recommended)
Download the latest pre-built standalone package from [Releases](../../releases):
1. Download **`Windget-windows-x64.zip`** and extract it.
2. Double-click **`Windget.exe`** to run immediately!

### Option 2: Build From Source
```powershell
# 1. Clone repository
git clone https://github.com/Jupiter0818/Winget.git
cd Winget

# 2. Configure build
cmake -S . -B build -G "MinGW Makefiles"

# 3. Build standalone Release executable
cmake --build build --config Release
```
The standalone single executable will be located at **`dist/Windget.exe`**.

---

## 🧩 Developing Your Own Widget

Windget uses an interface-based architecture. Simply implement `IWidget`:

```cpp
#include "widgets/IWidget.h"

class MyWidget : public Windget::IWidget
{
public:
    std::string GetId() const override { return "my_widget_1"; }
    std::string GetType() const override { return "my_widget"; }
    float GetWidth() const override { return 320.0f; }
    float GetHeight() const override { return 170.0f; }
    uint32_t GetUpdateIntervalMs() const override { return 1000; }

    void Initialize(IDWriteFactory* dwriteFactory) override {}
    void Update() override {}
    void Render(ID2D1DCRenderTarget* target, IDWriteFactory* dwriteFactory, const D2D1_RECT_F& cardBounds) override {}
    void Shutdown() override {}
};
```

Register it in `WidgetManager::Initialize`:
```cpp
RegisterWidgetType("my_widget", [](const std::string& id) {
    return std::make_shared<MyWidget>();
});
```

---

## 📄 License

Licensed under the **[MIT License](LICENSE)**. Contributions and Stars 🌟 are welcome!
