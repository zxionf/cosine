#pragma once

#include "scene.hpp"
#include "timeline.hpp"

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace xel::anim
{
// 播放器：把 Timeline 与 Scene 绑在一起，负责推进、应用与时间点事件。
// seek 是确定性的：只同步状态，不触发事件。
class Player
{
public:
    using Callback = std::function<void()>;

    Player(scene::Scene& scene, Timeline& timeline) : scene_(scene), timeline_(timeline) {}

    // 播放中才推进；推进后触发跨过的事件，再把采样结果写回场景。
    void tick(float dt);

    // 确定性跳转：seek + apply，不触发事件。
    void seek(float time);

    // 按当前时间重新应用（例如外部改了轨道后强制刷新）。
    void apply();

    void play() { timeline_.play(); }
    void pause() { timeline_.pause(); }
    void toggle()
    {
        if (playing()) pause();
        else play();
    }
    bool playing() const { return timeline_.playing(); }
    float time() const { return timeline_.time(); }

    // 事件在 tick 跨过其时间点时触发一次；循环回绕会让事件重新生效。
    void add_event(float time, Callback callback);
    void clear_events() { events_.clear(); }
    std::size_t event_count() const { return events_.size(); }

private:
    struct Event
    {
        float time = 0.0f;
        Callback callback;
    };

    void fire_events(float before, float after);

    scene::Scene& scene_;
    Timeline& timeline_;
    std::vector<Event> events_;
};
} // namespace xel::anim
