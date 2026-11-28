// graphics_demo：基本图形绘制实验台。
//
// 这个示例**不使用 UI 层**：没有控件树、没有布局、没有主题，只做三件事
//   1. 用 RenderBackend（gfx()）开始一帧；
//   2. 用 Canvas 把图元写进 DrawList（世界坐标经相机缩放/平移）；
//   3. 用 FontAtlas 直接画 HUD 文字。
//
// 缩放是实验重点：
//   滚轮   以光标为锚点缩放（不动点不漂移）
//   拖动   平移
//   0      适配内容    + / -  以视口中心缩放
//   [ ]    调整 batching 页的网格密度
//   1-5    切页（图元 / 线段 / 曲线 / 文字 / 批处理）

#include "canvas.hpp"
#include "scene.hpp"

#include "backend/render_backend.hpp"
#include "events/event.hpp"
#include "events/key_code.hpp"
#include "graphics/display_scale.hpp"
#include "graphics/frame_stats.hpp"
#include "xel_application.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>

namespace
{
using gdemo::Camera;
using gdemo::Canvas;
using gdemo::HudInfo;
using gdemo::SceneOptions;

struct State
{
    Camera camera{};
    SceneOptions options{};
    glm::vec2 viewport{1280.0f, 840.0f};
    bool fitted = false;
    bool hud = true;
    bool dragging = false;
    glm::vec2 drag_anchor{0.0f, 0.0f};
    float time = 0.0f;
};

// 世界内容适配视口：内容以原点为中心。
void fit_view(State& state)
{
    state.camera.fit(state.viewport, gdemo::kContentSize);
    state.fitted = true;
}

void handle_event(State& state, const xel::event::InputEvent& event, xel::Application& app)
{
    using xel::event::InputEvent;
    using xel::event::KeyCode;

    switch (event.type) {
        case InputEvent::Type::MouseScroll: {
            // 指数缩放：滚轮量线性，视觉上缩放速率恒定。
            const float factor = std::exp(event.scroll_delta * 0.15f);
            state.camera.zoom_at(event.mouse_pos, factor);
            break;
        }
        case InputEvent::Type::MouseButtonDown:
            if (event.mouse_button == 0) {
                state.dragging = true;
                state.drag_anchor = event.mouse_pos;
            }
            break;
        case InputEvent::Type::MouseButtonUp:
            if (event.mouse_button == 0) state.dragging = false;
            break;
        case InputEvent::Type::MouseMove:
            if (state.dragging) {
                state.camera.pan_by(event.mouse_pos - state.drag_anchor);
                state.drag_anchor = event.mouse_pos;
            }
            break;
        case InputEvent::Type::KeyDown:
            // 数字键 1–9 切页：页数增长时不需要改这里。
            if (event.key >= KeyCode::Digit1 && event.key <= KeyCode::Digit9) {
                const int index = static_cast<int>(event.key) - static_cast<int>(KeyCode::Digit1);
                if (index < static_cast<int>(gdemo::pages().size())) state.options.page = index;
                break;
            }
            switch (event.key) {
                case KeyCode::Digit0:
                    fit_view(state);
                    break;
                case KeyCode::Equal:
                    state.camera.zoom_at(state.viewport * 0.5f, 1.25f);
                    break;
                case KeyCode::Minus:
                    state.camera.zoom_at(state.viewport * 0.5f, 0.8f);
                    break;
                case KeyCode::G:
                    state.options.grid = !state.options.grid;
                    break;
                case KeyCode::H:
                    state.hud = !state.hud;
                    break;
                case KeyCode::LeftBracket:
                    state.options.density = std::max(4, state.options.density - 2);
                    break;
                case KeyCode::RightBracket:
                    state.options.density = std::min(24, state.options.density + 2);
                    break;
                case KeyCode::Escape:
                    app.request_quit();
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

// 逻辑视口 = framebuffer 像素 / ui_scale（与渲染投影同一口径）。
glm::vec2 logical_viewport(const xel::Application& app)
{
    const auto extent = app.window().get_extent();
    return app.display_scale().framebuffer_to_logical(
        {static_cast<float>(extent.width), static_cast<float>(extent.height)});
}
} // namespace

int main()
{
    xel::Application::Options options;
    options.width = 1280;
    options.height = 840;
    options.title = "xel graphics demo";
    xel::Application app{options};

    State state;
    state.viewport = logical_viewport(app);
    if (const char* start = std::getenv("COSINE_START_PAGE")) {
        const int index = gdemo::page_index(start);
        if (index >= 0) state.options.page = index;
    }
    const char* export_path = std::getenv("COSINE_EXPORT_PNG");
    // 导出可以用超采样倍数（像素 = 逻辑 × scale），方便看细节；默认 1。
    float export_scale = 1.0f;
    if (const char* raw = std::getenv("COSINE_EXPORT_SCALE")) {
        export_scale = std::max(0.25f, static_cast<float>(std::atof(raw)));
    }
    bool exported = false;

    Canvas canvas;
    app.run([&](float dt) {
        state.time += dt;

        // 视口变化（窗口 resize / DPI）：保持视口中心的世界点不动。
        const glm::vec2 viewport = logical_viewport(app);
        if (viewport != state.viewport && viewport.x > 1.0f && viewport.y > 1.0f) {
            if (state.fitted) state.camera.pan_by((viewport - state.viewport) * 0.5f);
            state.viewport = viewport;
        }
        if (!state.fitted) fit_view(state);

        for (const auto& event : app.window().events()) handle_event(state, event, app);

        auto& gfx = app.gfx();
        canvas.begin(&gfx.font_atlas(), state.camera, state.viewport);
        gdemo::build_scene(canvas, state.options, state.time);
        if (state.hud) {
            const auto& stats = gfx.stats();
            gdemo::build_hud(canvas, state.options,
                             HudInfo{stats.fps, stats.frame_time_ms,
                                     stats.draw.draw_calls, stats.draw.vertices,
                                     app.ui_scale()});
        }
        gfx.draw(canvas);

        // 离屏导出：尺寸按 ui_scale 换回物理像素，与窗口 DPI 无关。
        if (export_path != nullptr && !exported) {
            exported = true;
            const float scale = app.ui_scale() * export_scale;
            const auto width = static_cast<std::uint32_t>(state.viewport.x * scale);
            const auto height = static_cast<std::uint32_t>(state.viewport.y * scale);
            gfx.set_scale(scale);
            const auto pixels = gfx.capture(canvas, width, height);
            if (xel::backend::write_png(export_path, width, height, pixels)) {
                std::printf("exported %s (%ux%u)\n", export_path, width, height);
            } else {
                std::printf("export failed: %s\n", export_path);
            }
            app.request_quit();
        }
    });

    return 0;
}
