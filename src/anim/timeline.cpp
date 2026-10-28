#include "timeline.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace xel::anim
{
PropertyValue lerp_property(const PropertyValue& a, const PropertyValue& b, float t)
{
    const float u = std::clamp(t, 0.0f, 1.0f);

    if (const auto* pa = std::get_if<float>(&a)) {
        if (const auto* pb = std::get_if<float>(&b)) return glm::mix(*pa, *pb, u);
    }
    if (const auto* pa = std::get_if<glm::vec2>(&a)) {
        if (const auto* pb = std::get_if<glm::vec2>(&b)) return glm::mix(*pa, *pb, u);
    }
    if (const auto* pa = std::get_if<glm::vec3>(&a)) {
        if (const auto* pb = std::get_if<glm::vec3>(&b)) return glm::mix(*pa, *pb, u);
    }
    if (const auto* pa = std::get_if<glm::vec4>(&a)) {
        if (const auto* pb = std::get_if<glm::vec4>(&b)) return glm::mix(*pa, *pb, u);
    }

    // bool / string / uint32：阶跃，到终点才切换。
    return u < 1.0f ? a : b;
}

void Track::add_key(Keyframe key)
{
    const auto it = std::upper_bound(keys_.begin(), keys_.end(), key.time,
                                     [](float t, const Keyframe& k) { return t < k.time; });
    keys_.insert(it, std::move(key));
}

bool Track::remove_key(float time)
{
    const auto it = std::find_if(keys_.begin(), keys_.end(),
                                 [time](const Keyframe& k) { return k.time == time; });
    if (it == keys_.end()) return false;
    keys_.erase(it);
    return true;
}

bool Track::set_key_time(float old_time, float new_time)
{
    const auto it = std::find_if(keys_.begin(), keys_.end(),
                                 [old_time](const Keyframe& k) { return k.time == old_time; });
    if (it == keys_.end()) return false;

    Keyframe key = std::move(*it);
    keys_.erase(it);
    key.time = new_time;
    add_key(std::move(key)); // 重新插入以保持有序
    return true;
}

Keyframe* Track::find_key(float time)
{
    for (Keyframe& key : keys_) {
        if (key.time == time) return &key;
    }
    return nullptr;
}

PropertyValue Track::sample(float time) const
{
    if (keys_.empty()) return PropertyValue{0.0f};
    if (keys_.size() == 1 || time <= keys_.front().time) return keys_.front().value;
    if (time >= keys_.back().time) return keys_.back().value;

    const auto it = std::upper_bound(keys_.begin(), keys_.end(), time,
                                     [](float t, const Keyframe& k) { return t < k.time; });
    const Keyframe& b = *it;
    const Keyframe& a = *(it - 1);

    const float span = b.time - a.time;
    if (span <= 0.0f) return b.value;

    const float local = (time - a.time) / span;
    const float e = ease(b.easing, local, b.custom);
    return lerp_property(a.value, b.value, e);
}

void Timeline::set_duration(float seconds)
{
    duration_ = std::max(0.0f, seconds);
    seek(time_);
}

void Timeline::set_fps(float fps)
{
    fps_ = std::max(0.0f, fps);
}

void Timeline::seek(float seconds)
{
    if (duration_ <= 0.0f) {
        time_ = 0.0f;
        return;
    }
    if (loop_) {
        time_ = std::fmod(seconds, duration_);
        if (time_ < 0.0f) time_ += duration_;
    } else {
        time_ = std::clamp(seconds, 0.0f, duration_);
    }
}

void Timeline::advance(float dt)
{
    if (!playing_ || duration_ <= 0.0f) return;

    const float next = time_ + dt * time_scale_;
    if (loop_) {
        time_ = std::fmod(next, duration_);
        if (time_ < 0.0f) time_ += duration_;
    } else {
        time_ = std::clamp(next, 0.0f, duration_);
        if (time_ >= duration_) playing_ = false;
    }
}

Track& Timeline::add_track(scene::NodeId node, PropertyId prop)
{
    tracks_.emplace_back(node, prop);
    return tracks_.back();
}

Track& Timeline::ensure_track(scene::NodeId node, PropertyId prop)
{
    for (Track& track : tracks_) {
        if (track.node() == node && track.property() == prop) return track;
    }
    return add_track(node, prop);
}

bool Timeline::remove_track(scene::NodeId node, PropertyId prop)
{
    const auto it = std::find_if(tracks_.begin(), tracks_.end(), [node, prop](const Track& track) {
        return track.node() == node && track.property() == prop;
    });
    if (it == tracks_.end()) return false;
    tracks_.erase(it);
    return true;
}

Track* Timeline::find_track(scene::NodeId node, PropertyId prop)
{
    for (Track& track : tracks_) {
        if (track.node() == node && track.property() == prop) return &track;
    }
    return nullptr;
}

const Track* Timeline::find_track(scene::NodeId node, PropertyId prop) const
{
    for (const Track& track : tracks_) {
        if (track.node() == node && track.property() == prop) return &track;
    }
    return nullptr;
}

void Timeline::apply(scene::Scene& scene, float time) const
{
    for (const auto& track : tracks_) {
        scene.set_property(track.node(), track.property(), track.sample(time));
    }
}
} // namespace xel::anim
