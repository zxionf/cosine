// 样式系统快照安全网（UI-38）。
//
// 两条互补的断言：
//   1. 语义断言（环境无关）：同一控件的不同角色必须拿到不同颜色。
//      这是 P0-1「单槽过渡导致 Slider 轨道被染成 accent」的回归用例。
//   2. 像素哈希（环境相关）：整幅离屏渲染的 FNV-1a 哈希，用于发现"挪了 1px / 换了个色"的漂移。
//      基线只在同一环境内有效（同 GPU/字体）；换环境后用 COSINE_SNAPSHOT_RECORD=1 重录。
#include "xel_application.hpp"
#include "ui/containers/panel.hpp"
#include "ui/containers/vbox.hpp"
#include "ui/core/button.hpp"
#include "ui/core/checkbox.hpp"
#include "ui/core/label.hpp"
#include "ui/core/progress.hpp"
#include "ui/core/slider.hpp"
#include "ui/style/theme.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace
{
using namespace xel;

// 记录于当前环境（NVIDIA RTX 4060 / Release / Windows / 480×320）。
// 换 GPU 或字体环境后重跑一次，把新哈希填在这里即可。
//
// 历史：
//   833878f4  UI-38 完成时（Checkbox 框体仍直读 checkbox_bg/border）
//   c8a0966c  UI-40 完成后（框体改走 Input 角色，与 TextInput 同色）
constexpr uint32_t kSceneBaselineHash = 0x18857e21;
// 深色场景同一套控件 + Theme::light()（盯住 accent_on 与浅色调色板）。
constexpr uint32_t kLightBaselineHash = 0xb05393f2;

uint32_t fnv1a(const unsigned char* data, std::size_t n)
{
    uint32_t h = 2166136261u;
    for (std::size_t i = 0; i < n; ++i) {
        h ^= data[i];
        h *= 16777619u;
    }
    return h;
}

bool near_color(glm::vec4 a, glm::vec4 b, float eps = 0.02f)
{
    return std::abs(a.r - b.r) < eps && std::abs(a.g - b.g) < eps && std::abs(a.b - b.b) < eps;
}

struct Scene
{
    ui::Slider* slider = nullptr;
    ui::Checkbox* checkbox = nullptr;
    ui::Progress* progress = nullptr;
};

std::unique_ptr<ui::Vbox> build_scene(Scene& out)
{
    auto column = std::make_unique<ui::Vbox>(0.0f, 0.0f, 360.0f, 260.0f);
    column->set_padding(8.0f);
    column->set_spacing(10.0f);
    column->set_cross_align(ui::Align::top_left());

    auto slider = std::make_unique<ui::Slider>(240.0f);
    slider->set_range(0.0f, 100.0f);
    slider->set_value(50.0f); // 非零 → 触发 Accent 填充分支
    out.slider = slider.get();
    column->add(std::move(slider));

    auto checkbox = std::make_unique<ui::Checkbox>("checked row");
    checkbox->set_checked(true);
    out.checkbox = checkbox.get();
    column->add(std::move(checkbox));

    auto progress = std::make_unique<ui::Progress>(200.0f);
    progress->set_value(0.5f);
    out.progress = progress.get();
    column->add(std::move(progress));

    auto button = std::make_unique<ui::Button>("action");
    column->add(std::move(button));

    auto panel = std::make_unique<ui::Panel>(0.0f, 0.0f, 240.0f, 60.0f);
    panel->add(std::make_unique<ui::Label>("panel body", 16.0f));
    column->add(std::move(panel));
    return column;
}

// ---- 语义断言：多角色控件不得串色（进程级，不需要 Vulkan） ----
int check_role_separation()
{
    int failures = 0;
    auto root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
    Scene scene;
    root->add(build_scene(scene));
    root->layout();
    root->clear_layout_dirty();

    // 连续若干帧请求两个角色，模拟一帧内的绘制顺序（Track 先、Accent 后）。
    for (int frame = 0; frame < 8; ++frame) {
        const ui::BoxStyle track = scene.slider->visual_style(ui::StyleRole::Track, ui::ControlState::Normal);
        const ui::BoxStyle fill = scene.slider->visual_style(ui::StyleRole::Accent, ui::ControlState::Normal);
        if (near_color(track.fill, fill.fill)) {
            std::printf("FAIL  slider frame %d: track.fill == fill.fill (%s)\n", frame,
                        near_color(track.fill, fill.fill) ? "same color" : "?");
            ++failures;
            break;
        }
        scene.slider->update(1.0f / 60.0f);
    }

    // 收敛后各自应回到自己的主题目标色。
    const ui::Theme& theme = root->ui_theme();
    const ui::BoxStyle track_target = theme.style(ui::StyleRole::Track, ui::ControlState::Normal);
    const ui::BoxStyle accent_target = theme.style(ui::StyleRole::Accent, ui::ControlState::Normal);
    const ui::BoxStyle track_final = scene.slider->visual_style(ui::StyleRole::Track, ui::ControlState::Normal);
    const ui::BoxStyle accent_final = scene.slider->visual_style(ui::StyleRole::Accent, ui::ControlState::Normal);
    if (!near_color(track_final.fill, track_target.fill)) {
        std::printf("FAIL  slider track did not settle on its own target color\n");
        ++failures;
    }
    if (!near_color(accent_final.fill, accent_target.fill)) {
        std::printf("FAIL  slider fill did not settle on the accent target color\n");
        ++failures;
    }

    // Checkbox 的 Control 与 Accent（勾选块）也不应串色。
    const ui::BoxStyle box = scene.checkbox->visual_style(ui::StyleRole::Control, ui::ControlState::Normal);
    const ui::BoxStyle mark = scene.checkbox->visual_style(ui::StyleRole::Accent, ui::ControlState::Normal);
    if (near_color(box.fill, mark.fill)) {
        std::printf("FAIL  checkbox: control.fill == accent.fill\n");
        ++failures;
    }

    if (failures == 0) std::printf("ok    role separation (slider track != accent, checkbox box != mark)\n");
    return failures;
}

// ---- 像素断言 + 哈希：需要 Vulkan，无环境时 skip ----
int check_pixels()
{
    int failures = 0;
    try {
        xel::Application app{{480, 320, "style snapshot"}};
        Scene scene;
        app.ui().root().add(build_scene(scene));
        app.ui().tick();

        constexpr uint32_t kW = 480;
        constexpr uint32_t kH = 320;
        auto pixels = app.gfx().capture(app.ui().root(), kW, kH);
        if (pixels.empty()) {
            std::printf("skip  render_offscreen empty\n");
            return 0;
        }

        // 取像素（BGRA8 SRGB）。
        const auto at = [&](float x, float y) {
            const auto px = static_cast<std::size_t>(y) * kW + static_cast<std::size_t>(x);
            const unsigned char* p = pixels.data() + px * 4;
            return glm::vec4{p[2], p[1], p[0], p[3]} / 255.0f; // → RGBA
        };

        // 滑轨上下两端：左侧为已填充（accent），右侧为未填充（track）。
        const auto bounds = scene.slider->bounds();
        const float cy = bounds.pos.y + bounds.size.y * 0.5f;
        const auto left = at(bounds.pos.x + bounds.size.x * 0.25f, cy);
        const auto right = at(bounds.pos.x + bounds.size.x * 0.85f, cy);
        if (near_color(left, right, 0.03f)) {
            std::printf("FAIL  slider pixels: filled and empty track are the same color\n");
            ++failures;
        }

        const uint32_t hash = fnv1a(pixels.data(), pixels.size());
        std::printf("scene snapshot: %ux%u fnv1a=%08x\n", kW, kH, hash);
        if (kSceneBaselineHash == 0) {
            std::printf("record baseline: set kSceneBaselineHash = 0x%08x\n", hash);
        } else if (hash != kSceneBaselineHash) {
            std::printf("FAIL  scene hash mismatch: expected %08x\n", kSceneBaselineHash);
            xel::backend::write_png("style_snapshot.png", kW, kH, pixels);
            ++failures;
        }

        if (failures == 0) std::printf("ok    style snapshot pixels\n");

        // 同一场景切浅色主题：验证 accent_on / 浅色调色板不发散。
        app.ui().root().set_theme(ui::Theme::light());
        app.ui().tick();
        auto light_pixels = app.gfx().capture(app.ui().root(), kW, kH);
        if (!light_pixels.empty()) {
            const uint32_t light_hash = fnv1a(light_pixels.data(), light_pixels.size());
            std::printf("light snapshot: fnv1a=%08x\n", light_hash);
            if (kLightBaselineHash == 0) {
                std::printf("record baseline: set kLightBaselineHash = 0x%08x\n", light_hash);
            } else if (light_hash != kLightBaselineHash) {
                std::printf("FAIL  light hash mismatch: expected %08x\n", kLightBaselineHash);
                xel::backend::write_png("style_snapshot_light.png", kW, kH, light_pixels);
                ++failures;
            }
        }
    } catch (const std::exception& error) {
        std::printf("skip  no vulkan: %s\n", error.what());
    }
    return failures;
}
} // namespace

int main()
{
    int failures = 0;
    failures += check_role_separation();
    failures += check_pixels();
    std::printf("style snapshot: %s\n", failures == 0 ? "all passed" : "FAILED");
    return failures == 0 ? 0 : 1;
}
