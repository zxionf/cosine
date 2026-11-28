#pragma once

#include "ui/input/cursor.hpp"

#include <functional>

namespace xel::ui
{
// 宿主注入点：UISystem 只认识这两个回调，平台细节（GLFW 窗口、系统光标）
// 全部留在 app/backend 层。这样 ui/system 不依赖 backend，也不再轮询窗口尺寸。
struct UiHost
{
    // 空回调 = 不改光标（无窗口的纯逻辑测试）。
    std::function<void(CursorShape)> set_cursor;
};
} // namespace xel::ui
