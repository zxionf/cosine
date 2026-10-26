#include "player.hpp"

namespace xel::anim
{
void Player::tick(float dt)
{
    if (!timeline_.playing()) return;

    const float before = timeline_.time();
    timeline_.advance(dt);
    const float after = timeline_.time();

    fire_events(before, after);
    timeline_.apply(scene_);
}

void Player::seek(float time)
{
    timeline_.seek(time);
    timeline_.apply(scene_);
}

void Player::apply()
{
    timeline_.apply(scene_);
}

void Player::add_event(float time, Callback callback)
{
    events_.push_back({time, std::move(callback)});
}

void Player::fire_events(float before, float after)
{
    if (events_.empty()) return;

    // 循环回绕：时间从 before 跳到 duration 再回到 after。
    const bool wrapped = after < before;

    for (auto& event : events_) {
        // 用 (before, after] 半开区间，避免起点重复触发。
        const bool crossed = wrapped
            ? (event.time > before || event.time <= after)
            : (event.time > before && event.time <= after);

        if (crossed && event.callback) event.callback();
    }
}
} // namespace xel::anim
