#pragma once

#include "easing.hpp"
#include "property.hpp"
#include "scene.hpp"

#include <vector>

namespace xel::anim
{
struct Keyframe
{
    float time = 0.0f;
    PropertyValue value;
    Easing easing = Easing::Linear;
    CubicBezier custom{}; // 仅 Easing::CubicBezier 使用
};

// 同类型线性插值；bool / string / uint32 为阶跃（t < 1 取 a，否则取 b）。
PropertyValue lerp_property(const PropertyValue& a, const PropertyValue& b, float t);

// 单条属性轨道：关键帧按时间排序，采样时在相邻帧之间按后一帧的 easing 插值。
class Track
{
public:
    Track(scene::NodeId node, PropertyId prop) : node_(node), property_(prop) {}

    scene::NodeId node() const { return node_; }
    PropertyId property() const { return property_; }

    void add_key(Keyframe key);
    // 删除 time 精确匹配的关键帧；返回是否删除。
    bool remove_key(float time);
    // 修改关键帧时间并保持有序；返回是否找到。
    bool set_key_time(float old_time, float new_time);
    const std::vector<Keyframe>& keys() const { return keys_; }

    // 两端外推取端点；单帧轨道恒定。
    PropertyValue sample(float time) const;

private:
    scene::NodeId node_ = scene::kInvalidNode;
    PropertyId property_ = kInvalidProperty;
    std::vector<Keyframe> keys_;
};

class Timeline
{
public:
    void set_duration(float seconds);
    float duration() const { return duration_; }
    void set_fps(float fps);
    float fps() const { return fps_; }
    void set_loop(bool loop) { loop_ = loop; }
    bool loop() const { return loop_; }
    void set_time_scale(float scale) { time_scale_ = scale; }
    float time_scale() const { return time_scale_; }

    void play() { playing_ = true; }
    void pause() { playing_ = false; }
    bool playing() const { return playing_; }

    float time() const { return time_; }
    // 确定性跳转：循环模式取模到 [0, duration)，否则 clamp 到 [0, duration]。
    void seek(float seconds);

    // 推进时间；非循环模式到达末尾时停在 duration 并暂停。
    void advance(float dt);

    Track& add_track(scene::NodeId node, PropertyId prop);
    // 存在 (node, prop) 则复用，否则新建。编辑器与命令层使用。
    Track& ensure_track(scene::NodeId node, PropertyId prop);
    // 删除 (node, prop) 轨道；返回是否存在。
    bool remove_track(scene::NodeId node, PropertyId prop);
    // 查找轨道指针；不存在返回 nullptr。
    Track* find_track(scene::NodeId node, PropertyId prop);
    const Track* find_track(scene::NodeId node, PropertyId prop) const;
    const std::vector<Track>& tracks() const { return tracks_; }
    // 非 const：时间轴视图需要移动关键帧。
    std::vector<Track>& tracks() { return tracks_; }

    void apply(scene::Scene& scene) const { apply(scene, time_); }
    void apply(scene::Scene& scene, float time) const;

private:
    float duration_ = 5.0f;
    float fps_ = 60.0f;
    bool loop_ = true;
    float time_scale_ = 1.0f;
    bool playing_ = false;
    float time_ = 0.0f;
    std::vector<Track> tracks_;
};
} // namespace xel::anim
