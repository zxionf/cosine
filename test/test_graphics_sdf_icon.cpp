// MTSDF 图集图标的端到端验证（需要 GPU）。
//
// 链路：PNG+JSON → SdfAtlas → upload_texture（图片槽）→ kind=3 quad →
//       fragment 里 median(RGB) 重建距离 → 1px 过渡。
//
// 断言的是「与分辨率无关」：同一图标画 32px 与 128px，边缘覆盖率曲线形状一致。

#include "graphics/sdf.hpp"
#include "graphics/sdf_atlas.hpp"
#include "xel_application.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
namespace fs = std::filesystem;

int failures = 0;

void check(bool condition, const std::string& name, const std::string& detail = {})
{
    if (condition) {
        std::printf("ok      %s\n", name.c_str());
        return;
    }
    ++failures;
    std::printf("FAIL    %s", name.c_str());
    if (!detail.empty()) std::printf("  [%s]", detail.c_str());
    std::printf("\n");
}

// 只画一个图标：把 kind=3 的 quad 包成一个 Drawable。
class IconDrawable final : public xel::graphics::Drawable
{
public:
    IconDrawable(int slot, glm::vec4 uv, glm::vec4 rect, float px_range)
    : slot_{slot}, uv_{uv}, rect_{rect}, px_range_{px_range}
    {}

    void append_to(xel::graphics::DrawList& list) const override
    {
        xel::graphics::append_sdf_icon(list, slot_, uv_, rect_, {1.0f, 1.0f, 1.0f, 1.0f},
                                       px_range_);
    }

    xel::graphics::TextureSlot texture_slot() const override
    {
        return xel::graphics::TextureSlot::Texture;
    }

private:
    int slot_;
    glm::vec4 uv_;
    glm::vec4 rect_;
    float px_range_;
};

float alpha_at(const std::vector<unsigned char>& rgba, int width, int x, int y)
{
    const std::size_t offset = (static_cast<std::size_t>(y) * width + x) * 4;
    return static_cast<float>(rgba[offset + 3]) / 255.0f;
}

// 沿一行统计「从透明到不透明」的过渡宽度（像素）。
// 沿一行找「透明 → 不透明」的过渡带宽度（像素）。
// 注意不是整行跨度：图标内部是实心的，两端各有一条边。
int edge_width(const std::vector<unsigned char>& rgba, int width, int height, int y)
{
    if (y < 0 || y >= height) return 0;
    for (int x = 0; x < width; ++x) {
        if (alpha_at(rgba, width, x, y) <= 0.5f) continue;
        int start = x;
        while (start > 0 && alpha_at(rgba, width, start - 1, y) > 0.02f) --start;
        return x - start + 1;
    }
    return 0;
}

int opaque_count(const std::vector<unsigned char>& rgba, int width, int height)
{
    int count = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (alpha_at(rgba, width, x, y) > 0.5f) ++count;
        }
    }
    return count;
}
} // namespace

int main()
{
    const fs::path atlas_png = fs::path(COSINE_SOURCE_DIR) / "assets" / "sdf" / "ui_icons.png";
    const fs::path atlas_json = fs::path(COSINE_SOURCE_DIR) / "assets" / "sdf" / "ui_icons.json";
    check(fs::exists(atlas_png) && fs::exists(atlas_json), "atlas assets exist");

    xel::graphics::SdfAtlas atlas;
    std::string error;
    check(atlas.load(atlas_png.string(), atlas_json.string(), &error), "atlas loads", error);
    if (!atlas.valid()) return 1;
    check(atlas.px_range() > 0.0f, "atlas has px_range");
    const int icon = atlas.icon_index("play");
    check(icon >= 0, "icon 'play' exists");
    glm::vec4 uv{0.0f};
    check(atlas.icon_uv(icon, uv), "icon uv resolves");

    xel::Application app{{320, 320, "sdf icon"}};

    // SDF 图集放最后一个图片槽，避免和业务图片抢号。
    const int slot = static_cast<int>(xel::graphics::kMaxImageSlots) - 1;
    check(app.renderer().upload_texture(static_cast<uint32_t>(slot), atlas.width(), atlas.height(),
                                        atlas.pixels().data()),
          "atlas uploads to an image slot");

    const auto render = [&](int size) {
        IconDrawable drawable(slot, uv, {0.0f, 0.0f, static_cast<float>(size),
                                         static_cast<float>(size)},
                              atlas.px_range());
        return app.gfx().capture(drawable, static_cast<uint32_t>(size),
                                 static_cast<uint32_t>(size));
    };

    const auto small = render(32);
    const auto large = render(128);
    check(!small.empty() && !large.empty(), "offscreen capture returns pixels");
    if (small.empty() || large.empty()) return 1;

    // 32px：图标内部有不透明像素，四角透明。
    const int small_opaque = opaque_count(small, 32, 32);
    check(small_opaque > 32 * 32 / 10, "32px icon has a filled interior",
          "opaque=" + std::to_string(small_opaque));
    check(alpha_at(small, 32, 0, 0) < 0.05f, "32px icon corner is transparent");
    check(alpha_at(small, 32, 31, 31) < 0.05f, "32px icon opposite corner is transparent");

    // 128px：同样有内部填充，且边缘过渡仍是 1~2 像素。
    const int large_opaque = opaque_count(large, 128, 128);
    check(large_opaque > small_opaque, "128px icon covers more pixels",
          std::to_string(small_opaque) + " -> " + std::to_string(large_opaque));
    const int edge = edge_width(large, 128, 128, 64);
    check(edge >= 1 && edge <= 3, "128px edge transition stays ~1px",
          "edge=" + std::to_string(edge));

    // 覆盖率的近似一致性：32px 与 128px 的填充面积比应接近 1:16（面积）。
    // 调试：把渲染结果落盘看边缘（不参与断言）。
    if (const char* path = std::getenv("COSINE_SDF_SNAPSHOT")) {
        xel::backend::write_png(path, 128, 128, large);
    }

    const double ratio = static_cast<double>(large_opaque) / std::max(1, small_opaque);
    check(ratio > 10.0 && ratio < 22.0, "icon area scales proportionally",
          "ratio=" + std::to_string(ratio));

    if (failures != 0) {
        std::printf("sdf icon: %d failed\n", failures);
        return 1;
    }
    std::printf("sdf icon: all checks passed\n");
    return 0;
}
