#pragma once

#include "scroll_view.hpp"
#include "vbox.hpp"

#include <memory>

namespace xel::ui
{
class List : public ScrollView
{
public:
    List(float width, float height)
    {
        set_size(width, height);

        auto content = std::make_unique<Vbox>(0.0f, 0.0f, width, 0.0f);
        content_ = content.get();
        set_content(std::move(content));
    }

    void add_item(std::unique_ptr<Widget> item)
    {
        content_->add(std::move(item));
        mark_layout_dirty();
    }

    void clear_items()
    {
        content_->clear_children();
        mark_layout_dirty();
    }

    std::size_t item_count() const { return content_->children().size(); }

    void set_item_spacing(float spacing)
    {
        content_->set_spacing(spacing);
    }

private:
    Vbox* content_ = nullptr;
};
} // namespace xel::ui
