#include "ui/table.hpp"
#include "ui/ui_system.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>

namespace
{
using xel::event::InputEvent;
using xel::event::KeyCode;

InputEvent mouse(InputEvent::Type type, glm::vec2 pos)
{
    InputEvent event;
    event.type = type;
    event.mouse_pos = pos;
    return event;
}

InputEvent key(KeyCode code)
{
    InputEvent event;
    event.type = InputEvent::Type::KeyDown;
    event.key = code;
    return event;
}

void click(xel::ui::UISystem& ui, glm::vec2 pos)
{
    ui.dispatch(mouse(InputEvent::Type::MouseButtonDown, pos));
    ui.dispatch(mouse(InputEvent::Type::MouseButtonUp, pos));
}
} // namespace

int main()
{
    auto root = std::make_unique<xel::ui::UIRoot>(0.0f, 0.0f, 900.0f, 600.0f);

    auto table = std::make_unique<xel::ui::Table>(480.0f, 200.0f);
    table->set_pos(20.0f, 20.0f);
    table->set_columns({
        {"Name", 180.0f, xel::ui::TableColumn::Align::Left},
        {"Type", 120.0f, xel::ui::TableColumn::Align::Left},
        {"Size", 0.0f, xel::ui::TableColumn::Align::Right},
    });

    std::vector<std::vector<std::string>> rows;
    for (int i = 0; i < 12; ++i) {
        rows.push_back({"item_" + std::to_string(i),
                        (i % 2 == 0) ? "image" : "audio",
                        std::to_string(100 + i * 10) + " KB"});
    }
    table->set_rows(rows);
    table->set_selected(0);

    int select_calls = 0;
    int last_selected = -1;
    table->set_on_select([&](int index) {
        ++select_calls;
        last_selected = index;
    });

    auto* table_ptr = table.get();
    root->add(std::move(table));

    xel::ui::UISystem ui{std::move(root)};
    ui.tick();

    assert(table_ptr->row_count() == 12);
    assert(table_ptr->columns().size() == 3);
    assert(table_ptr->selected() == 0);
    assert(table_ptr->scroll_offset() == 0.0f);

    const auto bounds = table_ptr->global_bounds();
    assert(bounds.size.x == 480.0f);
    assert(bounds.size.y == 200.0f);
    assert(table_ptr->header_height() > 0.0f);

    // 点击第三行（行高 28，表头 30）。
    const float row_h = table_ptr->row_height();
    const float body_top = bounds.pos.y + table_ptr->header_height();
    click(ui, {bounds.pos.x + 40.0f, body_top + row_h * 2.5f});
    assert(table_ptr->selected() == 2);
    assert(select_calls == 1);
    assert(last_selected == 2);

    // 点击表头不改变选中。
    click(ui, {bounds.pos.x + 40.0f, bounds.pos.y + 5.0f});
    assert(table_ptr->selected() == 2);

    // 键盘导航。
    ui.set_focus(table_ptr);
    ui.dispatch(key(KeyCode::Down));
    assert(table_ptr->selected() == 3);
    ui.dispatch(key(KeyCode::Up));
    assert(table_ptr->selected() == 2);
    ui.dispatch(key(KeyCode::End));
    assert(table_ptr->selected() == 11);
    ui.dispatch(key(KeyCode::Home));
    assert(table_ptr->selected() == 0);

    // 选择越界被 clamp。
    table_ptr->set_selected(999);
    assert(table_ptr->selected() == 11);

    // 滚轮滚动：内容高度 12*28=336 > 视口 170。
    table_ptr->set_selected(0);
    // 离散模式：滚轮立即生效（平滑模式会延迟到下一帧）。
    table_ptr->set_scroll_mode(xel::ui::ScrollMode::Line);
    const float before = table_ptr->scroll_offset();
    ui.dispatch([] {
        InputEvent event;
        event.type = InputEvent::Type::MouseScroll;
        event.mouse_pos = {40.0f, 120.0f};
        event.scroll_delta = -1.0f;
        return event;
    }());
    assert(table_ptr->scroll_offset() > before);

    // 键盘选择最后一行应自动滚动到可见。
    table_ptr->set_selected(11);
    assert(table_ptr->scroll_offset() > 0.0f);

    // 清空行后 selected 归 -1，且键盘事件不崩。
    table_ptr->clear_rows();
    assert(table_ptr->row_count() == 0);
    assert(table_ptr->selected() == -1);
    ui.dispatch(key(KeyCode::Down));
    assert(table_ptr->selected() == -1);

    // 添加行后可再次选择。
    table_ptr->add_row({"only", "row", "1 KB"});
    assert(table_ptr->row_count() == 1);
    table_ptr->set_selected(0);
    assert(table_ptr->selected() == 0);

    std::cout << "table tests passed\n";
    return 0;
}
