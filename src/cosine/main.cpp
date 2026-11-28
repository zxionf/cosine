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
        xel::Application app{{1280, 840, "cosine"}};

        // Editor 在 Application 之后构造，保证 root/uisys 已就绪；
        // 析构顺序相反，Editor 先销毁。
        xel::editor::Editor editor{app.ui().root(), app.ui().uisys(), app.renderer()};

        // 离屏导出（与 ui_anim_showcase 同一机制）：完成一次布局后渲染到 PNG 并退出。
        if (const char* png = std::getenv("COSINE_EXPORT_PNG")) {
            constexpr std::uint32_t kExportW = 1280;
            constexpr std::uint32_t kExportH = 840;

            // 不依赖窗口尺寸：导出尺寸固定，与窗口 DPI 无关。
            auto& root = app.ui().root();
            root.set_size({static_cast<float>(kExportW), static_cast<float>(kExportH)});
            root.mark_layout_dirty();

            // 先定型一轮几何：Editor::tick 里的调试面板读的是当前布局结果，
            // 不先 layout 的话首帧会显示构造期的坐标。
            root.layout();
            root.clear_layout_dirty();

            editor.tick(1.0f / 60.0f);

            // 可选：先推进 N 秒再导出，用于验证 Player 驱动预览。
            if (const char* seconds = std::getenv("COSINE_EXPORT_TIME")) {
                const int steps = static_cast<int>(std::atof(seconds) * 60.0f);
                for (int i = 0; i < steps; ++i) editor.tick(1.0f / 60.0f);
            }

            const bool ok = app.save_png(png, kExportW, kExportH);
            std::cout << (ok ? "exported " : "export failed ") << png << '\n';
            return ok ? 0 : 1;
        }

        return app.run([&](float dt) { editor.tick(dt); });
    } catch (const std::exception& error) {
        std::cerr << "fatal: " << error.what() << '\n';
        return 1;
    }
}
