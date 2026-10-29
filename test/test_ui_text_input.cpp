#include "ui/core/text_input.hpp"

#include "graphics/font_atlas.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include <cassert>
#include <iostream>

namespace
{
using xel::event::KeyCode;
using xel::event::Modifier;

constexpr Modifier kNone = Modifier::None;
constexpr Modifier kShift = Modifier::Shift;

void test_single_line_regression()
{
    xel::ui::TextInput input("hello", 200.0f);
    assert(!input.multiline());
    assert(input.text() == "hello");
    assert(input.cursor() == 5);

    // 单行模式 Enter 不插入换行。
    assert(!input.on_key_down(KeyCode::Enter, kNone));
    assert(input.text() == "hello");

    // 单行模式 Up/Down 不处理。
    assert(!input.on_key_down(KeyCode::Up, kNone));
    assert(!input.on_key_down(KeyCode::Down, kNone));

    input.on_char(U'!');
    assert(input.text() == "hello!");

    input.on_key_down(KeyCode::Home, kNone);
    assert(input.cursor() == 0);
    input.on_key_down(KeyCode::End, kNone);
    assert(input.cursor() == 6);

    // Ctrl+A 全选后输入替换。
    input.on_key_down(KeyCode::A, Modifier::Control);
    assert(input.selection_begin() == 0);
    assert(input.selection_end() == 6);
    input.on_char(U'X');
    assert(input.text() == "X");

    // 测量仍是单行高度。
    const auto size = input.measure({300.0f, 300.0f});
    assert(size.x == 200.0f);
    assert(size.y > 0.0f);
}

void test_multiline_basics()
{
    xel::ui::TextInput input("ab", 200.0f);
    input.set_multiline(true);
    assert(input.multiline());

    input.on_char(U'\n');
    assert(input.text() == "ab\n");
    input.on_char(U'c');
    assert(input.text() == "ab\nc");
    assert(input.cursor() == 4);

    // 三行高度应大于两行。
    input.set_max_height(0.0f);
    const float three = input.measure({200.0f, 1000.0f}).y;
    input.set_text("ab\nc");
    assert(input.measure({200.0f, 1000.0f}).y == three);

    // Backspace 能删除换行。
    assert(input.on_key_down(KeyCode::Backspace, kNone));
    assert(input.text() == "ab\n");
    assert(input.on_key_down(KeyCode::Backspace, kNone));
    assert(input.text() == "ab");

    // 切回单行会把换行折叠成空格。
    xel::ui::TextInput folded("a\nb", 200.0f);
    folded.set_multiline(true);
    folded.set_multiline(false);
    assert(folded.text() == "a b");
    assert(!folded.multiline());
}

void test_multiline_navigation()
{
    xel::ui::TextInput input("abc\ndefgh\nij", 300.0f);
    input.set_multiline(true);
    input.set_preferred_size({300.0f, 200.0f});
    input.arrange({{0.0f, 0.0f}, {300.0f, 200.0f}});

    // 光标在末尾（第 3 行 "ij" 之后）。
    assert(input.cursor() == input.text().size());

    input.on_key_down(KeyCode::Up, kNone);
    std::size_t after_up = input.cursor();
    assert(after_up >= 4 && after_up <= 9); // 落在第二行 "defgh" 范围内（9 = 行尾）

    // Home 到行首，End 到行尾。
    input.on_key_down(KeyCode::Home, kNone);
    assert(input.cursor() == 4);
    input.on_key_down(KeyCode::End, kNone);
    assert(input.cursor() == 9);

    // 顶部再 Up 保持不动。
    input.on_key_down(KeyCode::Home, kNone);
    input.on_key_down(KeyCode::Up, kNone);
    input.on_key_down(KeyCode::Up, kNone);
    input.on_key_down(KeyCode::Home, kNone);
    assert(input.cursor() == 0);
    input.on_key_down(KeyCode::Up, kNone);
    assert(input.cursor() == 0);

    // 底部再 Down 保持不动。
    input.on_key_down(KeyCode::End, kNone);
    input.on_key_down(KeyCode::Down, kNone);
    input.on_key_down(KeyCode::Down, kNone);
    input.on_key_down(KeyCode::End, kNone);
    assert(input.cursor() == input.text().size());
    input.on_key_down(KeyCode::Down, kNone);
    assert(input.cursor() == input.text().size());
}

void test_multiline_selection()
{
    xel::ui::TextInput input("abc\ndef", 300.0f);
    input.set_multiline(true);
    input.arrange({{0.0f, 0.0f}, {300.0f, 200.0f}});

    // Ctrl+A 覆盖换行在内的全部内容。
    input.on_key_down(KeyCode::A, Modifier::Control);
    assert(input.selection_begin() == 0);
    assert(input.selection_end() == 7);

    // 输入替换整段。
    input.on_char(U'z');
    assert(input.text() == "z");

    // Shift+Home 选中当前行前半部分。
    input.set_text("hello world");
    input.on_key_down(KeyCode::End, kNone);
    input.on_key_down(KeyCode::Home, kShift);
    assert(input.selection_begin() == 0);
    assert(input.selection_end() == 11);
}

void test_multiline_newline_via_enter()
{
    xel::ui::TextInput input("", 200.0f);
    input.set_multiline(true);
    input.arrange({{0.0f, 0.0f}, {200.0f, 120.0f}});

    input.on_char(U'a');
    assert(input.on_key_down(KeyCode::Enter, kNone));
    input.on_char(U'b');
    assert(input.on_key_down(KeyCode::KeypadEnter, kNone));
    input.on_char(U'c');
    assert(input.text() == "a\nb\nc");
    assert(input.cursor() == 5);
}

void test_single_line_vertical_centering()
{
    // 合成图集：行高 1.5em（> 字号），用于暴露「文字偏下」的回归。
    xel::graphics::FontAtlas atlas;
    atlas.begin_generated(128, {1.15f, 0.35f, 1.5f}, true, 16);
    std::vector<unsigned char> bitmap(6 * 10, 220);
    atlas.add_glyph(U'h', bitmap.data(), 6, 10, 6, 0.6f, 0.0f, 0.8f, 16.0f);
    xel::graphics::set_active_font_atlas(&atlas);

    xel::ui::TextInput input("h", 200.0f);
    input.arrange({{10.0f, 20.0f}, {200.0f, 30.0f}});

    xel::graphics::DrawList list;
    input.append_to(list);

    const glm::vec4 text_color = input.ui_theme().text;
    float ink_min_y = 1.0e9f;
    float ink_max_y = -1.0e9f;
    for (const auto& batch : list.batches()) {
        for (const auto& vertex : batch.vertices) {
            if (vertex.color != text_color) continue;
            ink_min_y = std::min(ink_min_y, vertex.pos.y);
            ink_max_y = std::max(ink_max_y, vertex.pos.y);
        }
    }
    assert(ink_min_y < ink_max_y);

    const float box_center = 20.0f + 30.0f * 0.5f;
    const float ink_center = (ink_min_y + ink_max_y) * 0.5f;
    assert(std::fabs(ink_center - box_center) <= 2.0f);

    // 行高小于控件高度时，文字上方与下方留白应大致相等。
    const float margin_top = ink_min_y - 20.0f;
    const float margin_bottom = 50.0f - ink_max_y;
    assert(std::fabs(margin_top - margin_bottom) <= 4.0f);

    xel::graphics::set_active_font_atlas(nullptr);
}
void test_height_cap()
{
    xel::ui::TextInput input("1\n2\n3\n4\n5", 200.0f);
    input.set_multiline(true);
    input.set_max_height(80.0f);
    const auto size = input.measure({200.0f, 1000.0f});
    assert(size.y <= 80.0f + 0.01f);

    // 未设上限时按行数增长。
    input.set_max_height(0.0f);
    const auto tall = input.measure({200.0f, 1000.0f});
    assert(tall.y > size.y);
}
} // namespace

int main()
{
    test_single_line_regression();
    test_multiline_basics();
    test_multiline_navigation();
    test_multiline_selection();
    test_multiline_newline_via_enter();
    test_height_cap();
    test_single_line_vertical_centering();

    std::cout << "ui text input tests passed\n";
    return 0;
}
