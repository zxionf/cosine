#pragma once

#include "ui/views/table.hpp"
#include "ui/theme.hpp"
#include "ui/widget.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace xel::ui
{
inline std::string format_bytes(std::uintmax_t bytes)
{
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    if (unit == 0) return std::to_string(bytes) + " B";
    return std::format("{:.1f} {}", value, units[unit]);
}

inline std::string ascii_lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

// 文件浏览器：目录列举 + 扩展名/名字过滤 + 选中/双击，渲染、滚动、行选中全部交给 Table。
class FileBrowser : public Widget
{
public:
    explicit FileBrowser(glm::vec2 size = {560.0f, 372.0f})
    {
        set_size(size);
        set_min_size(size);
        set_max_size(size);
        preferred_height_ = size.y;

        auto table = std::make_unique<Table>(size.x, size.y);
        table->set_text_size(18.0f);
        table->set_row_height(32.0f);
        // FileBrowser 是纯列表：不要表头，否则行位置整体下移（
        // 列宽仍然要定义，用来把尺寸列右对齐）。
        table->set_header_height(0.0f);
        columns_ = {{"Name", 0.0f}, {"Size", 88.0f, TableColumn::Align::Right}};
        table->set_columns(columns_);
        table->set_on_select([this](int index) {
            selected_ = index;
            if (on_select_) on_select_();
        });
        table_ = table.get();
        adopt(std::move(table));
    }

    void set_directory(std::string path)
    {
        directory_ = std::move(path);
        selected_ = -1;
        refresh();
    }
    const std::string& directory() const { return directory_; }

    void set_extension_filter(std::string extension)
    {
        extension_ = std::move(extension);
        refresh();
    }
    const std::string& extension_filter() const { return extension_; }

    // 名字子串过滤：大小写不敏感，与扩展名过滤叠加；“..” 始终保留用于返回上级。
    void set_name_filter(std::string filter)
    {
        name_filter_ = std::move(filter);
        refresh();
    }
    const std::string& name_filter() const { return name_filter_; }

    void set_row_height(float height)
    {
        if (table_ != nullptr) table_->set_row_height(height);
        mark_layout_dirty();
    }
    float row_height() const { return table_ != nullptr ? table_->row_height() : 32.0f; }

    // 表头：默认 0（纯列表）。开启后行绘制/命中/滚动都会扣除表头高度。
    void set_header_height(float height)
    {
        if (table_ != nullptr) table_->set_header_height(height);
        mark_layout_dirty();
    }
    float header_height() const { return table_ != nullptr ? table_->header_height() : 0.0f; }

    // 列定义：FileBrowser 的行数据固定为 Name/Size 两列，这里只调整宽度与对齐。
    void set_columns(std::vector<TableColumn> columns)
    {
        columns_ = std::move(columns);
        if (table_ != nullptr) table_->set_columns(columns_);
    }
    const std::vector<TableColumn>& columns() const { return columns_; }

    // 弹性宽度：默认关闭（固定尺寸，保持 cosine 对话框行为）。
    // 开启后 measure 返回 min_size 作为「自然宽度」，高度用 preferred_height_；
    // 父容器（Hbox）用 set_flex_grow(1) 把剩余宽度分配进来。
    void set_auto_width(bool on)
    {
        auto_width_ = on;
        mark_layout_dirty();
    }
    bool auto_width() const { return auto_width_; }

    void set_preferred_height(float height)
    {
        preferred_height_ = std::max(row_height(), height);
        mark_layout_dirty();
    }

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

    std::string selected_path() const
    {
        const std::string& name = selected_name();
        if (name.empty()) return {};
        return resolve(name);
    }

    std::string resolve(const std::string& name) const
    {
        if (name.empty()) return {};
        const std::filesystem::path candidate(name);
        if (candidate.is_absolute()) return candidate.lexically_normal().string();
        return (std::filesystem::path(directory_) / candidate).lexically_normal().string();
    }

    std::size_t entry_count() const { return entries_.size(); }

    std::string entry_name(std::size_t index) const
    {
        if (index >= entries_.size()) return {};
        return entries_[index].name;
    }

    bool entry_is_directory(std::size_t index) const
    {
        return index < entries_.size() && entries_[index].is_directory;
    }

    std::uintmax_t entry_size(std::size_t index) const
    {
        return index < entries_.size() ? entries_[index].size : 0;
    }

    void set_on_activate(std::function<void()> handler) { on_activate_ = std::move(handler); }
    void set_on_select(std::function<void()> handler) { on_select_ = std::move(handler); }
    void set_on_directory_changed(std::function<void()> handler)
    {
        on_directory_changed_ = std::move(handler);
    }

    void select_index(int index)
    {
        if (entries_.empty()) {
            selected_ = -1;
            return;
        }
        const int clamped = std::clamp(index, 0, static_cast<int>(entries_.size()) - 1);
        selected_ = clamped;
        if (table_ != nullptr) table_->set_selected(clamped, false);
    }
    int selected_index() const { return selected_; }

    // 一屏可见行数（已扣除表头），用于 PageUp/PageDown。
    int page_rows() const
    {
        const float viewport = std::max(0.0f, height() - header_height());
        return std::max(1, static_cast<int>(viewport / std::max(1.0f, row_height())));
    }

    bool focusable() const override { return true; }

    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        if (entries_.empty()) return false;
        switch (key) {
        case event::KeyCode::Up:
            select_index(selected_ - 1);
            if (on_select_) on_select_();
            return true;
        case event::KeyCode::Down:
            select_index(selected_ + 1);
            if (on_select_) on_select_();
            return true;
        case event::KeyCode::Home:
            select_index(0);
            if (on_select_) on_select_();
            return true;
        case event::KeyCode::End:
            select_index(static_cast<int>(entries_.size()) - 1);
            if (on_select_) on_select_();
            return true;
        case event::KeyCode::PageUp:
            select_index(selected_ - page_rows());
            if (on_select_) on_select_();
            return true;
        case event::KeyCode::PageDown:
            select_index(selected_ + page_rows());
            if (on_select_) on_select_();
            return true;
        case event::KeyCode::Enter:
            return activate();
        case event::KeyCode::Backspace: {
            const std::filesystem::path current(directory_);
            if (!current.has_parent_path() || current.parent_path() == current) return true;
            set_directory(current.parent_path().string());
            if (on_directory_changed_) on_directory_changed_();
            return true;
        }
        default:
            return false;
        }
    }

    void refresh()
    {
        // 过滤/刷新不主动丢选中：同名同类型仍存在时恢复；set_directory 会先清空。
        const std::string keep_name = selected_name();
        const bool keep_is_dir = selected_is_directory();
        entries_.clear();
        std::error_code ec;
        const std::filesystem::path base(directory_);

        if (base.has_parent_path() && base.parent_path() != base) {
            entries_.push_back(Entry{"..", true, 0});
        }

        std::vector<Entry> dirs;
        std::vector<Entry> files;
        const std::string needle = ascii_lower(name_filter_);
        for (auto it = std::filesystem::directory_iterator(base, ec);
             !ec && it != std::filesystem::directory_iterator(); ++it) {
            const std::string name = it->path().filename().string();
            if (name.empty() || name.front() == '.') continue;
            if (!needle.empty() && ascii_lower(name).find(needle) == std::string::npos) continue;

            std::error_code entry_ec;
            const bool is_directory = it->is_directory(entry_ec);
            if (!is_directory && !extension_.empty()) {
                const std::string ext = it->path().extension().string();
                if (ext != extension_) continue;
            }
            std::uintmax_t size = 0;
            if (!is_directory) {
                std::error_code size_ec;
                size = it->file_size(size_ec);
            }
            (is_directory ? dirs : files).push_back(Entry{name, is_directory, size});
        }

        const auto by_name = [](const Entry& a, const Entry& b) { return a.name < b.name; };
        std::sort(dirs.begin(), dirs.end(), by_name);
        std::sort(files.begin(), files.end(), by_name);
        entries_.insert(entries_.end(), dirs.begin(), dirs.end());
        entries_.insert(entries_.end(), files.begin(), files.end());

        selected_ = -1;
        if (!keep_name.empty()) {
            for (std::size_t i = 0; i < entries_.size(); ++i) {
                if (entries_[i].name == keep_name && entries_[i].is_directory == keep_is_dir) {
                    selected_ = static_cast<int>(i);
                    break;
                }
            }
        }
        sync_table();
    }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        if (!auto_width_) return clamp_size(min_size());
        (void)constraints;
        return clamp_size({min_size().x, preferred_height_});
    }
    bool fixed_size() const override { return !auto_width_; }

    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
        if (table_ != nullptr) table_->arrange(slot);
    }

    bool on_mouse_down(glm::vec2 pos, int button) override
    {
        if (button != 0) return false;
        const int index = index_at(pos);
        if (index < 0) return false;
        select_index(index);
        if (on_select_) on_select_();
        return true;
    }

    bool on_double_click(glm::vec2 pos, int button) override
    {
        (void)button;
        const int index = index_at(pos);
        if (index < 0) return false;
        select_index(index);
        return activate();
    }

    bool on_scroll(glm::vec2, glm::vec2 delta) override
    {
        if (table_ == nullptr) return false;
        // 内容不够高就不消费滚轮，让外层容器接手。
        const float viewport = std::max(0.0f, global_.size.y - header_height());
        const float content = static_cast<float>(entries_.size()) * row_height();
        if (content <= viewport) return false;
        table_->set_scroll_value(table_->scroll_offset() - delta.y * row_height() * 0.75f);
        return true;
    }

    void append_to(graphics::DrawList& list) const override
    {
        if (table_ != nullptr) table_->append_to(list);
    }

    bool hit_testable() const override { return true; }


private:
    struct Entry
    {
        std::string name;
        bool is_directory = false;
        std::uintmax_t size = 0;
    };

    void sync_table()
    {
        if (table_ == nullptr) return;
        std::vector<std::vector<std::string>> rows;
        rows.reserve(entries_.size());
        for (const Entry& entry : entries_) {
            rows.push_back({entry.is_directory ? entry.name + "/" : entry.name,
                            entry.is_directory ? std::string{"-"} : format_bytes(entry.size)});
        }
        table_->set_rows(std::move(rows));
        if (selected_ >= 0 && selected_ < static_cast<int>(entries_.size())) {
            table_->set_selected(selected_, false);
        }
    }

    int index_at(glm::vec2 pos) const
    {
        if (entries_.empty() || !global_bounds().contains(pos)) return -1;
        const float offset = pos.y - global_.pos.y - header_height() +
                             (table_ != nullptr ? table_->scroll_offset() : 0.0f);
        if (offset < 0.0f) return -1;
        const int index = static_cast<int>(offset / row_height());
        if (index < 0 || index >= static_cast<int>(entries_.size())) return -1;
        return index;
    }

    bool activate()
    {
        if (selected_ < 0 || selected_ >= static_cast<int>(entries_.size())) return false;
        const Entry& entry = entries_[static_cast<std::size_t>(selected_)];
        if (entry.is_directory) {
            if (entry.name == "..") {
                const std::filesystem::path current(directory_);
                if (!current.has_parent_path() || current.parent_path() == current) return true;
                set_directory(current.parent_path().string());
            } else {
                set_directory(resolve(entry.name));
            }
            if (on_directory_changed_) on_directory_changed_();
            return true;
        }
        if (on_activate_) on_activate_();
        return true;
    }

    std::vector<Entry> entries_;
    std::vector<TableColumn> columns_{{"Name", 0.0f},
                                      {"Size", 88.0f, TableColumn::Align::Right}};
    std::string directory_{"."};
    std::string extension_;
    std::string name_filter_;
    int selected_ = -1;
    float preferred_height_ = 372.0f;
    bool auto_width_ = false;
    Table* table_ = nullptr;

    std::function<void()> on_select_;
    std::function<void()> on_activate_;
    std::function<void()> on_directory_changed_;
};
} // namespace xel::ui
