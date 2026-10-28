#pragma once

#include "style.hpp"
#include "theme.hpp"
#include "widget.hpp"
#include "../graphics/box.hpp"
#include "../graphics/rectangle.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace xel::ui
{
// 自建文件浏览器（M6.5）：不依赖系统对话框。
// 只负责「列目录 + 选文件」，路径输入与确认按钮由外层对话框承担。
//
// 行为约定：
//   - 目录排在文件前，各自按名字升序，非根目录第一项恒为 ".."；
//   - 扩展名过滤只作用于文件，目录永远可见；
//   - 双击目录进入，双击文件触发 on_activate（外层据此确认选择）。
class FileBrowser : public Widget
{
public:
    explicit FileBrowser(glm::vec2 size = {560.0f, 260.0f})
    {
        set_size(size);
        set_preferred_height(size.y);
    }

    void set_directory(std::string path)
    {
        directory_ = std::move(path);
        selected_ = -1;
        scroll_ = 0.0f;
        refresh();
    }
    const std::string& directory() const { return directory_; }

    void set_extension_filter(std::string extension)
    {
        extension_ = std::move(extension);
        refresh();
    }
    const std::string& extension_filter() const { return extension_; }

    void set_row_height(float height)
    {
        row_height_ = std::max(18.0f, height);
        mark_layout_dirty();
    }
    float row_height() const { return row_height_; }

    void set_preferred_height(float height)
    {
        preferred_height_ = std::max(row_height_, height);
        mark_layout_dirty();
    }

    // 选中的文件名；未选中返回空串。目录也算选中（".." 除外）。
    const std::string& selected_name() const
    {
        static const std::string empty;
        if (selected_ < 0 || selected_ >= static_cast<int>(entries_.size())) return empty;
        return entries_[static_cast<std::size_t>(selected_)].name;
    }

    bool selected_is_directory() const
    {
        if (selected_ < 0 || selected_ >= static_cast<int>(entries_.size())) return false;
        return entries_[static_cast<std::size_t>(selected_)].is_directory;
    }

    // 选中项的绝对路径（".." 返回上级目录）。
    std::string selected_path() const
    {
        const std::string& name = selected_name();
        if (name.empty()) return {};
        if (name == "..") return std::filesystem::path(directory_).parent_path().string();
        return (std::filesystem::path(directory_) / name).string();
    }

    // 当前目录下的完整路径（文件名来自外部输入框）。
    std::string resolve(const std::string& name) const
    {
        if (name.empty()) return {};
        const std::filesystem::path candidate(name);
        if (candidate.is_absolute()) return candidate.string();
        return (std::filesystem::path(directory_) / candidate).string();
    }

    std::size_t entry_count() const { return entries_.size(); }
    // 供测试与展示：第 index 项名字（越界返回空串）。
    std::string entry_name(std::size_t index) const
    {
        return index < entries_.size() ? entries_[index].name : std::string{};
    }
    bool entry_is_directory(std::size_t index) const
    {
        return index < entries_.size() && entries_[index].is_directory;
    }

    void set_on_activate(std::function<void()> handler) { on_activate_ = std::move(handler); }
    // 单击选中（用于把名字同步到输入框），双击才是 activate。
    void set_on_select(std::function<void()> handler) { on_select_ = std::move(handler); }
    void set_on_directory_changed(std::function<void()> handler)
    {
        on_directory_changed_ = std::move(handler);
    }

    // 重新枚举当前目录。目录不存在时列出空表（保持当前路径，便于用户改路径）。
    void refresh()
    {
        entries_.clear();
        selected_ = -1;
        scroll_ = 0.0f;

        namespace fs = std::filesystem;
        std::error_code ec;
        const fs::path dir(directory_);
        if (directory_.empty() || !fs::is_directory(dir, ec)) {
            mark_layout_dirty();
            return;
        }

        if (dir.has_parent_path() && dir.parent_path() != dir) {
            entries_.push_back(Entry{"..", true});
        }

        std::vector<Entry> dirs;
        std::vector<Entry> files;
        for (const auto& item : fs::directory_iterator(dir, ec)) {
            const std::string name = item.path().filename().string();
            if (name.empty()) continue;
            std::error_code entry_ec;
            if (item.is_directory(entry_ec)) {
                dirs.push_back(Entry{name, true});
            } else if (extension_.empty() || item.path().extension().string() == extension_) {
                files.push_back(Entry{name, false});
            }
        }

        auto by_name = [](const Entry& a, const Entry& b) { return a.name < b.name; };
        std::sort(dirs.begin(), dirs.end(), by_name);
        std::sort(files.begin(), files.end(), by_name);
        entries_.insert(entries_.end(), dirs.begin(), dirs.end());
        entries_.insert(entries_.end(), files.begin(), files.end());
        mark_layout_dirty();
    }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        const float width = std::isfinite(constraints.x) && constraints.x > 0.0f ? constraints.x
                                                                                : 560.0f;
        return clamp_size({width, preferred_height_});
    }

    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
        clamp_scroll();
    }

    bool on_mouse_down(glm::vec2 pos, int button) override
    {
        if (button != 0) return false;
        const int row = row_at(pos);
        if (row < 0) return false;
        selected_ = row;
        mark_layout_dirty();
        if (on_select_) on_select_();
        return true;
    }

    bool on_double_click(glm::vec2 pos, int button) override
    {
        if (button != 0) return false;
        const int row = row_at(pos);
        if (row < 0) return false;
        selected_ = row;
        activate(row);
        return true;
    }

    bool on_scroll(glm::vec2, glm::vec2 delta) override
    {
        const float content = static_cast<float>(entries_.size()) * row_height_;
        if (content <= global_.size.y) return false;
        scroll_ -= delta.y * row_height_ * 0.75f;
        clamp_scroll();
        mark_layout_dirty();
        return true;
    }

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();
        const glm::vec4 bounds{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y};
        const BoxStyle track = t.style(StyleRole::Track, ControlState::Normal);
        graphics::append_rounded_rect(list, bounds, corner_radius(track.radius), track.fill);
        if (track.border_width > 0.0f) {
            graphics::append_rounded_ring(list, bounds, corner_radius(track.radius),
                                          track.border_width, track.border);
        }

        list.push_clip(bounds);
        auto* atlas = font_atlas();

        const int first = std::max(0, static_cast<int>(scroll_ / row_height_));
        const int visible = static_cast<int>(global_.size.y / row_height_) + 1;
        for (int i = first; i < first + visible && i < static_cast<int>(entries_.size()); ++i) {
            const float y = global_.pos.y + static_cast<float>(i) * row_height_ - scroll_;
            if (y + row_height_ < global_.pos.y || y > global_.pos.y + global_.size.y) continue;

            if (i == selected_) {
                graphics::Rectangle{global_.pos.x, y, global_.size.x, row_height_,
                                    glm::vec4{t.accent.r, t.accent.g, t.accent.b, 0.22f}}
                    .append_to(list);
            }

            if (atlas == nullptr) continue;
            const Entry& entry = entries_[static_cast<std::size_t>(i)];
            const std::string label = entry.is_directory ? (entry.name + "/") : entry.name;
            atlas->append_text(list, global_.pos.x + 10.0f, y + (row_height_ - t.small_text_size) * 0.5f,
                               t.small_text_size, label,
                               entry.is_directory ? t.accent : t.text);
        }

        // 内容超出时在右下角提示还有多少项，避免「以为到底了」。
        if (atlas != nullptr) {
            const int total = static_cast<int>(entries_.size());
            const int shown = std::min(total, first + visible);
            if (total > shown) {
                const std::string more = "… +" + std::to_string(total - shown);
                const float width = atlas->measure_width(more, t.small_text_size);
                atlas->append_text(list, global_.pos.x + global_.size.x - width - 10.0f,
                                   global_.pos.y + global_.size.y - t.small_text_size - 6.0f,
                                   t.small_text_size, more, t.text);
            }
        }

        list.pop_clip();
    }

private:
    struct Entry
    {
        std::string name;
        bool is_directory = false;
    };

    void clamp_scroll()
    {
        const float content = static_cast<float>(entries_.size()) * row_height_;
        const float max_scroll = std::max(0.0f, content - global_.size.y);
        scroll_ = std::clamp(scroll_, 0.0f, max_scroll);
    }

    int row_at(glm::vec2 pos) const
    {
        if (!global_.contains(pos)) return -1;
        const float y = pos.y - global_.pos.y + scroll_;
        if (y < 0.0f) return -1;
        const int index = static_cast<int>(y / row_height_);
        return index < static_cast<int>(entries_.size()) ? index : -1;
    }

    void activate(int index)
    {
        if (index < 0 || index >= static_cast<int>(entries_.size())) return;
        const Entry& entry = entries_[static_cast<std::size_t>(index)];
        if (!entry.is_directory) {
            if (on_activate_) on_activate_();
            return;
        }

        namespace fs = std::filesystem;
        fs::path next = entry.name == ".." ? fs::path(directory_).parent_path()
                                           : fs::path(directory_) / entry.name;
        directory_ = next.string();
        refresh();
        if (on_directory_changed_) on_directory_changed_();
    }

    std::string directory_;
    std::string extension_;
    std::vector<Entry> entries_;
    int selected_ = -1;
    float scroll_ = 0.0f;
    float row_height_ = 28.0f;
    float preferred_height_ = 260.0f;
    std::function<void()> on_activate_;
    std::function<void()> on_select_;
    std::function<void()> on_directory_changed_;
};
} // namespace xel::ui
