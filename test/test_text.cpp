#include "graphics/font_atlas.hpp"
#include "graphics/utf8.hpp"
#include "ui/label.hpp"
#include "ui/text_input.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace
{
void test_utf8()
{
    using namespace xel::graphics;

    const std::string text = "A中";
    std::size_t index = 0;
    assert(utf8_decode(text, index) == U'A');
    assert(utf8_decode(text, index) == U'中');
    assert(index == text.size());

    const std::string encoded = utf8_encode(U'中');
    assert(encoded.size() == 3);
    assert(encoded == "中");

    assert(utf8_next(text, 0) == 1);
    assert(utf8_next(text, 1) == 4);
    assert(utf8_prev(text, 4) == 1);
    assert(utf8_prev(text, 1) == 0);
    assert(utf8_codepoint_count(text) == 2);
}

void test_text_input_codepoints()
{
    xel::ui::TextInput input("", 200.0f);
    input.on_char(U'中');
    input.on_char(U'文');
    assert(input.text() == "中文");

    input.on_key_down(GLFW_KEY_BACKSPACE, 0);
    assert(input.text() == "中");

    input.on_key_down(GLFW_KEY_LEFT, 0);
    input.on_char(U'A');
    assert(input.text() == "A中");
}

void test_font_atlas_and_wrap()
{
    const auto asset = std::filesystem::path(__FILE__).parent_path().parent_path()
        / "assets" / "fonts" / "default8.png";

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* pixels = stbi_load(asset.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    assert(pixels != nullptr);

    xel::graphics::FontAtlas atlas;
    atlas.load(width, height, pixels);
    stbi_image_free(pixels);

    assert(atlas.valid());
    assert(atlas.measure("A", 16.0f).x > 0.0f);
    assert(atlas.measure("中文", 16.0f).x > 0.0f);
    assert(atlas.white_uv().x >= 0.0f && atlas.white_uv().x <= 1.0f);

    const auto wrapped = atlas.measure_wrapped("Hello World Hello World", 16.0f, 48.0f);
    assert(wrapped.y >= 32.0f);

    xel::graphics::DrawBatch batch;
    atlas.append_text(batch, 0.0f, 0.0f, 16.0f, "Hello", glm::vec4{1.0f});
    assert(!batch.vertices.empty());

    xel::graphics::DrawBatch wrapped_batch;
    atlas.append_text_wrapped(wrapped_batch, 0.0f, 0.0f, 16.0f, "Hello World Hello World",
                              glm::vec4{1.0f}, 48.0f, 2, true);
    assert(!wrapped_batch.vertices.empty());

    xel::graphics::set_active_font_atlas(&atlas);

    xel::ui::Label label("Hello World Hello World");
    label.set_wrap(true);
    label.set_max_lines(2);
    label.set_ellipsis(true);
    const glm::vec2 label_size = label.measure({48.0f, 200.0f});
    assert(label_size.y == 32.0f);
    label.arrange({{0.0f, 0.0f}, label_size});

    xel::graphics::DrawBatch label_batch;
    label.append_to(label_batch);
    assert(!label_batch.vertices.empty());

    xel::ui::TextInput input("中文", 200.0f);
    input.set_global_bounds({{0.0f, 0.0f}, {200.0f, 40.0f}});
    input.set_pos(0.0f, 0.0f);
    input.set_size(200.0f, 40.0f);
    input.on_mouse_down({1.0f, 20.0f}, 0);
    input.on_char(U'A');
    assert(input.text().front() == 'A');

    input.on_mouse_down({199.0f, 20.0f}, 0);
    input.on_char(U'B');
    assert(input.text().back() == 'B');

    xel::graphics::set_active_font_atlas(nullptr);
}
} // namespace

int main()
{
    test_utf8();
    test_text_input_codepoints();
    test_font_atlas_and_wrap();

    std::cout << "text tests passed\n";
    return 0;
}
