// XEL UI Demo 入口。
//
// 只做装配：Application + demo::create() + app.run()。
// 所有展示内容在 sections/ 下，外壳与注册表在 demo.{hpp,cpp}。

#include "demo.hpp"

#include "xel_application.hpp"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>

int main()
{
    try {
        xel::Application app{{1360, 880, "XEL UI Demo"}};

        // 展示台接管整棵 UI 树；缩放开关需要 Application 才能生效，所以注入回调。
        // MTSDF 图标图集占最后一个图片槽（不与业务图片抢号）。
        // 上传回调在 Demo 构造期间调用，所以首个分区里的图标一开始就有 slot。
        const int icon_slot = static_cast<int>(xel::graphics::kMaxImageSlots) - 1;
        auto demo = demo::create(
            app.ui().root(), [&app](float scale) { app.set_ui_scale(scale); },
            [&app, icon_slot](xel::graphics::SdfAtlas& atlas) -> int {
                const bool ok = app.renderer().upload_texture(
                    static_cast<std::uint32_t>(icon_slot), atlas.width(), atlas.height(),
                    atlas.pixels().data());
                if (!ok) {
                    std::cerr << "ui_demo: failed to upload the MTSDF atlas\n";
                    return -1;
                }
                return icon_slot;
            });

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
