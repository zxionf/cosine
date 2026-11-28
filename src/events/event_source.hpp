#pragma once

#include "events/event.hpp"

#include <glm/glm.hpp>

#include <span>

namespace xel::event
{
// ── 输入源：把平台事件翻译成与平台无关的 InputEvent ────────────────
//
// 约定：
//   · 坐标空间统一为 **UI 逻辑坐标**（源自己完成 DPR / ui_scale 换算），
//     消费者只需要：src.poll(); for (e : src.events()) ui.dispatch(e);
//   · 事件在下一次 poll() 时清空，期间只读；
//   · 视图尺寸变化单独推送（framebuffer 物理像素），因为它是「状态」而不是「事件」。
//
// 实现者：backend::Window（GLFW）。测试可用手写实现或录制源替换，无需窗口。
class EventSource
{
public:
    virtual ~EventSource() = default;

    // 收集自上次 poll 以来的输入事件（平台回调在内部翻译后入队）。
    virtual void poll() = 0;
    // 上一轮 poll 收集到的事件（逻辑坐标；只读到下次 poll）。
    virtual std::span<const InputEvent> events() const = 0;

    // 视图尺寸是否变化；取走后复位。
    virtual bool has_pending_viewport() const = 0;
    virtual glm::vec2 take_viewport() = 0;

    virtual bool quit_requested() const = 0;
    virtual void request_quit() = 0;

    // 平台按键电平查询（按住检测/调试开关用）；不在映射表里的键一律 false。
    virtual bool key_down(KeyCode key) const = 0;
};
} // namespace xel::event
