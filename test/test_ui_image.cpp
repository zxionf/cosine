#include "ui/core/image.hpp"
#include "ui/containers/vbox.hpp"
#include "graphics/drawable.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>

namespace
{
bool nearly(float a, float b)
{
    return std::fabs(a - b) < 1e-3f;
}

void test_image_basic()
{
    xel::ui::Image image{120.0f, 80.0f};
    assert(nearly(image.width(), 120.0f));
    assert(nearly(image.height(), 80.0f));

    const glm::vec2 size = image.measure({200.0f, 200.0f});
    assert(nearly(size.x, 120.0f) && nearly(size.y, 80.0f));

    image.arrange({{10.0f, 20.0f}, {120.0f, 80.0f}});
    assert(nearly(image.global_bounds().pos.x, 10.0f));
    assert(nearly(image.global_bounds().pos.y, 20.0f));
}

void test_image_properties()
{
    xel::ui::Image image{100.0f, 100.0f};
    image.set_source("assets/tex/rock.png");
    image.set_texture_id(42);
    image.set_fit(xel::ui::Image::Fit::Cover);
    image.set_tint(glm::vec4{0.5f, 0.6f, 0.7f, 1.0f});

    assert(image.source() == "assets/tex/rock.png");
    assert(image.texture_id() == 42);
    assert(image.fit() == xel::ui::Image::Fit::Cover);
    assert(nearly(image.tint().x, 0.5f) && nearly(image.tint().z, 0.7f));
}

void test_placeholder_text_fallback()
{
    xel::ui::Image empty{10.0f, 10.0f};
    assert(empty.placeholder_text() == "image");

    empty.set_source("/tmp/some/path/photo.jpg");
    assert(empty.placeholder_text() == "photo.jpg");

    empty.set_source("C:\\tex\\tileset.png");
    assert(empty.placeholder_text() == "tileset.png");

    empty.set_placeholder_text("custom");
    assert(empty.placeholder_text() == "custom");
}

void test_image_in_vbox_percent_and_aspect()
{
    xel::ui::Vbox box{0.0f, 0.0f, 400.0f, 300.0f};
    box.set_padding(0.0f);
    auto image = std::make_unique<xel::ui::Image>(100.0f, 100.0f);
    image->set_size_percent({0.5f, 0.0f});
    image->set_aspect_ratio(16.0f / 9.0f);
    box.add(std::move(image));

    box.measure({400.0f, 300.0f});
    box.arrange({{0.0f, 0.0f}, {400.0f, 300.0f}});
    const auto& c = box.children();
    assert(nearly(c[0]->width(), 200.0f));
    assert(nearly(c[0]->height(), 200.0f / (16.0f / 9.0f)));
}
void test_image_placement()
{
    using Image = xel::ui::Image;
    const xel::ui::Rect rect{{0.0f, 0.0f}, {100.0f, 100.0f}};

    auto p = Image::compute_placement(rect, {200.0f, 100.0f}, Image::Fit::Fill);
    assert(nearly(p.dst.size.x, 100.0f) && nearly(p.dst.size.y, 100.0f));
    assert(nearly(p.uv.x, 0.0f) && nearly(p.uv.z, 1.0f));

    p = Image::compute_placement(rect, {200.0f, 100.0f}, Image::Fit::Contain);
    assert(nearly(p.dst.size.x, 100.0f) && nearly(p.dst.size.y, 50.0f));
    assert(nearly(p.dst.pos.x, 0.0f) && nearly(p.dst.pos.y, 25.0f));

    p = Image::compute_placement(rect, {100.0f, 200.0f}, Image::Fit::Contain);
    assert(nearly(p.dst.size.x, 50.0f) && nearly(p.dst.size.y, 100.0f));
    assert(nearly(p.dst.pos.x, 25.0f) && nearly(p.dst.pos.y, 0.0f));

    p = Image::compute_placement(rect, {200.0f, 100.0f}, Image::Fit::Cover);
    assert(nearly(p.dst.size.x, 200.0f) && nearly(p.dst.size.y, 100.0f));
    assert(nearly(p.dst.pos.x, -50.0f) && nearly(p.dst.pos.y, 0.0f));

    p = Image::compute_placement(rect, {100.0f, 200.0f}, Image::Fit::Cover);
    assert(nearly(p.dst.size.x, 100.0f) && nearly(p.dst.size.y, 200.0f));
    assert(nearly(p.dst.pos.x, 0.0f) && nearly(p.dst.pos.y, -50.0f));

    // 尺寸未知时回退 Fill。
    p = Image::compute_placement(rect, {0.0f, 0.0f}, Image::Fit::Cover);
    assert(nearly(p.dst.size.x, 100.0f) && nearly(p.dst.size.y, 100.0f));
}

void test_texture_render_slot_and_uv()
{
    xel::ui::Image image{100.0f, 100.0f};
    image.arrange({{0.0f, 0.0f}, {100.0f, 100.0f}});
    image.set_texture_id(3); // 图片槽索引 2 → slot 4
    image.set_texture_size({200.0f, 100.0f});
    image.set_fit(xel::ui::Image::Fit::Contain);

    xel::graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    image.append_to(list);

    bool found = false;
    for (const auto& batch : list.batches()) {
        if (batch.texture_slot != xel::graphics::image_slot(2)) continue;
        found = true;
        assert(batch.vertices.size() == 4);
        assert(nearly(batch.vertices[0].uv.x, 0.0f) && nearly(batch.vertices[0].uv.y, 0.0f));
        assert(nearly(batch.vertices[2].uv.x, 1.0f) && nearly(batch.vertices[2].uv.y, 1.0f));
        // Contain：200×100 放进 100×100 → 100×50，垂直居中。
        assert(nearly(batch.vertices[0].pos.y, 25.0f));
        assert(nearly(batch.vertices[2].pos.y, 75.0f));
    }
    assert(found);
}

void test_texture_tint_and_opacity()
{
    xel::ui::Image image{100.0f, 100.0f};
    image.arrange({{0.0f, 0.0f}, {100.0f, 100.0f}});
    image.set_texture_id(1);
    image.set_texture_size({100.0f, 100.0f});
    image.set_tint({0.5f, 0.25f, 1.0f, 0.5f});

    xel::graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    list.push_opacity(0.5f);
    image.append_to(list);
    list.pop_opacity();

    bool found = false;
    for (const auto& batch : list.batches()) {
        if (batch.texture_slot != xel::graphics::image_slot(0)) continue;
        found = true;
        const glm::vec4 c = batch.vertices[0].color;
        // tint 预乘 (0.25, 0.125, 0.5, 0.5)，再乘 DrawList opacity 0.5。
        assert(nearly(c.x, 0.125f) && nearly(c.y, 0.0625f));
        assert(nearly(c.z, 0.25f) && nearly(c.w, 0.25f));
    }
    assert(found);
}

void test_placeholder_has_no_texture_slot()
{
    xel::ui::Image image{100.0f, 100.0f};
    image.arrange({{0.0f, 0.0f}, {100.0f, 100.0f}});

    xel::graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    image.append_to(list);

    assert(!list.batches().empty());
    for (const auto& batch : list.batches()) {
        assert(!xel::graphics::is_image_slot(batch.texture_slot));
    }
}

void test_texture_background_and_border_flags()
{
    xel::ui::Image image{100.0f, 100.0f};
    image.arrange({{0.0f, 0.0f}, {100.0f, 100.0f}});
    image.set_texture_id(1);
    image.set_texture_size({100.0f, 100.0f});
    image.set_background({0.1f, 0.2f, 0.3f, 1.0f});
    assert(image.background_visible());

    xel::graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    image.append_to(list);
    assert(!list.batches().empty());
    // 底板使用字体图集槽（圆角矩形图元），纹理使用 image slot。
    assert(list.batches().front().texture_slot ==
           static_cast<std::uint32_t>(xel::graphics::TextureSlot::Texture));

    image.set_border_visible(false);
    assert(!image.border_visible());
}
} // namespace

int main()
{
    test_image_basic();
    test_image_properties();
    test_placeholder_text_fallback();
    test_image_in_vbox_percent_and_aspect();
    test_image_placement();
    test_texture_render_slot_and_uv();
    test_texture_tint_and_opacity();
    test_placeholder_has_no_texture_slot();
    test_texture_background_and_border_flags();
    std::cout << "test_ui_image: all passed\n";
    return 0;
}
