#include "ui/timeline_view.hpp"

#include "anim/easing.hpp"
#include "anim/property.hpp"
#include "anim/serialize.hpp"
#include "anim/timeline.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

namespace
{
namespace prop = xel::anim::prop;
using xel::anim::deserialize;
using xel::anim::Easing;
using xel::anim::Keyframe;
using xel::anim::Project;
using xel::anim::serialize;
using xel::anim::Timeline;
using xel::ui::TimelineView;

constexpr float kEps = 1.0e-3f;

// set_key_time 必须保持关键帧有序。
void test_set_key_time_keeps_order()
{
    Timeline timeline;
    auto& track = timeline.add_track(1, prop::Position);
    track.add_key({0.0f, glm::vec2{0.0f, 0.0f}, Easing::Linear});
    track.add_key({2.0f, glm::vec2{1.0f, 1.0f}, Easing::Linear});
    track.add_key({4.0f, glm::vec2{2.0f, 2.0f}, Easing::Linear});

    assert(track.set_key_time(2.0f, 3.0f));
    const auto& keys = track.keys();
    assert(keys.size() == 3);
    assert(std::fabs(keys[0].time - 0.0f) < kEps);
    assert(std::fabs(keys[1].time - 3.0f) < kEps);
    assert(std::fabs(keys[2].time - 4.0f) < kEps);
    // 移动后值保持不变。
    assert(std::fabs(std::get<glm::vec2>(keys[1].value).x - 1.0f) < kEps);

    // 移到最前，顺序重排。
    assert(track.set_key_time(4.0f, 0.5f));
    assert(std::fabs(track.keys()[0].time - 0.0f) < kEps);
    assert(std::fabs(track.keys()[1].time - 0.5f) < kEps);
    assert(std::fabs(track.keys()[2].time - 3.0f) < kEps);

    // 找不到时返回 false。
    assert(!track.set_key_time(9.0f, 1.0f));
}

// 点击标尺 → seek 到对应时间。
void test_click_ruler_seeks()
{
    Timeline timeline;
    timeline.set_duration(5.0f);

    TimelineView view(&timeline);
    view.set_pixels_per_second(100.0f);
    view.arrange({{0.0f, 0.0f}, {800.0f, 300.0f}});

    float seeked = -1.0f;
    view.set_on_seek([&seeked](float seconds) { seeked = seconds; });

    // track_left = 160，左侧留 8px；2s → 168 + 200 = 368。
    assert(view.on_mouse_down({368.0f, 12.0f}, 0));
    assert(std::fabs(seeked - 2.0f) < kEps);
    assert(std::fabs(view.time() - 2.0f) < kEps);

    // 超出 duration 时被夹住。
    assert(view.on_mouse_down({2000.0f, 12.0f}, 0));
    assert(std::fabs(seeked - 5.0f) < kEps);
}

// 拖动关键帧改时间。
void test_drag_keyframe()
{
    Timeline timeline;
    timeline.set_duration(5.0f);
    auto& track = timeline.add_track(1, prop::Position);
    track.add_key({1.0f, glm::vec2{0.0f, 0.0f}, Easing::Linear});

    TimelineView view(&timeline);
    view.set_pixels_per_second(100.0f);
    view.arrange({{0.0f, 0.0f}, {800.0f, 300.0f}});

    int moved = 0;
    view.set_on_key_moved([&moved] { ++moved; });

    // 1s 的关键帧：x = 168 + 100 = 268，首行中心 y = 34 + 18 = 52。
    assert(view.on_mouse_down({268.0f, 52.0f}, 0));
    assert(view.selected_track() == 0);
    assert(std::fabs(view.selected_time() - 1.0f) < kEps);

    // 拖到 3s（x = 468）。
    assert(view.on_mouse_move({468.0f, 52.0f}));
    assert(moved >= 1);
    assert(std::fabs(view.selected_time() - 3.0f) < kEps);
    assert(std::fabs(track.keys()[0].time - 3.0f) < kEps);

    // 松开后不再响应移动。
    assert(view.on_mouse_up({468.0f, 52.0f}, 0));
    assert(!view.on_mouse_move({568.0f, 52.0f}));
}

// 拖播放头应连续跟随。
void test_drag_playhead()
{
    Timeline timeline;
    timeline.set_duration(5.0f);

    TimelineView view(&timeline);
    view.set_pixels_per_second(100.0f);
    view.arrange({{0.0f, 0.0f}, {800.0f, 300.0f}});

    float seeked = -1.0f;
    view.set_on_seek([&seeked](float seconds) { seeked = seconds; });

    // 在标尺区域按下 1s（x = 168 + 100 = 268，y 在标尺内）。
    assert(view.on_mouse_down({268.0f, 10.0f}, 0));
    assert(std::fabs(seeked - 1.0f) < kEps);

    // 拖到 3s（x = 468）→ 播放头连续跟随。
    assert(view.on_mouse_move({468.0f, 10.0f}));
    assert(std::fabs(seeked - 3.0f) < kEps);
    assert(std::fabs(view.time() - 3.0f) < kEps);

    // 松开后不再跟随。
    assert(view.on_mouse_up({468.0f, 10.0f}, 0));
    assert(!view.on_mouse_move({568.0f, 10.0f}));
}

// 点击左侧标签区不应 seek。
void test_click_label_column_ignored()
{
    Timeline timeline;
    TimelineView view(&timeline);
    view.arrange({{0.0f, 0.0f}, {800.0f, 300.0f}});

    float seeked = -1.0f;
    view.set_on_seek([&seeked](float seconds) { seeked = seconds; });
    assert(!view.on_mouse_down({40.0f, 80.0f}, 0));
    assert(seeked < 0.0f);
}

// 右键删除命中的关键帧；轨道空后一并移除。
void test_right_click_removes_key()
{
    Timeline timeline;
    timeline.set_duration(5.0f);
    auto& track = timeline.add_track(1, prop::Position);
    track.add_key({1.0f, glm::vec2{0.0f, 0.0f}, Easing::Linear});

    TimelineView view(&timeline);
    view.set_pixels_per_second(100.0f);
    view.arrange({{0.0f, 0.0f}, {800.0f, 300.0f}});

    int changed = 0;
    view.set_on_changed([&changed] { ++changed; });

    // x = 168 + 100 = 268；首行中心 y = 34 + 18 = 52。
    assert(view.on_right_click({268.0f, 52.0f}));
    assert(changed == 1);
    assert(timeline.tracks().empty());

    // 已无关键帧，右键不应再命中。
    assert(!view.on_right_click({268.0f, 52.0f}));
}

// 双击循环插值：Linear → EaseIn。
void test_double_click_cycles_easing()
{
    Timeline timeline;
    timeline.set_duration(5.0f);
    auto& track = timeline.add_track(1, prop::Position);
    track.add_key({1.0f, glm::vec2{0.0f, 0.0f}, Easing::Linear});

    TimelineView view(&timeline);
    view.set_pixels_per_second(100.0f);
    view.arrange({{0.0f, 0.0f}, {800.0f, 300.0f}});

    assert(view.on_double_click({268.0f, 52.0f}, 0));
    assert(timeline.tracks()[0].keys()[0].easing == Easing::EaseIn);

    assert(view.on_double_click({268.0f, 52.0f}, 0));
    assert(timeline.tracks()[0].keys()[0].easing == Easing::EaseOut);

    // 值不受影响。
    assert(std::fabs(std::get<glm::vec2>(timeline.tracks()[0].keys()[0].value).x) < kEps);
    assert(std::fabs(timeline.tracks()[0].keys()[0].time - 1.0f) < kEps);
}

// 框选多个关键帧，拖动其中一个 → 全部同步平移。
void test_box_select_and_move()
{
    Timeline timeline;
    timeline.set_duration(5.0f);
    auto& track = timeline.add_track(1, prop::Position);
    track.add_key({1.0f, glm::vec2{0.0f, 0.0f}, Easing::Linear});
    track.add_key({2.0f, glm::vec2{1.0f, 1.0f}, Easing::Linear});

    TimelineView view(&timeline);
    view.set_pixels_per_second(100.0f);
    view.arrange({{0.0f, 0.0f}, {800.0f, 300.0f}});

    // 从轨道区空白开始框选，矩形盖住 x=268（1s）与 x=368（2s）两个菱形。
    assert(view.on_mouse_down({200.0f, 40.0f}, 0));
    assert(view.on_mouse_move({420.0f, 70.0f}));
    assert(view.on_mouse_up({420.0f, 70.0f}, 0));
    assert(view.selection_count() == 2);

    // 拖其中一个右移 1s，两个一起走。
    assert(view.on_mouse_down({268.0f, 52.0f}, 0));
    assert(view.on_mouse_move({368.0f, 52.0f}));
    view.on_mouse_up({368.0f, 52.0f}, 0);

    const auto& keys = timeline.tracks()[0].keys();
    assert(keys.size() == 2);
    assert(std::fabs(keys[0].time - 2.0f) < kEps);
    assert(std::fabs(keys[1].time - 3.0f) < kEps);
}

// 拖动会吸附到整秒。
void test_drag_snaps_to_second()
{
    Timeline timeline;
    timeline.set_duration(5.0f);
    auto& track = timeline.add_track(1, prop::Position);
    track.add_key({1.0f, glm::vec2{0.0f, 0.0f}, Easing::Linear});

    TimelineView view(&timeline);
    view.set_pixels_per_second(100.0f);
    view.arrange({{0.0f, 0.0f}, {800.0f, 300.0f}});

    assert(view.on_mouse_down({268.0f, 52.0f}, 0));
    // 拖到 2.03s（x = 168 + 203 = 371）→ 吸附到 2.0s。
    assert(view.on_mouse_move({371.0f, 52.0f}));
    assert(std::fabs(track.keys()[0].time - 2.0f) < kEps);
}
} // namespace

// 编辑后存盘读回：时间轴内容一致。
void test_edit_then_round_trip()
{
    Project project;
    project.timeline.set_duration(4.0f);
    auto& track = project.timeline.add_track(1, prop::Position);
    track.add_key({1.0f, glm::vec2{0.0f, 0.0f}, Easing::Linear});

    TimelineView view(&project.timeline);
    view.set_pixels_per_second(100.0f);
    view.arrange({{0.0f, 0.0f}, {800.0f, 300.0f}});

    // 把 1s 的关键帧拖到 2s。
    assert(view.on_mouse_down({268.0f, 52.0f}, 0));
    assert(view.on_mouse_move({368.0f, 52.0f}));
    view.on_mouse_up({368.0f, 52.0f}, 0);
    assert(std::fabs(project.timeline.tracks()[0].keys()[0].time - 2.0f) < kEps);

    const std::vector<std::uint8_t> bytes = serialize(project);
    Project restored;
    std::string error;
    assert(deserialize(bytes, restored, &error));

    assert(restored.timeline.tracks().size() == 1);
    const auto& keys = restored.timeline.tracks()[0].keys();
    assert(keys.size() == 1);
    assert(std::fabs(keys[0].time - 2.0f) < kEps);
    assert(restored.timeline.tracks()[0].node() == 1);
    assert(restored.timeline.tracks()[0].property() == prop::Position);
}

int main()
{
    test_set_key_time_keeps_order();
    test_click_ruler_seeks();
    test_drag_playhead();
    test_drag_keyframe();
    test_click_label_column_ignored();
    test_right_click_removes_key();
    test_double_click_cycles_easing();
    test_drag_snaps_to_second();
    test_box_select_and_move();
    test_edit_then_round_trip();
    std::printf("test_ui_timeline_view: all passed\n");
    return 0;
}
