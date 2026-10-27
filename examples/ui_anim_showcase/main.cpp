// 动画能力全面展示：缓动对比、层级变换、属性动画（位置/缩放/颜色/文本/可见性）、
// 时间点事件与播放控制。全部由 anim::Scene + Timeline + Player 驱动。

#include "anim/easing.hpp"
#include "anim/player.hpp"
#include "anim/scene.hpp"
#include "anim/timeline.hpp"
#include "ui/button.hpp"
#include "ui/hbox.hpp"
#include "ui/label.hpp"
#include "ui/progress.hpp"
#include "ui/root.hpp"
#include "ui/scene_view.hpp"
#include "xel_application.hpp"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <format>
#include <iostream>
#include <memory>

namespace
{
using xel::anim::Easing;
namespace prop = xel::anim::prop;
using xel::scene::NodeType;

struct Showcase
{
    xel::scene::Scene scene;
    xel::anim::Timeline timeline;
    std::unique_ptr<xel::anim::Player> player;
    xel::ui::SceneView* view = nullptr;
    xel::ui::Progress* progress = nullptr;
    xel::ui::Label* time_label = nullptr;
    xel::ui::Label* event_label = nullptr;
    xel::ui::Button* play_button = nullptr;
    float duration = 6.0f;

    void update(float dt)
    {
        player->tick(dt);

        const float t = duration > 0.0f ? player->time() / duration : 0.0f;
        if (progress) progress->set_value(t);
        if (time_label) {
            time_label->set_text(std::format("time: {:.2f} / {:.1f} s   ({})", player->time(),
                                             duration,
                                             player->playing() ? "playing" : "paused"));
        }
    }
};

// 创建一个纯色矩形节点，返回 id。
xel::scene::NodeId make_rect(xel::scene::Scene& scene, glm::vec2 pos, glm::vec2 size,
                             glm::vec4 color, const std::string& name = {})
{
    const auto id = scene.create(NodeType::Shape, name);
    auto* node = scene.find(id);
    node->transform.position = pos;
    node->size = size;
    node->color = color;
    return id;
}

xel::scene::NodeId make_text(xel::scene::Scene& scene, glm::vec2 pos, std::string text,
                             float size, glm::vec4 color)
{
    const auto id = scene.create(NodeType::Text, text);
    auto* node = scene.find(id);
    node->transform.position = pos;
    node->text = std::move(text);
    node->text_size = size;
    node->color = color;
    return id;
}

void build(Showcase& demo, xel::ui::UIRoot& root)
{
    const glm::vec4 label_color{0.78f, 0.83f, 0.90f, 1.0f};

    // ---------- 展区 A：五种缓动 ----------
    make_text(demo.scene, {10.0f, 4.0f}, "A. Easing: same 6s, different curves", 15.0f,
              glm::vec4{0.55f, 0.75f, 1.0f, 1.0f});

    const struct
    {
        Easing easing;
        const char* name;
        glm::vec4 color;
    } curves[] = {
        {Easing::Linear, "Linear", {0.55f, 0.62f, 0.72f, 1.0f}},
        {Easing::EaseIn, "EaseIn", {0.95f, 0.55f, 0.35f, 1.0f}},
        {Easing::EaseOut, "EaseOut", {0.35f, 0.75f, 0.55f, 1.0f}},
        {Easing::EaseInOut, "EaseInOut", {0.30f, 0.60f, 1.00f, 1.0f}},
        {Easing::CubicBezier, "CubicBezier", {0.85f, 0.45f, 0.85f, 1.0f}},
    };

    float y = 26.0f;
    for (const auto& curve : curves) {
        make_text(demo.scene, {16.0f, y + 3.0f}, curve.name, 12.0f, label_color);
        const auto block = make_rect(demo.scene, {120.0f, y}, {28.0f, 18.0f}, curve.color,
                                     curve.name);
        auto& track = demo.timeline.add_track(block, prop::Position);
        track.add_key({0.0f, glm::vec2{120.0f, y}, Easing::Linear});
        track.add_key({demo.duration, glm::vec2{660.0f, y}, curve.easing});
        y += 26.0f;
    }

    // ---------- 展区 B：层级变换 ----------
    make_text(demo.scene, {10.0f, 172.0f}, "B. Hierarchy: parent scale pulse + child offsets",
              15.0f, glm::vec4{0.55f, 0.75f, 1.0f, 1.0f});

    const auto group = demo.scene.create(NodeType::Group, "group");
    auto* group_node = demo.scene.find(group);
    group_node->transform.position = {200.0f, 240.0f};

    const glm::vec4 child_colors[] = {
        {0.90f, 0.45f, 0.40f, 1.0f},
        {0.40f, 0.80f, 0.50f, 1.0f},
        {0.40f, 0.60f, 0.95f, 1.0f},
    };
    for (int i = 0; i < 3; ++i) {
        const auto child = make_rect(demo.scene, {static_cast<float>(i) * 56.0f, 0.0f},
                                     {44.0f, 44.0f}, child_colors[i], "child");
        demo.scene.set_parent(child, group);
    }

    auto& pulse = demo.timeline.add_track(group, prop::Scale);
    pulse.add_key({0.0f, glm::vec2{1.0f, 1.0f}, Easing::Linear});
    pulse.add_key({1.5f, glm::vec2{1.4f, 1.4f}, Easing::EaseInOut});
    pulse.add_key({3.0f, glm::vec2{1.0f, 1.0f}, Easing::EaseInOut});
    pulse.add_key({6.0f, glm::vec2{1.0f, 1.0f}, Easing::Linear});

    // ---------- 展区 C：属性动画 ----------
    make_text(demo.scene, {10.0f, 312.0f},
              "C. Properties: position + scale + color + text + visible", 15.0f,
              glm::vec4{0.55f, 0.75f, 1.0f, 1.0f});

    // 位置 + 缩放 + 颜色。
    const auto mover = make_rect(demo.scene, {80.0f, 350.0f}, {60.0f, 40.0f},
                                 {0.95f, 0.40f, 0.40f, 1.0f}, "mover");
    auto& pos_track = demo.timeline.add_track(mover, prop::Position);
    pos_track.add_key({0.0f, glm::vec2{80.0f, 350.0f}, Easing::Linear});
    pos_track.add_key({3.0f, glm::vec2{300.0f, 350.0f}, Easing::EaseInOut});
    pos_track.add_key({6.0f, glm::vec2{80.0f, 350.0f}, Easing::EaseInOut});

    auto& scale_track = demo.timeline.add_track(mover, prop::Scale);
    scale_track.add_key({0.0f, glm::vec2{1.0f, 1.0f}, Easing::Linear});
    scale_track.add_key({3.0f, glm::vec2{1.6f, 0.7f}, Easing::EaseOut});
    scale_track.add_key({6.0f, glm::vec2{1.0f, 1.0f}, Easing::EaseIn});

    auto& color_track = demo.timeline.add_track(mover, prop::Color);
    color_track.add_key({0.0f, glm::vec4{0.95f, 0.40f, 0.40f, 1.0f}, Easing::Linear});
    color_track.add_key({3.0f, glm::vec4{0.40f, 0.55f, 0.95f, 1.0f}, Easing::EaseInOut});
    color_track.add_key({6.0f, glm::vec4{0.95f, 0.40f, 0.40f, 1.0f}, Easing::EaseInOut});

    // 文本阶跃。
    const auto caption = make_text(demo.scene, {420.0f, 360.0f}, "phase: 0", 18.0f,
                                   {0.90f, 0.92f, 0.98f, 1.0f});
    auto& text_track = demo.timeline.add_track(caption, prop::Text);
    text_track.add_key({0.0f, std::string{"phase: 0 / 3"}, Easing::Linear});
    text_track.add_key({2.0f, std::string{"phase: 1 / 3"}, Easing::Linear});
    text_track.add_key({4.0f, std::string{"phase: 2 / 3"}, Easing::Linear});
    text_track.add_key({5.9f, std::string{"phase: 3 / 3"}, Easing::Linear});

    // 可见性闪烁。
    const auto blinker = make_rect(demo.scene, {420.0f, 400.0f}, {40.0f, 24.0f},
                                   {0.95f, 0.80f, 0.35f, 1.0f}, "blink");
    auto& visible_track = demo.timeline.add_track(blinker, prop::Visible);
    for (int i = 0; i < 6; ++i) {
        visible_track.add_key({static_cast<float>(i), (i % 2) == 0, Easing::Linear});
    }
    visible_track.add_key({6.0f, true, Easing::Linear});

    make_text(demo.scene, {470.0f, 402.0f}, "visible on/off", 12.0f, label_color);

    demo.timeline.set_duration(demo.duration);
    demo.timeline.set_loop(true);
    demo.player = std::make_unique<xel::anim::Player>(demo.scene, demo.timeline);
    demo.player->play();

    // 时间点事件：更新事件标签。
    demo.player->add_event(2.0f, [&demo]() {
        if (demo.event_label) demo.event_label->set_text("event @2.0s: phase 1");
    });
    demo.player->add_event(4.0f, [&demo]() {
        if (demo.event_label) demo.event_label->set_text("event @4.0s: phase 2");
    });

    // ---------- UI ----------
    auto view = std::make_unique<xel::ui::SceneView>(&demo.scene);
    view->set_pos(40.0f, 40.0f);
    view->set_size(720.0f, 500.0f);
    demo.view = view.get();
    root.add(std::move(view));

    auto controls = std::make_unique<xel::ui::Hbox>(40.0f, 556.0f, 720.0f, 36.0f);
    controls->set_spacing(6.0f);
    controls->set_min_size({720.0f, 36.0f});
    controls->set_max_size({720.0f, 36.0f});

    auto play = std::make_unique<xel::ui::Button>("Pause", [&demo]() {
        demo.player->toggle();
        if (demo.play_button) {
            demo.play_button->set_label(demo.player->playing() ? "Pause" : "Play");
        }
    });
    demo.play_button = play.get();
    controls->add(std::move(play));

    controls->add(std::make_unique<xel::ui::Button>("Back 1s", [&demo]() {
        demo.player->seek(std::max(0.0f, demo.player->time() - 1.0f));
    }));
    controls->add(std::make_unique<xel::ui::Button>("Fwd 1s", [&demo]() {
        demo.player->seek(std::min(demo.duration, demo.player->time() + 1.0f));
    }));
    controls->add(std::make_unique<xel::ui::Button>("Reset", [&demo]() {
        demo.player->seek(0.0f);
    }));
    controls->add(std::make_unique<xel::ui::Button>("Speed 1x", [&demo]() {
        const float next = demo.timeline.time_scale() >= 2.0f ? 0.5f
            : demo.timeline.time_scale() >= 1.0f ? 2.0f
                                                 : 1.0f;
        demo.timeline.set_time_scale(next);
    }));
    controls->add(std::make_unique<xel::ui::Button>("Loop: On", [&demo]() {
        demo.timeline.set_loop(!demo.timeline.loop());
    }));
    root.add(std::move(controls));

    auto progress = std::make_unique<xel::ui::Progress>(720.0f);
    progress->set_pos(40.0f, 604.0f);
    demo.progress = progress.get();
    root.add(std::move(progress));

    auto time_label = std::make_unique<xel::ui::Label>("time: 0.00 s", 13.0f);
    time_label->set_pos(40.0f, 640.0f);
    demo.time_label = time_label.get();
    root.add(std::move(time_label));

    auto event_label = std::make_unique<xel::ui::Label>("event: -", 13.0f);
    event_label->set_pos(40.0f, 664.0f);
    demo.event_label = event_label.get();
    root.add(std::move(event_label));
}

int run()
{
    xel::Application app{1200, 800, "XEL Animation Showcase"};

    Showcase demo;
    build(demo, *app.uisys.root());

    // 离屏导出：推进一次布局与动画后渲染到 PNG，然后退出（M6.0）。
    if (const char* png = std::getenv("COSINE_EXPORT_PNG")) {
        constexpr float kExportW = 1200.0f;
        constexpr float kExportH = 800.0f;

        demo.update(1.0f);
        app.uisys.tick(1.0f / 60.0f);

        // 导出尺寸与窗口 DPI 无关：root 与离屏目标使用同一逻辑尺寸。
        auto* root = app.uisys.root();
        root->set_size(kExportW, kExportH);
        root->set_global_bounds({{0.0f, 0.0f}, {kExportW, kExportH}});
        root->mark_layout_dirty();
        root->layout();
        root->clear_layout_dirty();

        app.renderer_.set_clear_color(app.uisys.context()->theme.window_bg);
        const auto pixels = app.renderer_.render_offscreen(*root, static_cast<uint32_t>(kExportW),
                                                          static_cast<uint32_t>(kExportH));
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
        const float dt = std::clamp(static_cast<float>(stats.frame_time_ms) / 1000.0f, 0.0f, 0.1f);

        demo.update(dt);
        app.uisys.tick(dt);

        app.renderer_.draw(*app.uisys.root());
        app.renderer_.end();

        total_time += dt;
        if (exit_after > 0.0f && total_time >= exit_after) {
            glfwSetWindowShouldClose(app.window_.get_glfw_window(), GLFW_TRUE);
        }
    }
    return 0;
}
} // namespace

int main()
{
    try {
        return run();
    } catch (const std::exception& error) {
        std::cerr << "fatal: " << error.what() << '\n';
        return 1;
    }
}
