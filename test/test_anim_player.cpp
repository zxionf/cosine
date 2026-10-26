#include "anim/player.hpp"
#include "anim/scene.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

namespace
{
using namespace xel;

bool nearly(float a, float b, float eps = 1e-3f)
{
    return std::fabs(a - b) < eps;
}

// 构造一个 0→100 的位置轨道。
anim::Timeline make_timeline(scene::Scene& scene, scene::NodeId node, float duration, bool loop)
{
    anim::Timeline timeline;
    timeline.set_duration(duration);
    timeline.set_loop(loop);
    auto& track = timeline.add_track(node, anim::prop::Position);
    track.add_key({0.0f, glm::vec2{0.0f, 0.0f}, anim::Easing::Linear});
    track.add_key({duration, glm::vec2{100.0f, 0.0f}, anim::Easing::Linear});
    return timeline;
}

void test_tick_applies_and_fires_once()
{
    scene::Scene scene;
    const auto node = scene.create(scene::NodeType::Shape, "n");
    auto timeline = make_timeline(scene, node, 2.0f, true);

    anim::Player player(scene, timeline);
    int fired = 0;
    player.add_event(1.0f, [&fired] { ++fired; });

    player.play();
    player.tick(0.4f); // 0.0 -> 0.4
    assert(fired == 0);
    player.tick(0.4f); // 0.4 -> 0.8
    assert(fired == 0);
    player.tick(0.4f); // 0.8 -> 1.2，跨过 1.0
    assert(fired == 1);
    player.tick(0.4f); // 1.2 -> 1.6，不再触发
    assert(fired == 1);

    // 场景已按时间应用。
    const auto pos = std::get<glm::vec2>(*scene.get_property(node, anim::prop::Position));
    assert(pos.x > 0.0f);
}

void test_paused_does_not_advance()
{
    scene::Scene scene;
    const auto node = scene.create(scene::NodeType::Shape);
    auto timeline = make_timeline(scene, node, 2.0f, true);
    anim::Player player(scene, timeline);

    // 默认未播放。
    player.tick(1.0f);
    assert(nearly(player.time(), 0.0f));

    player.play();
    player.tick(0.5f);
    assert(nearly(player.time(), 0.5f));

    player.pause();
    player.tick(1.0f);
    assert(nearly(player.time(), 0.5f));
}

void test_loop_rearms_events()
{
    scene::Scene scene;
    const auto node = scene.create(scene::NodeType::Shape);
    auto timeline = make_timeline(scene, node, 1.0f, true);

    anim::Player player(scene, timeline);
    int fired = 0;
    player.add_event(0.5f, [&fired] { ++fired; });

    player.play();
    player.tick(0.6f); // 0.0 -> 0.6，跨过 0.5
    assert(fired == 1);
    player.tick(0.6f); // 0.6 -> 0.2（回绕），事件 0.5 在区间外
    assert(fired == 1);
    player.tick(0.4f); // 0.2 -> 0.6，再次跨过 0.5
    assert(fired == 2);
}

void test_seek_does_not_fire_events()
{
    scene::Scene scene;
    const auto node = scene.create(scene::NodeType::Shape);
    auto timeline = make_timeline(scene, node, 2.0f, false);
    anim::Player player(scene, timeline);

    int fired = 0;
    player.add_event(1.0f, [&fired] { ++fired; });

    player.seek(1.5f);
    assert(fired == 0);
    assert(nearly(player.time(), 1.5f));

    // seek 也会应用场景。
    const auto pos = std::get<glm::vec2>(*scene.get_property(node, anim::prop::Position));
    assert(nearly(pos.x, 75.0f));
}

void test_multiple_events_in_one_tick()
{
    scene::Scene scene;
    const auto node = scene.create(scene::NodeType::Shape);
    auto timeline = make_timeline(scene, node, 4.0f, true);
    anim::Player player(scene, timeline);

    int a = 0;
    int b = 0;
    player.add_event(0.5f, [&a] { ++a; });
    player.add_event(1.5f, [&b] { ++b; });

    player.play();
    player.tick(2.0f); // 一次跨过两个事件
    assert(a == 1);
    assert(b == 1);
}

void test_clear_events()
{
    scene::Scene scene;
    const auto node = scene.create(scene::NodeType::Shape);
    auto timeline = make_timeline(scene, node, 1.0f, true);
    anim::Player player(scene, timeline);

    int fired = 0;
    player.add_event(0.5f, [&fired] { ++fired; });
    assert(player.event_count() == 1);

    player.clear_events();
    assert(player.event_count() == 0);
    player.play();
    player.tick(0.6f);
    assert(fired == 0);
}
} // namespace

int main()
{
    test_tick_applies_and_fires_once();
    test_paused_does_not_advance();
    test_loop_rearms_events();
    test_seek_does_not_fire_events();
    test_multiple_events_in_one_tick();
    test_clear_events();
    std::printf("test_anim_player: all passed\n");
    return 0;
}
