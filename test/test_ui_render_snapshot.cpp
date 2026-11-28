// Table/TreeView 迁移的渲染快照安全网。
// 用 render_offscreen 离屏渲染一个固定数据的 Table，输出像素 FNV-1a 哈希，
// 与硬编码基线对比。需要 Vulkan 环境（本地 GPU 或 lavapipe）；无 Vulkan 时 skip。
// 注意：基线哈希对环境敏感（字体光栅化/GPU 差异），只在同一环境内做「迁移前后对比」。
#include "xel_application.hpp"
#include "ui/views/table.hpp"

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace
{
// 迁移前基线（NVIDIA RTX 4060，480×320）。环境敏感：换 GPU/lavapipe 需重新记录。
constexpr uint32_t kBaselineHash = 0x26d00c69;

uint32_t fnv1a(const unsigned char* data, std::size_t n)
{
    uint32_t h = 2166136261u;
    for (std::size_t i = 0; i < n; ++i) {
        h ^= data[i];
        h *= 16777619u;
    }
    return h;
}

std::unique_ptr<xel::ui::Table> build_table()
{
    auto table = std::make_unique<xel::ui::Table>(480.0f, 280.0f);
    table->set_columns({
        {"Name", 180.0f, xel::ui::TableColumn::Align::Left},
        {"Type", 120.0f, xel::ui::TableColumn::Align::Left},
        {"Size", 0.0f, xel::ui::TableColumn::Align::Right},
    });
    std::vector<std::vector<std::string>> rows;
    for (int i = 0; i < 12; ++i) {
        rows.push_back({"item_" + std::to_string(i),
                        (i % 2 == 0) ? "image" : "audio",
                        std::to_string(100 + i * 10) + " KB"});
    }
    table->set_rows(rows);
    table->set_selected(0);
    table->set_pos(0.0f, 0.0f);
    return table;
}
} // namespace

int main()
{
    try {
        xel::Application app{480, 320, "render snapshot"};
        app.ui.root().add(build_table());
        app.ui.tick();

        constexpr uint32_t kW = 480;
        constexpr uint32_t kH = 320;
        auto pixels = app.renderer_.render_offscreen(app.ui.root(), kW, kH);
        if (pixels.empty()) {
            std::printf("skip: render_offscreen returned empty\n");
            return 0;
        }
        const uint32_t hash = fnv1a(pixels.data(), pixels.size());
        std::printf("table snapshot: %zu pixels, fnv1a=%08x\n", pixels.size(), hash);
        if (hash == kBaselineHash) {
            std::printf("snapshot match\n");
            return 0;
        }
        std::printf("snapshot MISMATCH: expected %08x, got %08x\n", kBaselineHash, hash);
        app.renderer_.write_png("table_snapshot.png", kW, kH, pixels);
        return 1;
    } catch (const std::exception& error) {
        std::printf("skip (no vulkan?): %s\n", error.what());
        return 0;
    }
}
