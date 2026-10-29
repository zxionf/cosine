#include "graphics/frame_stats.hpp"
#include "ui/composites/debug_overlay.hpp"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    xel::graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 100.0f, 100.0f});
    list.set_texture(xel::graphics::TextureSlot::Texture);

    auto& b0 = list.batch();
    b0.vertices.push_back({{0.0f, 0.0f}, glm::vec4{1.0f}, {0.0f, 0.0f}});
    b0.indices.push_back(0);

    list.push_clip({10.0f, 10.0f, 50.0f, 50.0f});
    auto& b1 = list.batch();
    b1.vertices.push_back({{10.0f, 10.0f}, glm::vec4{1.0f}, {0.0f, 0.0f}});
    b1.indices.push_back(0);

    const auto stats = xel::graphics::collect_draw_stats(list, 100.0f, 100.0f);
    assert(stats.batches == 2);
    assert(stats.draw_calls == 2);
    assert(stats.vertices == 2);
    assert(stats.indices == 2);
    assert(stats.clipped_batches == 1);

    xel::graphics::FrameStats frame;
    frame.frame_time_ms = 16.67;
    frame.fps = 60.0;
    frame.draw = stats;

    xel::ui::DebugOverlay overlay;
    assert(!overlay.visible());
    overlay.toggle();
    assert(overlay.visible());

    overlay.update(frame, nullptr, nullptr, {0.0f, 40.0f});
    const std::string& text = overlay.text();
    assert(text.find("FPS: 60.0") != std::string::npos);
    assert(text.find("draw calls: 2") != std::string::npos);
    assert(text.find("scroll: 0, 40") != std::string::npos);

    std::cout << "debug metrics tests passed\n";
    return 0;
}
