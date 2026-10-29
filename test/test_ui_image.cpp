#include "ui/core/image.hpp"
#include "ui/containers/vbox.hpp"

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
} // namespace

int main()
{
    test_image_basic();
    test_image_properties();
    test_placeholder_text_fallback();
    test_image_in_vbox_percent_and_aspect();
    std::cout << "test_ui_image: all passed\n";
    return 0;
}
