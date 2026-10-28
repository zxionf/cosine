// cosine 编辑器入口（M6.4a 骨架）。
// 只做装配：Application + Editor + 主循环；所有界面逻辑在 src/cosine/editor.*。

#include "editor.hpp"
#include "xel_application.hpp"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <iostream>

int main()
{
    try {
        xel::Application app{1280, 840, "cosine"};

        // Editor 在 Application 之后构造，保证 root/uisys 已就绪；
        // 析构顺序相反，Editor 先销毁。
        xel::editor::Editor editor{*app.uisys.root(), app.uisys, app.renderer_};

        // 离屏导出（与 ui_anim_showcase 同一机制）：完成一次布局后渲染到 PNG 并退出。
        if (const char* png = std::getenv("COSINE_EXPORT_PNG")) {
            constexpr float kExportW = 1280.0f;
            constexpr float kExportH = 840.0f;

            // 不依赖窗口尺寸：窗口可能还没收到 framebuffer 回调（root 尺寸为 0），
            // 那时 layout() 会直接 return，界面会堆在左上角。
            auto* root = app.uisys.root();
            root->set_size(kExportW, kExportH);
            root->set_global_bounds({{0.0f, 0.0f}, {kExportW, kExportH}});
            root->mark_layout_dirty();

            editor.tick(1.0f / 60.0f);

            // 可选：先推进 N 秒再导出，用于验证 Player 驱动预览。
            if (const char* seconds = std::getenv("COSINE_EXPORT_TIME")) {
                const int steps = static_cast<int>(std::atof(seconds) * 60.0f);
                for (int i = 0; i < steps; ++i) editor.tick(1.0f / 60.0f);
            }
            root->layout();
            root->clear_layout_dirty();

            app.renderer_.set_clear_color(app.uisys.context()->theme.window_bg);
            const auto pixels = app.renderer_.render_offscreen(
                *root, static_cast<uint32_t>(kExportW), static_cast<uint32_t>(kExportH));
            const bool ok = xel::backend::vulkan::Renderer::write_png(
                png, static_cast<uint32_t>(kExportW), static_cast<uint32_t>(kExportH), pixels);
            std::cout << (ok ? "exported " : "export failed ") << png << '\n';
            return ok ? 0 : 1;
        }

        const char* exit_after_env = std::getenv("COSINE_EXIT_AFTER");
        const float exit_after = exit_after_env != nullptr ? std::atof(exit_after_env) : 0.0f;
        float total_time = 0.0f;

        while (!app.window_.should_close()) {
            glfwPollEvents();

            app.renderer_.set_clear_color(app.uisys.context()->theme.window_bg);
            app.renderer_.begin();

            const auto& stats = app.renderer_.frame_stats();
            const float dt =
                std::clamp(static_cast<float>(stats.frame_time_ms) / 1000.0f, 0.0f, 0.1f);

            editor.tick(dt);
            app.uisys.tick(dt);
            app.renderer_.draw(*app.uisys.root());
            app.renderer_.end();

            total_time += dt;
            if (exit_after > 0.0f && total_time >= exit_after) {
                glfwSetWindowShouldClose(app.window_.get_glfw_window(), GLFW_TRUE);
            }
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "fatal: " << error.what() << '\n';
        return 1;
    }
}
