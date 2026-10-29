// 动画演示：scene::Scene + anim::Timeline + ui::SceneView。
// 每条轨道驱动一个 Shape 节点的 position，显示同一时间下五种缓动的差异。

#include "anim/easing.hpp"
#include "anim/player.hpp"
#include "anim/scene.hpp"
#include "anim/timeline.hpp"
#include "ui/core/button.hpp"
#include "ui/containers/hbox.hpp"
#include "ui/core/label.hpp"
#include "ui/core/progress.hpp"
#include "ui/root.hpp"
#include "ui/views/scene_view.hpp"
#include "xel_application.hpp"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace
{
struct Demo
{
    xel::scene::Scene scene;
    xel::anim::Timeline timeline;
    std::unique_ptr<xel::anim::Player> player;
    xel::ui::SceneView* view = nullptr;
    xel::ui::Progress* progress = nullptr;
    xel::ui::Label* time_label = nullptr;
    xel::ui::Button* play_button = nullptr;
    float duration = 3.0f;

    void update(float dt)
    {
        player->tick(dt); // 播放中推进 + 采样写回场景

        const float t = duration > 0.0f ? player->time() / duration : 0.0f;
        if (progress) progress->set_value(t);
        if (time_label) {
            time_label->set_text(std::format("time: {:.2f} s / {:.1f} s   ({})", player->time(),
                                             duration,
                                             player->playing() ? "playing" : "paused"));
        }
    }
};

void build(Demo& demo, xel::ui::UIRoot& root)
{
    const struct
    {
        xel::anim::Easing easing;
        const char* name;
        glm::vec4 color;
    } defs[] = {
        {xel::anim::Easing::Linear, "Linear", {0.55f, 0.62f, 0.72f, 1.0f}},
        {xel::anim::Easing::EaseIn, "EaseIn", {0.95f, 0.55f, 0.35f, 1.0f}},
        {xel::anim::Easing::EaseOut, "EaseOut", {0.35f, 0.75f, 0.55f, 1.0f}},
        {xel::anim::Easing::EaseInOut, "EaseInOut", {0.30f, 0.60f, 1.00f, 1.0f}},
        {xel::anim::Easing::CubicBezier, "Custom cubic-bezier", {0.85f, 0.45f, 0.85f, 1.0f}},
    };

    const float start_x = 60.0f;
    const float end_x = 520.0f;
    float y = 12.0f;

    for (const auto& def : defs) {
        // 轨道名（Text 节点）。
        const auto label = demo.scene.create(xel::scene::NodeType::Text, def.name);
        auto* label_node = demo.scene.find(label);
        label_node->text = def.name;
        label_node->text_size = 14.0f;
        label_node->color = {0.80f, 0.85f, 0.90f, 1.0f};
        label_node->transform.position = {10.0f, y + 5.0f};

        // 方块（Shape 节点）。
        const auto block = demo.scene.create(xel::scene::NodeType::Shape, def.name);
        auto* block_node = demo.scene.find(block);
        block_node->size = {36.0f, 22.0f};
        block_node->color = def.color;
        block_node->transform.position = {start_x, y};

        auto& track = demo.timeline.add_track(block, xel::anim::prop::Position);
        track.add_key({0.0f, glm::vec2{start_x, y}, xel::anim::Easing::Linear});
        track.add_key({demo.duration, glm::vec2{end_x, y}, def.easing});

        y += 46.0f;
    }

    // 参考线。
    const auto guide = demo.scene.create(xel::scene::NodeType::Shape, "guide");
    auto* guide_node = demo.scene.find(guide);
    guide_node->size = {460.0f, 2.0f};
    guide_node->color = {0.35f, 0.38f, 0.42f, 1.0f};
    guide_node->transform.position = {start_x, 250.0f};

    demo.timeline.set_duration(demo.duration);
    demo.timeline.set_loop(true);

    // Demo 已先于 build 存在，Player 持有的引用稳定。
    demo.player = std::make_unique<xel::anim::Player>(demo.scene, demo.timeline);
    demo.player->play();

    auto view = std::make_unique<xel::ui::SceneView>(&demo.scene);
    view->set_pos(40.0f, 40.0f);
    view->set_size(720.0f, 300.0f);
    demo.view = view.get();
    root.add(std::move(view));

    auto controls = std::make_unique<xel::ui::Hbox>(40.0f, 360.0f, 720.0f, 40.0f);
    controls->set_spacing(8.0f);
    controls->set_min_size({720.0f, 40.0f});
    controls->set_max_size({720.0f, 40.0f});

    auto play = std::make_unique<xel::ui::Button>("Pause", [&demo]() {
        demo.player->toggle();
        if (demo.play_button) {
            demo.play_button->set_label(demo.player->playing() ? "Pause" : "Play");
        }
    });
    demo.play_button = play.get();
    controls->add(std::move(play));

    controls->add(std::make_unique<xel::ui::Button>("Reset", [&demo]() {
        demo.player->seek(0.0f);
    }));
    root.add(std::move(controls));

    auto progress = std::make_unique<xel::ui::Progress>(720.0f);
    progress->set_pos(40.0f, 420.0f);
    demo.progress = progress.get();
    root.add(std::move(progress));

    auto time_label = std::make_unique<xel::ui::Label>("time: 0.00 s", 14.0f);
    time_label->set_pos(40.0f, 462.0f);
    demo.time_label = time_label.get();
    root.add(std::move(time_label));
}

int run()
{
    xel::Application app{1200, 800, "XEL Animation Demo"};

    // Demo 必须先于 build 存在：SceneView 持有 &demo.scene。
    Demo demo;
    build(demo, *app.uisys.root());

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
