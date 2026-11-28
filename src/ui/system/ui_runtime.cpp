#include "ui/system/ui_runtime.hpp"

#include <algorithm>

namespace xel::ui
{
namespace
{
// DPI 合法区间：0.25×–4×。太小会让命中区域不可用，太大没有实际意义。
constexpr float kMinUiScale = 0.25f;
constexpr float kMaxUiScale = 4.0f;
} // namespace

UiRuntime::UiRuntime() : UiRuntime(std::make_unique<UIRoot>(0.0f, 0.0f, 0.0f, 0.0f)) {}

UiRuntime::UiRuntime(std::unique_ptr<UIRoot> root) : uisys_{std::move(root)} {}

UiRuntime::~UiRuntime() = default;

void UiRuntime::bind_host(Hooks hooks)
{
    hooks_ = std::move(hooks);
    // 光标下沉给调度器（它自己只认识 UiHost 回调，不认识窗口）。
    uisys_.bind_host(UiHost{hooks_.set_cursor});
    // 先注入再补一次：宿主可能在 set_ui_scale 之后才绑定，否则渲染缩放会漏一次。
    apply_render_scale();
}

void UiRuntime::set_font_atlas(graphics::FontAtlas* atlas)
{
    font_atlas_ = atlas;
    if (auto* ctx = uisys_.context()) ctx->font_atlas = atlas;
}

void UiRuntime::set_viewport(glm::vec2 framebuffer_size)
{
    uisys_.set_viewport(framebuffer_size);
}

void UiRuntime::set_ui_scale(float scale)
{
    const float value = std::clamp(scale, kMinUiScale, kMaxUiScale);
    if (value == ui_scale_) return;
    ui_scale_ = value;
    uisys_.set_ui_scale(value); // 逻辑像素换算（root 尺寸 / 输入映射）
    apply_render_scale();       // 渲染缩放 + 字体栅格化
}

void UiRuntime::apply_render_scale()
{
    if (hooks_.set_render_scale) hooks_.set_render_scale(ui_scale_);
}

void UiRuntime::tick()
{
    tick(1.0f / 60.0f);
}

void UiRuntime::tick(float dt)
{
    uisys_.tick(dt);
}
} // namespace xel::ui
