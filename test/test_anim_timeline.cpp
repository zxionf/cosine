#include "anim/scene.hpp"
#include "anim/timeline.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>

namespace
{
using namespace xel;

bool nearly(float a, float b, float eps = 1e-4f)
{
    return std::fabs(a - b) < eps;
}

void test_lerp_property()
{
    using anim::PropertyValue;

    assert(nearly(std::get<float>(anim::lerp_property(PropertyValue{0.0f}, PropertyValue{10.0f}, 0.5f)), 5.0f));

    const auto v2 = anim::lerp_property(PropertyValue{glm::vec2{0.0f, 0.0f}},
                                        PropertyValue{glm::vec2{10.0f, 20.0f}}, 0.25f);
    assert(nearly(std::get<glm::vec2>(v2).x, 2.5f));
    assert(nearly(std::get<glm::vec2>(v2).y, 5.0f));

    const auto v4 = anim::lerp_property(PropertyValue{glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}},
                                        PropertyValue{glm::vec4{1.0f, 1.0f, 1.0f, 0.0f}}, 0.5f);
    assert(nearly(std::get<glm::vec4>(v4).x, 0.5f));

    // 阶跃类型：不到终点保持前值。
    assert(std::get<bool>(anim::lerp_property(PropertyValue{true}, PropertyValue{false}, 0.99f)));
    assert(!std::get<bool>(anim::lerp_property(PropertyValue{true}, PropertyValue{false}, 1.0f)));
    assert(std::get<std::string>(anim::lerp_property(PropertyValue{std::string{"a"}},
                                                     PropertyValue{std::string{"b"}}, 0.5f))
           == "a");
}

void test_track_sample()
{
    // 空轨道返回默认值。
    anim::Track empty(scene::kInvalidNode, anim::prop::Position);
    assert(nearly(std::get<float>(empty.sample(0.0f)), 0.0f));

    // 单帧：恒定。
    anim::Track one(scene::kInvalidNode, anim::prop::Position);
    one.add_key({1.0f, glm::vec2{5.0f, 0.0f}, anim::Easing::Linear});
    assert(nearly(std::get<glm::vec2>(one.sample(0.0f)).x, 5.0f));
    assert(nearly(std::get<glm::vec2>(one.sample(99.0f)).x, 5.0f));

    // 两帧线性。
    anim::Track two(scene::kInvalidNode, anim::prop::Position);
    two.add_key({0.0f, glm::vec2{0.0f, 0.0f}, anim::Easing::Linear});
    two.add_key({2.0f, glm::vec2{100.0f, 0.0f}, anim::Easing::Linear});
    assert(nearly(std::get<glm::vec2>(two.sample(1.0f)).x, 50.0f));
    // 两端外推取端点。
    assert(nearly(std::get<glm::vec2>(two.sample(-5.0f)).x, 0.0f));
    assert(nearly(std::get<glm::vec2>(two.sample(50.0f)).x, 100.0f));

    // 乱序添加也会排序。
    anim::Track unsorted(scene::kInvalidNode, anim::prop::Position);
    unsorted.add_key({2.0f, glm::vec2{100.0f, 0.0f}, anim::Easing::Linear});
    unsorted.add_key({0.0f, glm::vec2{0.0f, 0.0f}, anim::Easing::Linear});
    assert(unsorted.keys().size() == 2);
    assert(nearly(unsorted.keys()[0].time, 0.0f));
}

void test_track_easing()
{
    anim::Track linear(scene::kInvalidNode, anim::prop::Position);
    linear.add_key({0.0f, glm::vec2{0.0f, 0.0f}, anim::Easing::Linear});
    linear.add_key({1.0f, glm::vec2{100.0f, 0.0f}, anim::Easing::Linear});

    anim::Track ease_in(scene::kInvalidNode, anim::prop::Position);
    ease_in.add_key({0.0f, glm::vec2{0.0f, 0.0f}, anim::Easing::Linear});
    ease_in.add_key({1.0f, glm::vec2{100.0f, 0.0f}, anim::Easing::EaseIn});

    const float mid_linear = std::get<glm::vec2>(linear.sample(0.5f)).x;
    const float mid_ease_in = std::get<glm::vec2>(ease_in.sample(0.5f)).x;
    assert(nearly(mid_linear, 50.0f));
    assert(mid_ease_in < mid_linear); // 缓入在前半段更慢
}

void test_timeline_advance_loop()
{
    anim::Timeline timeline;
    timeline.set_duration(2.0f);
    timeline.set_loop(true);
    timeline.play();

    timeline.advance(1.0f);
    assert(nearly(timeline.time(), 1.0f));
    timeline.advance(1.5f); // 2.5 -> 0.5
    assert(nearly(timeline.time(), 0.5f));
}

void test_timeline_no_loop_clamps()
{
    anim::Timeline timeline;
    timeline.set_duration(1.0f);
    timeline.set_loop(false);
    timeline.play();

    timeline.advance(2.0f);
    assert(nearly(timeline.time(), 1.0f));
    assert(!timeline.playing()); // 到末尾自动暂停
}

void test_timeline_apply_is_deterministic()
{
    scene::Scene s;
    const auto node = s.create(scene::NodeType::Shape, "node");

    anim::Timeline timeline;
    timeline.set_duration(2.0f);
    timeline.set_loop(false);
    auto& track = timeline.add_track(node, anim::prop::Position);
    track.add_key({0.0f, glm::vec2{0.0f, 0.0f}, anim::Easing::Linear});
    track.add_key({2.0f, glm::vec2{100.0f, 40.0f}, anim::Easing::Linear});

    timeline.seek(1.0f);
    timeline.apply(s);
    const auto first = std::get<glm::vec2>(*s.get_property(node, anim::prop::Position));
    assert(nearly(first.x, 50.0f));
    assert(nearly(first.y, 20.0f));

    // 再次采样同一时间，结果必须完全一致（确定性 seek）。
    timeline.apply(s);
    const auto second = std::get<glm::vec2>(*s.get_property(node, anim::prop::Position));
    assert(nearly(second.x, first.x));
    assert(nearly(second.y, first.y));

    // 直接按时间 apply 等价于 seek + apply。
    timeline.apply(s, 1.0f);
    const auto third = std::get<glm::vec2>(*s.get_property(node, anim::prop::Position));
    assert(nearly(third.x, first.x));
}

void test_timeline_time_scale()
{
    anim::Timeline timeline;
    timeline.set_duration(10.0f);
    timeline.set_loop(true);
    timeline.set_time_scale(2.0f);
    timeline.play();

    timeline.advance(1.0f);
    assert(nearly(timeline.time(), 2.0f));
}
} // namespace

int main()
{
    test_lerp_property();
    test_track_sample();
    test_track_easing();
    test_timeline_advance_loop();
    test_timeline_no_loop_clamps();
    test_timeline_apply_is_deterministic();
    test_timeline_time_scale();
    std::printf("test_anim_timeline: all passed\n");
    return 0;
}
