#pragma once

#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <cstdint>

namespace Windget {

class IWidget
{
public:
    virtual ~IWidget() = default;

    // 获取小组件实例标识与类型
    virtual std::string GetId() const = 0;
    virtual std::string GetType() const = 0;

    // 逻辑设计尺寸 (DIP)
    virtual float GetWidth() const = 0;
    virtual float GetHeight() const = 0;

    // 定时刷新周期 (毫秒)
    virtual uint32_t GetUpdateIntervalMs() const = 0;

    // 生命周期回调
    virtual void Initialize(IDWriteFactory* dwriteFactory) = 0;
    virtual void Update() = 0;
    virtual void Render(
        ID2D1DCRenderTarget* target,
        IDWriteFactory* dwriteFactory,
        const D2D1_RECT_F& cardBounds
    ) = 0;
    virtual void Shutdown() = 0;

    // 鼠标点击事件处理 (局部卡片 DIP 坐标，返回 true 表示事件被小组件控件消耗)
    virtual bool OnClick(float /*localX*/, float /*localY*/) { return false; }

    // 鼠标交互拓展 (局部卡片 DIP 坐标)
    virtual bool OnMouseMove(float /*localX*/, float /*localY*/) { return false; }
    virtual void OnMouseLeave() {}
    virtual bool OnMouseDown(float /*localX*/, float /*localY*/) { return false; }
    virtual bool OnMouseUp(float /*localX*/, float /*localY*/) { return false; }

    // 是否需要呈现手型光标 (IDC_HAND)
    virtual bool WantsHandCursor() const { return false; }
};

} // namespace Windget
