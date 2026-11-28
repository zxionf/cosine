// XEL UI Demo 入口。
//
// 只做装配：Application + demo::create() + app.run()。
// 所有展示内容在 sections/ 下，外壳与注册表在 demo.{hpp,cpp}。

#include "demo.hpp"

#include "xel_application.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>

int main()
{
    try {
        xel::Application app{{1360, 880, "XEL UI Demo"}};

        // 展示台接管整棵 UI 树；缩放开关需要 Application 才能生效，所以注入回调。
        auto demo = demo::create(app.ui().root(),
                                 [&app](float scale) { app.set_ui_scale(scale); });

        // 离屏导出：定格一帧后写 PNG（用于文档截图与回归对比）。
        if (const char* png = std::getenv("COSINE_EXPORT_PNG")) {
            constexpr std::uint32_t kWidth = 1360;
            constexpr std::uint32_t kHeight = 880;
            app.ui().tick(1.0f / 60.0f);
            demo->update(1.0f / 60.0f, app.gfx().stats(), nullptr, nullptr);
            const bool ok = app.save_png(png, kWidth, kHeight);
            std::cout << (ok ? "exported " : "export failed ") << png << '\n';
            return ok ? 0 : 1;
        }

        return app.run([&](float dt) {
            demo->update(dt, app.gfx().stats(), app.ui().hovered(), app.ui().focused());
        });
    } catch (const std::exception& error) {
        std::cerr << "fatal: " << error.what() << '\n';
        return 1;
    }
}
