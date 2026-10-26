// 动画演示：用现有 UI 控件 + cosine_anim 的缓动驱动位置与进度。
// 目的：在 M4 的 scene 渲染接入之前，直观展示五种缓动曲线的差异。

#include "anim/easing.hpp"
#include "ui/button.hpp"
#include "ui/color_rect.hpp"
#include "ui/hbox.hpp"
#include "ui/label.hpp"
#include "ui/panel.hpp"
#include "ui/progress.hpp"
#include "ui/root.hpp"
#include "xel_application.hpp"

#include <algorithm>
#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace
{
struct Track
{
    xel::anim::Easing easing = xel::anim::Easing::Linear;
    std::string name;
    xel::ui::ColorRect* marker = nullptr;
    float origin_x = 0.0f;
    float span = 0.0f;
};

struct Demo
{
    xel::ui::Progress* progress = nullptr;
    xel::ui::Label* time_label = nullptr;
    xel::ui::Button* play_button = nullptr;
    std::vector<Track> tracks;
    float time = 0.0f;
    float duration = 3.0f;
    bool playing = true;

    float phase() const { return duration > 0.0f ? time / duration : 0.0f; }

    void update(float dt)
    {
        if (playing) {
            time += dt;
            if (time >= duration) time -= duration;
        }
        const float t = phase();

        // 同一时间、不同曲线：方块位置直接反映缓动结果。
        for (auto& track : tracks) {
            const float e = xel::anim::ease(track.easing, t);
            track.marker->set_pos(track.origin_x + e * track.span, track.marker->y());
        }

        if (progress) progress->set_value(t);
        if (time_label) {
            time_label->set_text(std::format("time: {:.2f} s / {:.1f} s   ({})",
                                             time, duration, playing ? "playing" : "paused"));
        }
    }
};

Demo build(xel::ui::UIRoot& root)
{
    Demo demo;

    auto stage = std::make_unique<xel::ui::Panel>(40.0f, 40.0f, 720.0f, 300.0f);
    stage->set_min_size({720.0f, 300.0f});
    stage->set_max_size({720.0f, 300.0f});

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

    float y = 12.0f;
    for (const auto& def : defs) {
        auto label = std::make_unique<xel::ui::Label>(def.name, 14.0f);
        label->set_absolute(true);
        label->set_absolute_pos({10.0f, y + 5.0f});
        stage->add(std::move(label));

        auto marker = std::make_unique<xel::ui::ColorRect>(0.0f, 0.0f, 36.0f, 22.0f, def.color);
        marker->set_absolute(true);
        marker->set_absolute_pos({150.0f, y});
        demo.tracks.push_back({def.easing, def.name, marker.get(), 150.0f, 540.0f});
        stage->add(std::move(marker));

        y += 46.0f;
    }

    // 轨道参考线（静态）。
    auto guide = std::make_unique<xel::ui::ColorRect>(
        0.0f, 0.0f, 540.0f, 2.0f, glm::vec4{0.35f, 0.38f, 0.42f, 1.0f});
    guide->set_absolute(true);
    guide->set_absolute_pos({150.0f, 250.0f});
    stage->add(std::move(guide));

    root.add(std::move(stage));

    auto controls = std::make_unique<xel::ui::Hbox>(40.0f, 360.0f, 720.0f, 40.0f);
    controls->set_spacing(8.0f);
    controls->set_min_size({720.0f, 40.0f});
    controls->set_max_size({720.0f, 40.0f});

    auto play = std::make_unique<xel::ui::Button>("Pause", [&demo]() {
        demo.playing = !demo.playing;
        if (demo.play_button) demo.play_button->set_label(demo.playing ? "Pause" : "Play");
    });
    demo.play_button = play.get();
    controls->add(std::move(play));

    controls->add(std::make_unique<xel::ui::Button>("Reset", [&demo]() { demo.time = 0.0f; }));
    root.add(std::move(controls));

    auto progress = std::make_unique<xel::ui::Progress>(720.0f);
    progress->set_pos(40.0f, 420.0f);
    demo.progress = progress.get();
    root.add(std::move(progress));

    auto time_label = std::make_unique<xel::ui::Label>("time: 0.00 s", 14.0f);
    time_label->set_pos(40.0f, 462.0f);
    demo.time_label = time_label.get();
    root.add(std::move(time_label));

    return demo;
}

int run()
{
    xel::Application app{1200, 800, "XEL Animation Demo"};
    auto demo = build(*app.uisys.root());

    while (!app.window_.should_close()) {
        glfwPollEvents();

        app.renderer_.set_clear_color(app.uisys.context()->theme.window_bg);
        app.renderer_.begin();

        const auto& stats = app.renderer_.frame_stats();
        const float dt = std::clamp(static_cast<float>(stats.frame_time_ms) / 1000.0f, 0.0f, 0.1f);

        // 先推进动画，再让 UI 系统按新位置布局。
        demo.update(dt);
        app.uisys.tick(dt);

        app.renderer_.draw(*app.uisys.root());
        app.renderer_.end();
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
