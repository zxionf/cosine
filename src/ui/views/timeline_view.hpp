#pragma once

#include "ui/theme.hpp"
#include "ui/style.hpp"
#include "ui/widget.hpp"
#include "anim/timeline.hpp"
#include "events/event.hpp"
#include "graphics/box.hpp"
#include "graphics/rectangle.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <functional>
#include <string>
#include <vector>

namespace xel::ui
{
// 拖动结束上报的一条关键帧位移。一次拖动可能带多个关键帧，
// 编辑器据此合成一条复合命令（整组可撤销）。
struct KeyMove
{
    int track = -1;
    float from = 0.0f;
    float to = 0.0f;
};

// 时间轴编辑视图（M6.3）：标尺 + 轨道行 + 关键帧菱形 + 播放头。
// 只依赖 anim::Timeline；点击标尺 seek，拖动关键帧改时间。
class TimelineView : public Widget
{
public:
    explicit TimelineView(anim::Timeline* timeline = nullptr) : timeline_(timeline) {}

    void set_timeline(anim::Timeline* timeline)
    {
        timeline_ = timeline;
        mark_layout_dirty();
    }
    anim::Timeline* timeline() const { return timeline_; }

    void set_time(float seconds) { playhead_ = std::max(0.0f, seconds); }
    float time() const { return playhead_; }

    void set_pixels_per_second(float pixels) { pps_ = std::max(10.0f, pixels); }
    float pixels_per_second() const { return pps_; }

    int selected_track() const { return selected_track_; }
    float selected_time() const { return selected_time_; }
    std::size_t selection_count() const { return selection_.size(); }

    // 场景树选中的节点：其轨道行高亮，便于两侧对照。
    void set_selected_node(scene::NodeId node) { selected_node_ = node; }
    scene::NodeId selected_node() const { return selected_node_; }

    void set_on_seek(std::function<void(float)> handler) { on_seek_ = std::move(handler); }
    void set_on_key_moved(std::function<void()> handler) { on_key_moved_ = std::move(handler); }
    void set_on_changed(std::function<void()> handler) { on_changed_ = std::move(handler); }
    // 编辑上报：由编辑器合成撤销命令（拖动结束 / 删除 / 切插值）。
    // 拖动上报是批量形式：多选拖动只产生一条命令。
    void set_on_move_end(std::function<void(const std::vector<KeyMove>&)> handler)
    {
        move_end_ = std::move(handler);
    }
    void set_on_remove_key(std::function<void(int, float)> handler)
    {
        remove_key_ = std::move(handler);
    }
    void set_on_cycle_easing(
        std::function<void(int, float, anim::Easing, anim::Easing)> handler)
    {
        cycle_easing_ = std::move(handler);
    }

    glm::vec2 measure(glm::vec2) override { return clamp_size(local_bounds().size); }

    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
    }

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();
        const glm::vec4 bounds{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y};

        const BoxStyle surface = t.style(StyleRole::Surface, ControlState::Normal);
        graphics::append_rounded_rect(list, bounds, corner_radius(surface.radius), surface.fill);
        if (surface.border_width > 0.0f) {
            graphics::append_rounded_ring(list, bounds, corner_radius(surface.radius),
                                          surface.border_width, surface.border);
        }

        list.push_clip(bounds);

        const float ruler = ruler_height();
        const float left = track_left();
        const float area = track_area_width();
        const float duration = timeline_ != nullptr ? timeline_->duration() : 5.0f;
        const BoxStyle track_style = t.style(StyleRole::Track, ControlState::Normal);

        // 标尺条。
        graphics::append_rounded_rect(list, {left, global_.pos.y, area, ruler}, 3.0f,
                                      track_style.fill);

        auto* atlas = font_atlas();
        const float step = tick_step();
        for (float time = 0.0f; time <= duration + 1.0e-4f; time += step) {
            const float x = x_at_time(time);
            if (x < left - 0.5f || x > left + area + 0.5f) continue;

            graphics::Rectangle{x, global_.pos.y + ruler, 1.0f, global_.size.y - ruler,
                                track_style.border}
                .append_to(list);

            if (atlas != nullptr) {
                const std::string text = std::format("{:.1f}s", time);
                atlas->append_text(list, x + 4.0f, global_.pos.y + 6.0f, t.small_text_size, text,
                                   t.text);
            }
        }

        // 轨道行。
        if (timeline_ != nullptr) {
            const auto& tracks = timeline_->tracks();
            for (std::size_t i = 0; i < tracks.size(); ++i) {
                const float row_y =
                    global_.pos.y + ruler + static_cast<float>(i) * row_height();
                if (row_y > global_.pos.y + global_.size.y) break;

                const bool selected_by_node =
                    selected_node_ != scene::kInvalidNode && tracks[i].node() == selected_node_;
                const bool selected_row =
                    selected_by_node || static_cast<int>(i) == selected_track_;
                if (selected_row) {
                    graphics::Rectangle{global_.pos.x, row_y, global_.size.x, row_height(),
                                        glm::vec4{t.accent.r, t.accent.g, t.accent.b, 0.08f}}
                        .append_to(list);
                }

                if (atlas != nullptr) {
                    const std::string label = std::format("node {} · prop {}", tracks[i].node(),
                                                          tracks[i].property());
                    atlas->append_text(list, global_.pos.x + 8.0f, row_y + 6.0f,
                                       t.small_text_size, label, t.text);
                }

                for (const anim::Keyframe& key : tracks[i].keys()) {
                    const float x = x_at_time(key.time);
                    if (x < left - 8.0f || x > left + area + 8.0f) continue;
                    const bool selected = is_selected(static_cast<int>(i), key.time);
                    append_diamond(list, {x, row_y + row_height() * 0.5f}, 6.0f,
                                   selected ? t.accent_text : t.text);
                }
            }
        }

        // 播放头。
        const float head = x_at_time(playhead_);
        if (head >= left - 1.0f && head <= left + area + 1.0f) {
            graphics::Rectangle{head - 1.0f, global_.pos.y + ruler * 0.5f, 2.0f,
                                global_.size.y - ruler * 0.5f, t.accent}
                .append_to(list);
        }

        // 框选矩形。
        if (box_selecting_) {
            const float x0 = std::min(box_start_.x, box_end_.x);
            const float x1 = std::max(box_start_.x, box_end_.x);
            const float y0 = std::min(box_start_.y, box_end_.y);
            const float y1 = std::max(box_start_.y, box_end_.y);

            graphics::Rectangle{x0, y0, x1 - x0, y1 - y0,
                                glm::vec4{t.accent.r, t.accent.g, t.accent.b, 0.15f}}
                .append_to(list);
            graphics::Rectangle{x0, y0, x1 - x0, 1.0f, t.accent}.append_to(list);
            graphics::Rectangle{x0, y1 - 1.0f, x1 - x0, 1.0f, t.accent}.append_to(list);
            graphics::Rectangle{x0, y0, 1.0f, y1 - y0, t.accent}.append_to(list);
            graphics::Rectangle{x1 - 1.0f, y0, 1.0f, y1 - y0, t.accent}.append_to(list);
        }

        list.pop_clip();
    }

    bool focusable() const override { return true; }

    bool on_mouse_down(glm::vec2 pos, int button) override
    {
        if (button != 0) return false;
        if (pos.x < track_left()) return false;

        const bool extend = modifiers_extend();
        const KeyHit hit = hit_key(pos);
        if (hit.valid()) {
            if (extend) {
                // Shift/Ctrl：切换该关键帧的选中状态，不开始拖动。
                const auto it = std::find_if(selection_.begin(), selection_.end(),
                                             [&](const KeyRef& ref) {
                                                 return ref.track == hit.track &&
                                                        std::fabs(ref.time - hit.time) < 1.0e-4f;
                                             });
                if (it != selection_.end()) {
                    selection_.erase(it);
                } else {
                    selection_.push_back(KeyRef{hit.track, hit.time});
                }
                selected_track_ = hit.track;
                selected_time_ = hit.time;
                return true;
            }

            // 命中已选中的关键帧则保留多选，否则重置为单选。
            if (!is_selected(hit.track, hit.time)) {
                selection_.clear();
                selection_.push_back(KeyRef{hit.track, hit.time});
            }
            selected_track_ = hit.track;
            selected_time_ = hit.time;
            dragging_ = true;
            primary_origin_ = hit.time;
            drag_start_time_ = time_at_x(pos.x);
            capture_drag_origins();
            return true;
        }

        // 标尺区：拖播放头。
        if (pos.y < global_.pos.y + ruler_height()) {
            scrubbing_ = true;
            playhead_ = std::clamp(time_at_x(pos.x), 0.0f,
                                   timeline_ != nullptr ? timeline_->duration() : 5.0f);
            if (on_seek_) on_seek_(playhead_);
            return true;
        }

        // 轨道区空白：开始框选（Shift/Ctrl 保留已有选择，追加）。
        box_selecting_ = true;
        box_start_ = pos;
        box_end_ = pos;
        if (!extend) selection_.clear();
        selected_track_ = -1;
        return true;
    }

    bool on_mouse_move(glm::vec2 pos) override
    {
        // 拖播放头：连续 scrub。
        if (scrubbing_) {
            playhead_ = std::clamp(time_at_x(pos.x), 0.0f,
                                   timeline_ != nullptr ? timeline_->duration() : 5.0f);
            if (on_seek_) on_seek_(playhead_);
            return true;
        }

        // 框选：只更新选框。
        if (box_selecting_) {
            box_end_ = pos;
            return true;
        }

        if (!dragging_ || timeline_ == nullptr || drag_origins_.empty()) return false;

        const float duration = timeline_->duration();
        // 吸附作用在主关键帧上，其余按同一 delta 跟随。
        float delta = time_at_x(pos.x) - drag_start_time_;
        delta = snap_time(primary_origin_ + delta) - primary_origin_;

        auto& tracks = timeline_->tracks();

        // 防碰撞：向右拖时先移时间大的，向左拖时先移时间小的。
        std::vector<DragOrigin*> ordered;
        ordered.reserve(drag_origins_.size());
        for (DragOrigin& entry : drag_origins_) ordered.push_back(&entry);
        if (delta >= 0.0f) {
            std::sort(ordered.begin(), ordered.end(),
                      [](const DragOrigin* a, const DragOrigin* b) { return a->origin > b->origin; });
        } else {
            std::sort(ordered.begin(), ordered.end(),
                      [](const DragOrigin* a, const DragOrigin* b) { return a->origin < b->origin; });
        }

        for (DragOrigin* entry : ordered) {
            if (entry->track < 0 || entry->track >= static_cast<int>(tracks.size())) continue;
            const float next = std::clamp(entry->origin + delta, 0.0f, duration);
            anim::Track& track = tracks[static_cast<std::size_t>(entry->track)];
            if (track.set_key_time(entry->current, next)) entry->current = next;
        }
        // 同步选中状态：主关键帧时间 + 选择集里的时间（按 capture 顺序与 drag_origins 对应）。
        selected_time_ = std::clamp(primary_origin_ + delta, 0.0f, duration);
        for (std::size_t i = 0; i < selection_.size() && i < drag_origins_.size(); ++i) {
            selection_[i].time = drag_origins_[i].current;
        }

        if (on_key_moved_) on_key_moved_();
        return true;
    }

    bool on_mouse_up(glm::vec2, int) override
    {
        const bool handled = dragging_ || scrubbing_ || box_selecting_;
        if (box_selecting_) collect_box_selection();

        // 拖动结束：上报所有真正位移过的关键帧，由编辑器合成一条复合命令。
        if (dragging_ && move_end_) {
            std::vector<KeyMove> moves;
            for (const DragOrigin& entry : drag_origins_) {
                if (std::fabs(entry.current - entry.origin) > 1.0e-4f) {
                    moves.push_back(KeyMove{entry.track, entry.origin, entry.current});
                }
            }
            if (!moves.empty()) move_end_(moves);
        }

        dragging_ = false;
        scrubbing_ = false;
        box_selecting_ = false;
        drag_origins_.clear();
        return handled;
    }

    // 右键：上报删除，实际删除由编辑器走 RemoveKeyCommand（可撤销）。
    bool on_right_click(glm::vec2 pos) override
    {
        if (timeline_ == nullptr || pos.x < track_left()) return false;
        const KeyHit hit = hit_key(pos);
        if (!hit.valid()) return false;

        if (remove_key_) remove_key_(hit.track, hit.time);
        selected_track_ = -1;
        selection_.clear();
        return true;
    }

    // 双击：循环切换插值（Linear → EaseIn → EaseOut → EaseInOut → CubicBezier）。
    // 双击：上报插值切换（from → to），实际修改由编辑器走命令。
    bool on_double_click(glm::vec2 pos, int button) override
    {
        if (button != 0 || timeline_ == nullptr || pos.x < track_left()) return false;
        const KeyHit hit = hit_key(pos);
        if (!hit.valid()) return false;

        auto& tracks = timeline_->tracks();
        auto& track = tracks[static_cast<std::size_t>(hit.track)];

        const anim::Easing* current = nullptr;
        for (const anim::Keyframe& key : track.keys()) {
            if (key.time == hit.time) {
                current = &key.easing;
                break;
            }
        }
        if (current == nullptr) return false;

        const anim::Easing from = *current;
        const anim::Easing to = next_easing(from);
        if (cycle_easing_) cycle_easing_(hit.track, hit.time, from, to);
        return true;
    }

private:
    struct KeyHit
    {
        int track = -1;
        float time = 0.0f;
        bool valid() const { return track >= 0; }
    };

    // 选中的关键帧（轨道索引 + 时间）。
    struct KeyRef
    {
        int track = -1;
        float time = 0.0f;
    };

    // 拖动中的关键帧：origin 是拖动前时间，current 是当前位置（用于 set_key_time 定位）。
    struct DragOrigin
    {
        int track = -1;
        float origin = 0.0f;
        float current = 0.0f;
    };

    // 鼠标回调里拿不到 mods，改从共享上下文读最近一次事件的修饰键。
    bool modifiers_extend() const
    {
        const UIContext* ctx = context();
        if (ctx == nullptr) return false;
        return event::has_modifier(ctx->modifiers, event::Modifier::Shift) ||
               event::has_modifier(ctx->modifiers, event::Modifier::Control);
    }

    bool is_selected(int track, float time) const
    {
        for (const KeyRef& ref : selection_) {
            if (ref.track == track && std::fabs(ref.time - time) < 1.0e-4f) return true;
        }
        return false;
    }

    void capture_drag_origins()
    {
        drag_origins_.clear();
        if (timeline_ == nullptr) return;
        const auto& tracks = timeline_->tracks();
        for (const KeyRef& ref : selection_) {
            if (ref.track < 0 || ref.track >= static_cast<int>(tracks.size())) continue;
            drag_origins_.push_back(DragOrigin{ref.track, ref.time, ref.time});
        }
    }

    void collect_box_selection()
    {
        if (timeline_ == nullptr) return;

        const float x0 = std::min(box_start_.x, box_end_.x);
        const float x1 = std::max(box_start_.x, box_end_.x);
        const float y0 = std::min(box_start_.y, box_end_.y);
        const float y1 = std::max(box_start_.y, box_end_.y);

        const auto& tracks = timeline_->tracks();
        for (std::size_t i = 0; i < tracks.size(); ++i) {
            const float row_y = global_.pos.y + ruler_height() + static_cast<float>(i) * row_height();
            for (const anim::Keyframe& key : tracks[i].keys()) {
                const float x = x_at_time(key.time);
                const float y = row_y + row_height() * 0.5f;
                if (x >= x0 && x <= x1 && y >= y0 && y <= y1) {
                    const int track = static_cast<int>(i);
                    if (!is_selected(track, key.time)) {
                        selection_.push_back(KeyRef{track, key.time});
                    }
                }
            }
        }

        if (!selection_.empty()) {
            selected_track_ = selection_.front().track;
            selected_time_ = selection_.front().time;
        }
    }

    float ruler_height() const { return 34.0f; }
    float row_height() const { return 36.0f; }
    float label_width() const { return 160.0f; }
    float track_left() const { return global_.pos.x + label_width(); }
    float track_area_width() const { return std::max(10.0f, global_.size.x - label_width()); }

    // 轨道区左侧留 8px，避免 0s 的关键帧压住左侧标签。
    float x_at_time(float seconds) const { return track_left() + 8.0f + seconds * pps_; }
    float time_at_x(float x) const { return (x - track_left() - 8.0f) / pps_; }

    // 相邻刻度至少 64px。
    float tick_step() const
    {
        constexpr float kCandidates[] = {0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 30.0f};
        for (float step : kCandidates) {
            if (step * pps_ >= 64.0f) return step;
        }
        return 60.0f;
    }

    KeyHit hit_key(glm::vec2 pos) const
    {
        if (timeline_ == nullptr) return {};

        const auto& tracks = timeline_->tracks();
        for (std::size_t i = 0; i < tracks.size(); ++i) {
            const float row_y = global_.pos.y + ruler_height() + static_cast<float>(i) * row_height();
            if (pos.y < row_y || pos.y > row_y + row_height()) continue;

            for (const anim::Keyframe& key : tracks[i].keys()) {
                if (std::fabs(pos.x - x_at_time(key.time)) <= 8.0f) {
                    return {static_cast<int>(i), key.time};
                }
            }
        }
        return {};
    }

    static void append_diamond(graphics::DrawList& list, glm::vec2 center, float radius,
                              glm::vec4 color)
    {
        list.set_texture(graphics::TextureSlot::Texture);
        auto& batch = list.batch();
        const glm::vec2 uv = graphics::active_font_atlas() != nullptr
                                 ? graphics::active_font_atlas()->white_uv()
                                 : glm::vec2{0.0f};
        const glm::vec4 premultiplied{color.r * color.a, color.g * color.a, color.b * color.a,
                                      color.a};

        const uint32_t base = static_cast<uint32_t>(batch.vertices.size());
        batch.vertices.push_back({{center.x, center.y - radius}, premultiplied, uv});
        batch.vertices.push_back({{center.x + radius, center.y}, premultiplied, uv});
        batch.vertices.push_back({{center.x, center.y + radius}, premultiplied, uv});
        batch.vertices.push_back({{center.x - radius, center.y}, premultiplied, uv});
        batch.indices.insert(batch.indices.end(),
                             {base, base + 1, base + 2, base + 2, base + 3, base});
    }

    // 吸附：整秒与其它关键帧，阈值为 6px 对应的时间。
    float snap_time(float seconds) const
    {
        constexpr float kSnapPixels = 6.0f;
        const float tolerance = kSnapPixels / pps_;

        const float nearest_second = std::round(seconds);
        if (std::fabs(seconds - nearest_second) <= tolerance) return nearest_second;

        if (timeline_ != nullptr) {
            for (const anim::Track& track : timeline_->tracks()) {
                for (const anim::Keyframe& key : track.keys()) {
                    if (std::fabs(seconds - key.time) <= tolerance) return key.time;
                }
            }
        }
        return seconds;
    }

    static anim::Easing next_easing(anim::Easing easing)
    {
        switch (easing) {
            case anim::Easing::Linear: return anim::Easing::EaseIn;
            case anim::Easing::EaseIn: return anim::Easing::EaseOut;
            case anim::Easing::EaseOut: return anim::Easing::EaseInOut;
            case anim::Easing::EaseInOut: return anim::Easing::CubicBezier;
            case anim::Easing::CubicBezier: return anim::Easing::Linear;
        }
        return anim::Easing::Linear;
    }

    anim::Timeline* timeline_ = nullptr;
    float playhead_ = 0.0f;
    float pps_ = 120.0f;
    int selected_track_ = -1;
    float selected_time_ = 0.0f;
    scene::NodeId selected_node_ = scene::kInvalidNode;
    bool dragging_ = false;
    bool scrubbing_ = false;
    bool box_selecting_ = false;
    glm::vec2 box_start_{0.0f, 0.0f};
    glm::vec2 box_end_{0.0f, 0.0f};
    float primary_origin_ = 0.0f;
    float drag_start_time_ = 0.0f;
    std::vector<KeyRef> selection_;
    std::vector<DragOrigin> drag_origins_;
    std::function<void(float)> on_seek_;
    std::function<void()> on_key_moved_;
    std::function<void()> on_changed_;
    std::function<void(const std::vector<KeyMove>&)> move_end_;
    std::function<void(int, float)> remove_key_;
    std::function<void(int, float, anim::Easing, anim::Easing)> cycle_easing_;
};
} // namespace xel::ui
